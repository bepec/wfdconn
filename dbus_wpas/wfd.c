#include "wfd.h"
#include <string.h>
#include <glib.h>
#include <dbus/dbus-glib.h>
#include "dbus_base.h"
#include "wpas_dbus.h"
#include "wfd_peer.h"
#include "proxy_wpa_supplicant1_Interface.h"

#define WPAS_DBUS_INTERFACE_IFACE      WPAS_DBUS_SERVICE_NAME    ".Interface"
#define WPAS_DBUS_INTERFACE_WPS_IFACE  WPAS_DBUS_INTERFACE_IFACE ".WPS"
#define WPAS_DBUS_INTERFACE_P2P_IFACE  WPAS_DBUS_INTERFACE_IFACE ".P2PDevice"

#define WLAN_P2P_NO_GROUP_OPATH "/"
#define WLAN_P2P_GROUP_ROLE_GO  "GO"

struct WlanInterface
{
  const char* opath;
  DBusGConnection *bus;
  DBusGProxy *proxy;
  DBusGProxy *props;

  DBusGProxy *wps;
  DBusGProxy *p2p;

  gboolean display_defined;
  struct WlanDisplayInfo display_info;

  int group_state;
  struct WfdGroup *group;

  GHashTable* peers;
  struct WfdPeer* connecting_peer;

  struct WlanCallbacks *cb;
  struct WpaSupplicant *wpas;
};

struct DBusBase* wlan_get_dbus_base(struct WlanInterface *wlan)
{
    return (struct DBusBase*)wlan;
}

void _destroy_peer(gpointer data)
{
  g_assert(NULL != data);
  struct WfdPeer* peer = data;
  peer_release(&peer);
  g_assert(NULL == peer);
}

void handle_sta_authorized(DBusGProxy *proxy, const char* name, struct WlanInterface* wlan)
{
  g_debug("handle_sta_authorized(%p, %s, %p)", proxy, name, wlan);
  g_assert(NULL != proxy);
  g_assert(NULL != name);
  g_assert(NULL != wlan);

  if (NULL == wlan->connecting_peer)
  {
    g_warning("Unexpected STA authorized, no peer candidate for connection.");
  }
  else if (NULL != wlan->cb && NULL != wlan->cb->cb_device_connected)
  {
    const char* p2p_mac = peer_get_mac(wlan->connecting_peer);
    wlan->cb->cb_device_connected(wlan, p2p_mac, name);
  }
}

void handle_device_found(DBusGProxy *proxy, const char* opath, struct WlanInterface* wlan)
{
  g_debug("handle_device_found(%p, %s, %p)", proxy, opath, wlan);
  g_assert(NULL != proxy);
  g_assert(NULL != opath);
  g_assert(NULL != wlan);

  if (TRUE == g_hash_table_contains(wlan->peers, opath))
  {
    g_warning("device is already registered: %s", opath);
  }
  else
  {
    struct WfdPeer* peer = NULL;
    g_assert(WPAS_RESULT_OK == peer_create(&peer, opath));
    g_assert(NULL != peer);
    g_hash_table_insert(wlan->peers, g_strdup(opath), peer);
    g_message("peer device registered: %s.", peer_trace(peer));
  }
}

void handle_device_lost(DBusGProxy *proxy, const char* opath, struct WlanInterface* wlan)
{
  g_debug("handle_device_lost(%p, %s, %p)", proxy, opath, wlan);
  g_assert(NULL != proxy);
  g_assert(NULL != opath);
  g_assert(NULL != wlan);

  if (TRUE == g_hash_table_contains(wlan->peers, opath))
  {
    struct WfdPeer* peer = g_hash_table_lookup(wlan->peers, opath);
    g_assert(NULL != peer);
    if (peer == wlan->connecting_peer) wlan->connecting_peer = NULL;
    g_assert(TRUE == g_hash_table_remove(wlan->peers, opath));
    g_message("Peer device unregistered: %s.", opath);
  }
  else
  {
    g_warning("Unknown peer device: %s", opath);
  }
}

void handle_provision_discovery_pbc_request(DBusGProxy* proxy, const char* opath, struct WlanInterface* wlan)
{
  g_debug("handle_provision_discovery_pbc_request(%p, %s, %p)", proxy, opath, wlan);
  g_assert(NULL != proxy);
  g_assert(NULL != opath);
  g_assert(NULL != wlan);

  if (TRUE == g_hash_table_contains(wlan->peers, opath))
  {
    struct WfdPeer* peer = g_hash_table_lookup(wlan->peers, opath);
    g_assert(NULL != peer);
    g_message("Detected PBC request from device: %s", peer_trace(peer));
    wlan->connecting_peer = peer;

    if (NULL != wlan->cb && NULL != wlan->cb->cb_connection_request)
    {
      wlan->cb->cb_connection_request(
              wlan,
              peer_get_mac(peer),
              peer_get_name(peer),
              peer_get_wfd_ie(peer));
    }
  }
  else
  {
    g_warning("Unknown peer device: %s", opath);
  }
}

void handle_group_started(DBusGProxy *proxy, GHashTable *props, gpointer wlan)
{
  g_debug("handle_group_started(%p, %p, %p)", proxy, props, wlan);
  g_assert(NULL != proxy);
  g_assert(NULL != props);
  g_assert(NULL != wlan);

  g_message("Params size: %u.", g_hash_table_size(props));

  GValue *value = g_hash_table_lookup(props, "group_object");
  if (NULL != value)
  {
    const char *group_opath = g_value_get_boxed(value);
    g_message("Group path: %s", group_opath);
  }
}

void handle_group_finished(DBusGProxy *proxy, GHashTable *props, struct WlanInterface* wlan)
{
  g_debug("handle_group_finished(%p, %p, %p)", proxy, props, wlan);
  g_assert(NULL != proxy);
  g_assert(NULL != props);
  g_assert(NULL != wlan);

  g_debug("Params size: %u.", g_hash_table_size(props));

  GValue* value = g_hash_table_lookup(props, "group_object");
  g_debug("Value: %s", (const char*)g_value_get_boxed(value));
  /* if (NULL != value) */
  /* { */
    /* g_message("Found group_object: %s", G_VALUE_TYPE_NAME(props)); */
    /* g_message("Group path: %s", group_opath); */
  /* } */
}

int pack_wlan_display_info(struct WlanDisplayInfo *info, GByteArray *bytes)
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
  uint16_t ie_size_be = GUINT16_TO_BE(ie_size); 
  uint8_t  ie_id = 0;

  g_byte_array_append(bytes, &ie_id,      sizeof(ie_id));
  g_byte_array_append(bytes, (uint8_t*)&ie_size_be, sizeof(ie_size));
  g_byte_array_append(bytes,  ie_buffer,  ie_size);

  return WPAS_RESULT_OK;
}


/* void _iterate_wfd_peers(const GValue *value, gpointer data) */
/* { */
    /* g_debug("_iterate_wpas_interfaces(value=%s, data=%p)", (char*)g_value_get_boxed(value), data); */
    /* g_assert(NULL != value); */
    /* g_assert(NULL != data); */

    /* struct WpaSupplicant* wpas = data; */
    /* g_ptr_array_add(wpas->ifaces, g_value_get_boxed(value)); */
/* } */

gboolean _find_peer_by_mac(gpointer key, gpointer value, gpointer mac)
{
  g_assert(NULL != value);
  struct WfdPeer* peer = value;
  return (strcmp((const char*)mac, peer_get_mac(peer)) == 0)
         ? TRUE
         : FALSE;
}

int _wlan_init_peers(struct WlanInterface *wlan)
{
  g_debug("_wlan_init_peers(%p)", wlan);
  g_assert(NULL != wlan);

  GValue peers = {0,};

  g_assert(TRUE == DBUS_GET_PROPERTY(wlan,
              WPAS_DBUS_INTERFACE_P2P_IFACE,
              "Peers",
              &peers));
  g_assert(dbus_g_type_is_collection(G_VALUE_TYPE(&peers)));
  /* dbus_g_type_collection_value_iterate(&peers, _iterate_wfd_peers, wlan); */

  return WPAS_RESULT_OK;
}

int _wlan_init(struct WlanInterface *wlan, const char* dbus_path)
{
  g_debug("_wlan_init(%p, %s)", wlan, dbus_path);
  g_assert(NULL != wpas_get_dbus_g_connection());

  int result;

  wlan->peers = g_hash_table_new_full(g_str_hash,
                                      g_str_equal,
                                      g_free,
                                      _destroy_peer);

  result = wpas_create(&wlan->wpas);
  g_return_val_if_fail(WPAS_RESULT_OK == result, result);
  g_assert(NULL != wlan->wpas);

  result = dbusbase_init(
      (struct DBusBase*)wlan,
      wpas_get_dbus_g_connection(),
      WPAS_DBUS_SERVICE_NAME,
      dbus_path,
      WPAS_DBUS_INTERFACE_IFACE);
  g_return_val_if_fail(WPAS_RESULT_OK == result, result);

  g_debug("Getting \'%s\' iface for \'%s\' object.", WPAS_DBUS_INTERFACE_WPS_IFACE, dbus_path);
  wlan->wps = dbus_g_proxy_new_from_proxy(
      wlan->proxy,
      WPAS_DBUS_INTERFACE_WPS_IFACE,
      dbus_path);
  g_return_val_if_fail(NULL != wlan->wps, WPAS_RESULT_ERROR);

  g_debug("Getting \'%s\' iface for \'%s\' object.", WPAS_DBUS_INTERFACE_P2P_IFACE, dbus_path);
  wlan->p2p = dbus_g_proxy_new_from_proxy(
      wlan->proxy,
      WPAS_DBUS_INTERFACE_P2P_IFACE,
      dbus_path);
  g_return_val_if_fail(NULL != wlan->p2p, WPAS_RESULT_ERROR);

  _wlan_init_peers(wlan);

  dbus_g_proxy_add_signal(
      wlan->proxy,
      "StaAuthorized",
      G_TYPE_STRING,
      G_TYPE_INVALID);

  dbus_g_proxy_connect_signal(
      wlan->proxy,
      "StaAuthorized",
      G_CALLBACK(handle_sta_authorized),
      wlan, NULL);

  dbus_g_proxy_add_signal(
      wlan->p2p,
      "GroupStarted",
      dbus_g_type_get_map("GHashTable", G_TYPE_STRING, G_TYPE_VALUE),
      G_TYPE_INVALID);

  dbus_g_proxy_connect_signal(
      wlan->p2p,
      "GroupStarted",
      G_CALLBACK(handle_group_started),
      wlan, NULL);

  dbus_g_proxy_add_signal(
      wlan->p2p,
      "GroupFinished",
      dbus_g_type_get_map("GHashTable", G_TYPE_STRING, G_TYPE_VALUE),
      G_TYPE_INVALID);

  dbus_g_proxy_connect_signal(
      wlan->p2p,
      "GroupFinished",
      G_CALLBACK(handle_group_finished),
      wlan, NULL);

  dbus_g_proxy_add_signal(
      wlan->p2p,
      "DeviceFound",
      DBUS_TYPE_G_OBJECT_PATH,
      G_TYPE_INVALID);

  dbus_g_proxy_connect_signal(
      wlan->p2p,
      "DeviceFound",
      G_CALLBACK(handle_device_found),
      wlan, NULL);

  dbus_g_proxy_add_signal(
      wlan->p2p,
      "DeviceLost",
      DBUS_TYPE_G_OBJECT_PATH,
      G_TYPE_INVALID);

  dbus_g_proxy_connect_signal(
      wlan->p2p,
      "DeviceLost",
      G_CALLBACK(handle_device_lost),
      wlan, NULL);

  dbus_g_proxy_add_signal(
      wlan->p2p,
      "ProvisionDiscoveryPBCRequest",
      DBUS_TYPE_G_OBJECT_PATH,
      G_TYPE_INVALID);

  dbus_g_proxy_connect_signal(
      wlan->p2p,
      "ProvisionDiscoveryPBCRequest",
      G_CALLBACK(handle_provision_discovery_pbc_request),
      wlan, NULL);

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
  g_debug("wlan_create(%p %p, %s)", o_wlan, *o_wlan, dbus_path);
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
  g_debug("wlan_release(%p)", wfd);
  g_assert(NULL != wfd);
  g_assert(NULL != *wfd);

  _wlan_deinit(*wfd);
  g_free(*wfd);
  *wfd = NULL;

  return WPAS_RESULT_OK;
}

int wlan_set_callbacks(struct WlanInterface* wfd, struct WlanCallbacks* cb)
{
  g_debug("wlan_set_callbacks(%p, %p)", wfd, cb);
  g_assert(NULL != wfd);

  wfd->cb = cb;

  return WPAS_RESULT_OK;
}

int wlan_set_config_methods(struct WlanInterface *wlan, const char *methods)
{
  g_debug("wlan_set_config_methods(%p, %s)", wlan, methods);
  g_assert(NULL != wlan);
  g_assert(NULL != methods);

  int result = WPAS_RESULT_OK;

  GError *error = NULL;
  GValue  value = {0,};

  g_value_init(&value, G_TYPE_STRING);
  g_value_set_string(&value, methods);

  if (TRUE != org_freedesktop_DBus_Properties_set(
                  wlan->props,
                  WPAS_DBUS_INTERFACE_WPS_IFACE,
                  "ConfigMethods",
                  &value,
                  &error))
  {
    g_assert(NULL != error);
    g_error("org_freedesktop_DBus_Properties_set(..): %s",
            error->message);
    result = WPAS_RESULT_ERROR;
  }

  return result;
}

int wlan_set_display(struct WlanInterface* wlan, struct WlanDisplayInfo *info)
{
  g_debug("wlan_set_display(%p)", wlan);
  g_assert(NULL != wlan);

  int result = WPAS_RESULT_OK;

  if (NULL == info)
  {
    wlan->display_defined = FALSE;
  }
  else
  {
    wlan->display_defined = TRUE;
    g_memmove(&wlan->display_info, info, sizeof(struct WlanDisplayInfo));

    GByteArray *bytes = g_byte_array_new();
    if (NULL != info)
    {
        result = pack_wlan_display_info(info, bytes);
    }
    if (WPAS_RESULT_OK == result)
    {
      result = wpas_set_wfd_ie(wlan->wpas, bytes->data, bytes->len);
    }
    g_byte_array_free(bytes, TRUE);
  }

  return result;
}

int wlan_get_group_info(struct WlanInterface *wlan, int *o_value)
{
  g_debug("wlan_get_group_info(%p)", wlan);
  g_assert(NULL != wlan);
  g_assert(NULL != o_value);

  int result = WPAS_RESULT_OK;
  GValue  group = {0,};
  GValue  role  = {0,};

  g_return_val_if_fail(DBUS_GET_PROPERTY(
      wlan,
      WPAS_DBUS_INTERFACE_P2P_IFACE,
      "Group",
      &group) == TRUE,
    WPAS_RESULT_ERROR);

  g_return_val_if_fail(DBUS_GET_PROPERTY(
      wlan,
      WPAS_DBUS_INTERFACE_P2P_IFACE,
      "Role",
      &role) == TRUE,
    WPAS_RESULT_ERROR);

  g_assert(G_VALUE_HOLDS_BOXED(&group));
  char *group_content = g_value_get_boxed(&group);
  g_debug("Group: %s", group_content);
  if (strcmp(group_content, WLAN_P2P_NO_GROUP_OPATH) == 0)
  {
    *o_value = WPAS_P2P_GROUP_NONE;
  }
  else
  {
    const char* role_content = g_value_get_string(&role);
    g_debug("Role: %s", role_content);
    *o_value = 
        (strcmp(role_content, WLAN_P2P_GROUP_ROLE_GO) == 0)
        ? WPAS_P2P_GROUP_OWNER
        : WPAS_P2P_GROUP_PEER;
  }
  g_free(group_content);

  g_debug("wlan_get_group_info(): %d", *o_value);
  return result;
}

int wlan_start_autonomous_group(struct WlanInterface *wlan)
{
  g_debug("wlan_start_autonomous_group(%p)", wlan);
  g_assert(NULL != wlan);

  int result = WPAS_RESULT_OK;
  GHashTable *params;
  GError     *error = NULL;

  params = g_hash_table_new(NULL, NULL);

  if (TRUE != fi_w1_wpa_supplicant1_Interface_P2PDevice_group_add(
                  wlan->p2p,
                  params,
                  &error))
  {
    g_assert(NULL != error);
    g_error("fi_w1_wpa_supplicant1_Interface_P2PDevice_group_add(..): %s",
            error->message);
    result = WPAS_RESULT_ERROR;
  }

  return result;
}

int wlan_flush(struct WlanInterface *wlan)
{
  g_debug("wlan_flush(%p)", wlan);
  g_assert(NULL != wlan);

  int result = WPAS_RESULT_OK;
  GError *error = NULL;

  if (TRUE != fi_w1_wpa_supplicant1_Interface_P2PDevice_flush(
                  wlan->p2p,
                  &error))
  {
    g_assert(NULL != error);
    g_error("fi_w1_wpa_supplicant1_Interface_P2PDevice_flush(..): %s",
            error->message);
    result = WPAS_RESULT_ERROR;
  }

  /* if (TRUE != fi_w1_wpa_supplicant1_Interface_remove_all_networks( */
                  /* wlan->proxy, */
                  /* &error)) */
  /* { */
    /* g_warning("fi_w1_wpa_supplicant1_Interface_remove_all_networks(..): %s", */
            /* error->message); */
  /* } */

  if (TRUE != fi_w1_wpa_supplicant1_Interface_disconnect(
                  wlan->p2p,
                  &error))
  {
    g_warning("fi_w1_wpa_supplicant1_Interface_disconnect(..): %s",
            error->message);
  }

  return result;
}

int wlan_start_pbc(struct WlanInterface* wlan, const char* bssid)
{
  g_debug("wlan_start_pbc(%p, %s)", wlan, bssid);
  g_assert(NULL != wlan);

  int result = WPAS_RESULT_OK;

  struct WfdPeer* peer = g_hash_table_find(wlan->peers, _find_peer_by_mac, (gpointer)bssid);

  if (NULL == peer)
  {
    g_warning("Uknown BSSID %s.", bssid);
    result = WPAS_RESULT_ERROR;
  }
  else
  {
    g_debug("Found peer: %s", peer_trace(peer));

    GValue role  = G_VALUE_INIT;
    GValue type  = G_VALUE_INIT;
    /* GValue bssid = G_VALUE_INIT; */
    GHashTable* params = g_hash_table_new(g_str_hash, g_str_equal);
    g_assert(NULL != params);

    g_value_init(&role, G_TYPE_STRING);
    g_value_set_static_string(&role, "enrollee");
    g_hash_table_insert(params, "Role", &role);

    g_value_init(&type, G_TYPE_STRING);
    g_value_set_static_string(&type, "pbc");
    g_hash_table_insert(params, "Type", &type);

    /* g_value_init(&bssid, G_TYPE_STRING); */
    /* g_value_set_static_string(&bssid, bssid); */
    /* g_hash_table_insert(params, "BSSID", &bssid); */

    GError *error = NULL;
    fi_w1_wpa_supplicant1_Interface_WPS_start(
            wlan->wps, params, NULL, &error);
    if (NULL != error)
    {
      g_error("fi_w1_wpa_supplicant1_Interface_WPS_start(..) failed: %s",
              error->message);
      result = WPAS_RESULT_ERROR;
    }
  }

  return result;
}

int wlan_reject_peer(struct WlanInterface* wlan, const char* p2p_mac)
{
  g_debug("wlan_reject_connection(%p, %s)", wlan, p2p_mac);
  g_assert(NULL != wlan);

  int result = WPAS_RESULT_OK;

  struct WfdPeer* peer = g_hash_table_find(wlan->peers, _find_peer_by_mac, (gpointer)p2p_mac);

  if (NULL == peer)
  {
    g_warning("Uknown P2P MAC %s.", p2p_mac);
    result = WPAS_RESULT_ERROR;
  }
  else
  {
    g_debug("Found peer: %s", peer_trace(peer));

    GError* error = NULL;
    fi_w1_wpa_supplicant1_Interface_P2PDevice_reject_peer(
        wlan->p2p, ((struct DBusBase*)peer)->opath, &error);
    if (NULL != error)
    {
      g_error("fi_w1_wpa_supplicant1_Interface_P2PDevice_reject_peer(..) failed: %s",
              error->message);
      result = WPAS_RESULT_ERROR;
    }
  }

  return result;
}
