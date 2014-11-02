#include <string.h>
#include <stdio.h>
#include "wfd_peer.h"
#include "dbus_base.h"
#include "proxy_wpa_supplicant1.h"

#define WPAS_DBUS_PEER_IFACE "fi.w1.wpa_supplicant1.Peer"

struct WfdPeer
{
  const char      *opath;
  DBusGConnection *bus;
  DBusGProxy      *proxy;
  DBusGProxy      *props;

  char  mac[24];
  char *device_name;
  char *wfd_ie;
};

gboolean _get_mac_from_opath(const char* opath, char* mac)
{
  g_debug("g_value_dup_string(%s)", opath);
  g_assert(NULL != opath);
  g_assert(NULL != mac);

  const char* start = strrchr(opath, '/');
  g_return_val_if_fail(NULL != start, FALSE);

  start += 1;
  g_return_val_if_fail(strlen(start) == 12, FALSE);

  char* pmac = mac;
  for (int i = 0; i < strlen(start); i++)
  {
    if (i != 0 && i % 2 == 0)
    {
      *pmac = ':';
      pmac++;
    }
    *pmac = start[i];
    pmac++;
  }

  g_debug("_get_mac_from_opath(): mac=%s", mac);
  return TRUE;
}

int _peer_init(struct WfdPeer *peer, const char* opath)
{
  g_debug("_peer_init(%p, %s)", peer, opath);
  g_assert(NULL != wpas_get_dbus_g_connection());

  int result;

  result = dbusbase_init(
      (struct DBusBase*)peer,
      wpas_get_dbus_g_connection(),
      WPAS_DBUS_SERVICE_NAME,
      opath,
      WPAS_DBUS_PEER_IFACE);
  g_return_val_if_fail(WPAS_RESULT_OK == result, result);

  g_return_val_if_fail(
      TRUE == _get_mac_from_opath(opath, peer->mac),
      WPAS_RESULT_ERROR);
  g_message("Peer MAC: %s", peer->mac);

  GValue  device_name = {0};
  g_assert(TRUE == DBUS_GET_PROPERTY(peer,
          WPAS_DBUS_PEER_IFACE,
          "DeviceName",
          &device_name));
  peer->device_name = g_value_dup_string(&device_name);

  GValue  ies = {0};
  g_assert(TRUE == DBUS_GET_PROPERTY(peer,
          WPAS_DBUS_PEER_IFACE,
          "IEs",
          &ies));
  g_assert(TRUE == G_VALUE_HOLDS_BOXED(&ies));
  GArray *wfdies = g_value_get_boxed(&ies);
  g_debug("Len: %d", wfdies->len);

  return WPAS_RESULT_OK;
}

int _peer_deinit(struct WfdPeer *wfd)
{
  g_debug("_peer_deinit(%p)", wfd);
  g_assert(NULL != wfd);

  dbusbase_deinit((struct DBusBase*)wfd);

  return WPAS_RESULT_OK;
}

int peer_create(struct WfdPeer** o_peer, const char* opath)
{
  g_debug("peer_create(%p %p, %s)", o_peer, *o_peer, opath);
  g_assert(NULL != o_peer);
  g_assert(NULL == *o_peer);
  g_assert(NULL != opath);

  *o_peer = g_new0(struct WfdPeer, 1);
  g_return_val_if_fail(NULL != *o_peer, WPAS_RESULT_ERROR);

  int result = WPAS_RESULT_OK;

  if (WPAS_RESULT_OK != (result = _peer_init(*o_peer, opath)))
  {
    g_free(*o_peer);
    *o_peer = NULL;
  }

  return result;
}

int peer_release(struct WfdPeer** wfd)
{
  g_debug("peer_release(%p)", wfd);
  g_assert(NULL != wfd);
  g_assert(NULL != *wfd);

  _peer_deinit(*wfd);
  g_free(*wfd);
  *wfd = NULL;

  return WPAS_RESULT_OK;
}

char* peer_trace(struct WfdPeer* peer)
{
  static char tracestr[1024];

  sprintf(tracestr, "WfdPeer{opath=\'%s\', mac=\'%s\', device_name=\'%s\', wfd_ie=\'%s\'}",
          peer->opath, peer->mac, peer->device_name, peer->wfd_ie);

  return tracestr;
}

const char* peer_get_mac(struct WfdPeer* peer)
{
  g_assert(NULL != peer);
  return peer->mac;
}

const char* peer_get_name(struct WfdPeer* peer)
{
  g_assert(NULL != peer);
  return peer->device_name;
}

const char* peer_get_wfd_ie(struct WfdPeer* peer)
{
  g_assert(NULL != peer);
  return peer->wfd_ie;
}

