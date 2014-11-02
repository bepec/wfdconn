/* Generated by dbus-binding-tool; do not edit! */

#include <glib.h>
#include <dbus/dbus-glib.h>

G_BEGIN_DECLS

#ifndef _DBUS_GLIB_ASYNC_DATA_FREE
#define _DBUS_GLIB_ASYNC_DATA_FREE
static
#ifdef G_HAVE_INLINE
inline
#endif
void
_dbus_glib_async_data_free (gpointer stuff)
{
	g_slice_free (DBusGAsyncData, stuff);
}
#endif

#ifndef DBUS_GLIB_CLIENT_WRAPPERS_org_freedesktop_DBus_Introspectable
#define DBUS_GLIB_CLIENT_WRAPPERS_org_freedesktop_DBus_Introspectable

static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_freedesktop_DBus_Introspectable_introspect (DBusGProxy *proxy, char ** OUT_data, GError **error)

{
  return dbus_g_proxy_call (proxy, "Introspect", error, G_TYPE_INVALID, G_TYPE_STRING, OUT_data, G_TYPE_INVALID);
}

typedef void (*org_freedesktop_DBus_Introspectable_introspect_reply) (DBusGProxy *proxy, char * OUT_data, GError *error, gpointer userdata);

static void
org_freedesktop_DBus_Introspectable_introspect_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  char * OUT_data;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_STRING, &OUT_data, G_TYPE_INVALID);
  (*(org_freedesktop_DBus_Introspectable_introspect_reply)data->cb) (proxy, OUT_data, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_freedesktop_DBus_Introspectable_introspect_async (DBusGProxy *proxy, org_freedesktop_DBus_Introspectable_introspect_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_slice_new (DBusGAsyncData);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "Introspect", org_freedesktop_DBus_Introspectable_introspect_async_callback, stuff, _dbus_glib_async_data_free, G_TYPE_INVALID);
}
#endif /* defined DBUS_GLIB_CLIENT_WRAPPERS_org_freedesktop_DBus_Introspectable */

#ifndef DBUS_GLIB_CLIENT_WRAPPERS_org_freedesktop_DBus_Properties
#define DBUS_GLIB_CLIENT_WRAPPERS_org_freedesktop_DBus_Properties

static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_freedesktop_DBus_Properties_get (DBusGProxy *proxy, const char * IN_interface, const char * IN_propname, GValue* OUT_value, GError **error)

{
  return dbus_g_proxy_call (proxy, "Get", error, G_TYPE_STRING, IN_interface, G_TYPE_STRING, IN_propname, G_TYPE_INVALID, G_TYPE_VALUE, OUT_value, G_TYPE_INVALID);
}

typedef void (*org_freedesktop_DBus_Properties_get_reply) (DBusGProxy *proxy, GValue OUT_value, GError *error, gpointer userdata);

static void
org_freedesktop_DBus_Properties_get_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  GValue OUT_value = { 0, };
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_VALUE, &OUT_value, G_TYPE_INVALID);
  (*(org_freedesktop_DBus_Properties_get_reply)data->cb) (proxy, OUT_value, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_freedesktop_DBus_Properties_get_async (DBusGProxy *proxy, const char * IN_interface, const char * IN_propname, org_freedesktop_DBus_Properties_get_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_slice_new (DBusGAsyncData);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "Get", org_freedesktop_DBus_Properties_get_async_callback, stuff, _dbus_glib_async_data_free, G_TYPE_STRING, IN_interface, G_TYPE_STRING, IN_propname, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_freedesktop_DBus_Properties_get_all (DBusGProxy *proxy, const char * IN_interface, GHashTable** OUT_props, GError **error)

{
  return dbus_g_proxy_call (proxy, "GetAll", error, G_TYPE_STRING, IN_interface, G_TYPE_INVALID, dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_VALUE), OUT_props, G_TYPE_INVALID);
}

typedef void (*org_freedesktop_DBus_Properties_get_all_reply) (DBusGProxy *proxy, GHashTable *OUT_props, GError *error, gpointer userdata);

static void
org_freedesktop_DBus_Properties_get_all_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  GHashTable* OUT_props;
  dbus_g_proxy_end_call (proxy, call, &error, dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_VALUE), &OUT_props, G_TYPE_INVALID);
  (*(org_freedesktop_DBus_Properties_get_all_reply)data->cb) (proxy, OUT_props, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_freedesktop_DBus_Properties_get_all_async (DBusGProxy *proxy, const char * IN_interface, org_freedesktop_DBus_Properties_get_all_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_slice_new (DBusGAsyncData);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "GetAll", org_freedesktop_DBus_Properties_get_all_async_callback, stuff, _dbus_glib_async_data_free, G_TYPE_STRING, IN_interface, G_TYPE_INVALID);
}
static
#ifdef G_HAVE_INLINE
inline
#endif
gboolean
org_freedesktop_DBus_Properties_set (DBusGProxy *proxy, const char * IN_interface, const char * IN_propname, const GValue* IN_value, GError **error)

{
  return dbus_g_proxy_call (proxy, "Set", error, G_TYPE_STRING, IN_interface, G_TYPE_STRING, IN_propname, G_TYPE_VALUE, IN_value, G_TYPE_INVALID, G_TYPE_INVALID);
}

typedef void (*org_freedesktop_DBus_Properties_set_reply) (DBusGProxy *proxy, GError *error, gpointer userdata);

static void
org_freedesktop_DBus_Properties_set_async_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
  DBusGAsyncData *data = (DBusGAsyncData*) user_data;
  GError *error = NULL;
  dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_INVALID);
  (*(org_freedesktop_DBus_Properties_set_reply)data->cb) (proxy, error, data->userdata);
  return;
}

static
#ifdef G_HAVE_INLINE
inline
#endif
DBusGProxyCall*
org_freedesktop_DBus_Properties_set_async (DBusGProxy *proxy, const char * IN_interface, const char * IN_propname, const GValue* IN_value, org_freedesktop_DBus_Properties_set_reply callback, gpointer userdata)

{
  DBusGAsyncData *stuff;
  stuff = g_slice_new (DBusGAsyncData);
  stuff->cb = G_CALLBACK (callback);
  stuff->userdata = userdata;
  return dbus_g_proxy_begin_call (proxy, "Set", org_freedesktop_DBus_Properties_set_async_callback, stuff, _dbus_glib_async_data_free, G_TYPE_STRING, IN_interface, G_TYPE_STRING, IN_propname, G_TYPE_VALUE, IN_value, G_TYPE_INVALID);
}
#endif /* defined DBUS_GLIB_CLIENT_WRAPPERS_org_freedesktop_DBus_Properties */

#ifndef DBUS_GLIB_CLIENT_WRAPPERS_fi_w1_wpa_supplicant1_Peer
#define DBUS_GLIB_CLIENT_WRAPPERS_fi_w1_wpa_supplicant1_Peer

#endif /* defined DBUS_GLIB_CLIENT_WRAPPERS_fi_w1_wpa_supplicant1_Peer */

G_END_DECLS
