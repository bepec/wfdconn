#include "dbus_base.h"
#include "proxy_wpa_supplicant1.h"

int dbusbase_init(struct DBusBase *base,
                  DBusGConnection *bus,
                  const char      *service,
                  const char      *opath,
                  const char      *iface)
{
  g_debug("dbusbase_init(%p, %p, %s, %s, %s)",
          base, bus, service, opath, iface);
  g_assert(NULL != base);
  g_assert(NULL != bus);
  g_assert(NULL != service);
  g_assert(NULL != opath);
  g_assert(NULL != iface);

  base->opath = g_strdup(opath);
  base->bus = bus;

  g_debug("Getting \'%s\' iface for \'%s\' object.", iface, opath);
  base->proxy = dbus_g_proxy_new_for_name(
      base->bus,
      service,
      opath,
      iface);
  g_return_val_if_fail(NULL != base->proxy, 1);

  g_debug("Getting \'%s\' iface for \'%s\' object.", DBUS_INTERFACE_PROPERTIES, opath);
  base->props = dbus_g_proxy_new_from_proxy(
      base->proxy,
      DBUS_INTERFACE_PROPERTIES,
      opath);
  g_return_val_if_fail(NULL != base->props, 1);

  return 0;
}

void dbusbase_deinit(struct DBusBase* base)
{
  g_debug("dbusbase_deinit(%p)", base);
  g_assert(NULL != base);

  g_free(base->opath);
}

gboolean dbusbase_get_property(struct DBusBase* base, const char* iface, const char* name, GValue* o_value)
{
  g_debug("dbusbase_get_property(%p, \'%s\', \'%s\').",
          base, iface, name);
  g_assert(NULL != base);
  g_assert(NULL != iface);
  g_assert(NULL != name);
  g_assert(NULL != o_value);

  gboolean result = TRUE;
  GError*  error  = NULL;

  if (org_freedesktop_DBus_Properties_get(
              base->props,
              iface,
              name,
              o_value,
              &error) == TRUE)
  {
    g_assert(NULL == error);
    g_debug("Got property \'%s\' of type %s: %s", name, G_VALUE_TYPE_NAME(o_value), g_strdup_value_contents(o_value));
  }
  else
  {
    g_error("org_freedesktop_DBus_Properties_get(..) failed with error: %s", error->message);
    result = FALSE;
  }

  return result;
}

void print_introspection(DBusGProxy *proxy)
{
    g_assert(NULL != proxy);
    const char* path = dbus_g_proxy_get_path(proxy);
    g_debug("Creating introspectable proxy object for %s.", path);

    GError *error = NULL;

    DBusGProxy *proxy_introspectable = dbus_g_proxy_new_from_proxy(
            proxy,
            DBUS_INTERFACE_INTROSPECTABLE,
            path);
    if (NULL == proxy_introspectable)
    {
        g_warning("Failed to create proxy object for %s.", path);
    }
    else
    {
        char *introspection;
        if (FALSE == org_freedesktop_DBus_Introspectable_introspect(proxy_introspectable, &introspection, &error)) {
            g_assert(NULL != error);
            g_warning("Failed to get introspection: %s", error->message);
        }
        else
        {
            g_assert(NULL != introspection);
            g_debug("Introspection for \"%s\":\n%s", path, introspection);
            g_free(introspection);
        }
        g_object_unref(proxy_introspectable);
    }
}
