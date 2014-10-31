#include "wfd.h"
#include <string.h>
#include <glib.h>
#include <dbus/dbus-glib.h>
#include "wpas_dbus.h"

#define WPAS_DBUS_INTERFACE_IFACE      WPAS_DBUS_SERVICE_NAME    ".Interface"
#define WPAS_DBUS_INTERFACE_WPS_IFACE  WPAS_DBUS_INTERFACE_IFACE ".WPS"
#define WPAS_DBUS_INTERFACE_P2P_IFACE  WPAS_DBUS_INTERFACE_IFACE ".P2PDevice"

struct WfdGroup
{
  const char *opath;
  DBusGProxy *proxy;
  GPtrArray  *WfdPeer;
};

struct WfdPeer
{
  const char *opath;
  DBusGProxy *proxy;
  int connection_state;
};

struct WlanInterface
{
  int error;
  const char* opath;
  DBusGConnection *bus;
  DBusGProxy *proxy;
  DBusGProxy *proxy_wps;
  DBusGProxy *proxy_p2p;
  DBusGProxy *proxy_props;

  gboolean display_defined;
  struct WlanDisplayInfo display_info;

  int group_state;
  struct WfdGroup *group;

  GPtrArray *peers;

  struct WlanCallbacks *cb;
  struct WpaSupplicant *wpas;
};

int serialize_wlan_display_info(struct WlanDisplayInfo *info, GByteArray *bytes)
{
  g_assert(NULL != info);
  g_assert(NULL != bytes);

  uint8_t ie_buffer[] =
    { 0
    , (info->device_type  & 0x03) << 0
    | (info->available    & 0x03) << 4
    | (info->connectivity & 0x01) << 6 
    , info->control_port >> 8
    , info->control_port
    , info->bandwidth >> 8
    , info->bandwidth
    };
  uint16_t ie_size = sizeof(ie_buffer);
  uint8_t  ie_id = 0;

  uint16_t ie_size_be = GUINT16_TO_BE(ie_size); 

  g_byte_array_append(bytes, &ie_id,      sizeof(ie_id));
  g_byte_array_append(bytes, (uint8_t*)&ie_size_be, sizeof(ie_size));
  g_byte_array_append(bytes,  ie_buffer,  ie_size);

  return WPAS_RESULT_OK;
}

int _wlan_init(struct WlanInterface *wfd, const char* dbus_path)
{
  g_debug("_wlan_init(%p, %s)", wfd, dbus_path);
  g_assert(NULL != wpas_get_dbus_g_connection());

  wfd->opath = dbus_path;
  wfd->bus = wpas_get_dbus_g_connection();

  g_assert(WPAS_RESULT_OK == wpas_create(&wfd->wpas));

  g_message("Getting \'%s\' iface for \'%s\' object.", WPAS_DBUS_INTERFACE_IFACE, dbus_path);
  wfd->proxy = dbus_g_proxy_new_for_name(
      wfd->bus,
      WPAS_DBUS_SERVICE_NAME,
      dbus_path,
      WPAS_DBUS_INTERFACE_IFACE);
  g_return_val_if_fail(NULL != wfd->proxy, WPAS_RESULT_ERROR);

  g_message("Getting \'%s\' iface for \'%s\' object.", DBUS_INTERFACE_PROPERTIES, dbus_path);
  wfd->proxy_props = dbus_g_proxy_new_from_proxy(
      wfd->proxy,
      DBUS_INTERFACE_PROPERTIES,
      dbus_path);
  g_return_val_if_fail(NULL != wfd->proxy_props, WPAS_RESULT_ERROR);

  g_message("Getting \'%s\' iface for \'%s\' object.", WPAS_DBUS_INTERFACE_WPS_IFACE, dbus_path);
  wfd->proxy_wps = dbus_g_proxy_new_from_proxy(
      wfd->proxy,
      WPAS_DBUS_INTERFACE_WPS_IFACE,
      dbus_path);
  g_return_val_if_fail(NULL != wfd->proxy_wps, WPAS_RESULT_ERROR);

  g_message("Getting \'%s\' iface for \'%s\' object.", WPAS_DBUS_INTERFACE_P2P_IFACE, dbus_path);
  wfd->proxy_p2p = dbus_g_proxy_new_from_proxy(
      wfd->proxy,
      WPAS_DBUS_INTERFACE_P2P_IFACE,
      dbus_path);
  g_return_val_if_fail(NULL != wfd->proxy_p2p, WPAS_RESULT_ERROR);

  return WPAS_RESULT_OK;
}

int _wlan_deinit(struct WlanInterface *wfd)
{
  g_debug("_wlan_deinit(%p)", wfd);
  g_assert(NULL != wfd);

  wpas_release(&wfd->wpas);

  return WPAS_RESULT_OK;
}

int wlan_create(struct WlanInterface** o_wlan, const char* dbus_path)
{
  g_message("-> wlan_create(%p %p, %s)", o_wlan, *o_wlan, dbus_path);
  g_assert(NULL != o_wlan);
  g_assert(NULL == *o_wlan);
  g_assert(NULL != dbus_path);

  *o_wlan = g_new0(struct WlanInterface, 1);
  g_return_val_if_fail(NULL != *o_wlan, WPAS_RESULT_ERROR);

  int result = WPAS_RESULT_OK;

  if (WPAS_RESULT_OK != (result = _wlan_init(*o_wlan, dbus_path)))
  {
    g_free(*o_wlan);
    *o_wlan = NULL;
  }

  return result;
}

int wlan_release(struct WlanInterface** wfd)
{
  g_debug("-> wlan_release(%p)", wfd);
  g_assert(NULL != wfd);
  g_assert(NULL != *wfd);

  _wlan_deinit(*wfd);
  g_free(*wfd);
  *wfd = NULL;

  return WPAS_RESULT_OK;
}

int wlan_set_callbacks(struct WlanInterface* wfd, struct WlanCallbacks* cb)
{
  g_debug("-> wlan_set_callbacks(%p)", wfd);
  g_assert(NULL != wfd);

  wfd->cb = cb;

  return WPAS_RESULT_OK;
}

int wlan_set_display(struct WlanInterface* wfd, struct WlanDisplayInfo *info)
{
  g_debug("wlan_set_display(%p)", wfd);
  g_assert(NULL != wfd);

  int result = WPAS_RESULT_OK;

  if (NULL == info)
  {
    wfd->display_defined = FALSE;
  }
  else
  {
    wfd->display_defined = TRUE;
    g_memmove(info, &wfd->display_info, sizeof(struct WlanDisplayInfo));

    GByteArray *bytes = g_byte_array_new();
    if (WPAS_RESULT_OK == (result = serialize_wlan_display_info(info, bytes)))
    {
      result = wpas_set_wfd_ie(wfd->wpas, bytes->data, bytes->len);
    }
    g_byte_array_free(bytes, TRUE);
  }

  return result;
}

int wlan_start_autonomous_group(struct WlanInterface*);
int wlan_stop_autonomous_group(struct WlanInterface*);
int wlan_accept_connection(struct WlanInterface*, const char* mac_address);
int wlan_reject_connection(struct WlanInterface*, const char* mac_address);
