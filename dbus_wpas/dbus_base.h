#pragma once

#include <dbus/dbus-glib.h>

struct DBusBase
{
  char            *opath;
  DBusGConnection *bus;
  DBusGProxy      *proxy;
  DBusGProxy      *props;
};

int  dbusbase_init(struct DBusBase*, DBusGConnection*, const char*, const char*, const char*);
void dbusbase_deinit(struct DBusBase*);
gboolean dbusbase_get_property(struct DBusBase*, const char* iface, const char* name, GValue* o_value);

#define DBUS_GET_PROPERTY(proxy, iface, pname, o_val) \
  dbusbase_get_property((struct DBusBase*)proxy, iface, pname, o_val)

#define DBUS_REGISTER_SIGNAL_1(proxy, sname, type1, cb, udata) \
  dbus_g_proxy_add_signal(proxy, sname, type1, G_TYPE_INVALID);\
  dbus_g_proxy_connect_signal(proxy, sname, G_CALLBACK(cb), udata, NULL);

void print_introspection(DBusGProxy *proxy);

// class DBusBase
// {
// public:
  // const char            *opath;
  // const DBusGConnection *bus;
  // DBusGProxy      *proxy;
  // DBusGProxy      *props;

  // DBusBase(const DBusConnection *bus, const char *opath)
    // : opath(opath)
    // , bus(bus)
  // {
  // }
// };
