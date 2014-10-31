#include <stdlib.h>
#include <glib.h>
#include <dbus/dbus-glib.h>
#include "wpas_dbus.h"
#include "wpa_supplicant1_Interface.h"
#include "wfd.h"


DBusGConnection *wpas_dbus_g_connection = NULL;

DBusGConnection *wpas_get_dbus_g_connection()
{
    return wpas_dbus_g_connection;
}

struct WpaSupplicant
{
    DBusGConnection *bus;
    DBusGProxy      *proxy;
    DBusGProxy      *props;

    GPtrArray       *ifaces;
};

void _iterate_wpas_interfaces(const GValue *value, gpointer data)
{
    g_debug("_iterate_wpas_interfaces(value=%s, data=%p)", (char*)g_value_get_boxed(value), data);

    g_assert(NULL != value);
    g_assert(NULL != data);

    /* g_message("Value type: %s", G_VALUE_TYPE_NAME(value)); */
    /* g_message("Value string: %s", g_value_get_boxed(value)); */

    struct WpaSupplicant* wpas = data;
    g_ptr_array_add(wpas->ifaces, g_value_get_boxed(value));
}

int _wpas_init(struct WpaSupplicant* wpas)
{
    g_debug("_wpas_init(%p)", wpas);
    g_assert(NULL != wpas);

    wpas->ifaces = g_ptr_array_new();
    g_assert(NULL != wpas->ifaces);

    wpas->bus = wpas_get_dbus_g_connection();
    g_assert(NULL != wpas->bus);

    g_message("Creating proxy object for " WPAS_DBUS_SERVICE_NAME ".");
    wpas->proxy = dbus_g_proxy_new_for_name(
            wpas->bus,
            WPAS_DBUS_SERVICE_NAME,
            WPAS_DBUS_SERVICE_OPATH,
            WPAS_DBUS_SERVICE_IFACE);
    g_return_val_if_fail(NULL != wpas->proxy, WPAS_RESULT_ERROR);

    g_message("Creating properties proxy for " WPAS_DBUS_SERVICE_NAME ".");
    wpas->props = dbus_g_proxy_new_from_proxy(
            wpas->proxy,
            DBUS_INTERFACE_PROPERTIES,
            dbus_g_proxy_get_path(wpas->proxy));
    g_return_val_if_fail(NULL != wpas->props, WPAS_RESULT_ERROR);

    return WPAS_RESULT_OK;
}

int _wpas_deinit(struct WpaSupplicant* wpas)
{
    g_debug("_wpas_deinit(%p)", wpas);

    g_ptr_array_free(wpas->ifaces, TRUE);

    return WPAS_RESULT_OK;
}

int _wpas_update_ifaces(struct WpaSupplicant* wpas)
{
    g_debug("_wpas_update_ifaces(%p)", wpas);
    g_assert(NULL != wpas);

    GError *error = NULL;
    GValue value = { 0 };

    g_message("Getting Interfaces property.");
    org_freedesktop_DBus_Properties_get(wpas->props, WPAS_DBUS_SERVICE_IFACE, "Interfaces", &value, &error);
    if (NULL != error)
    {
        g_print(error->message);
    }
    g_return_val_if_fail(NULL == error, WPAS_RESULT_ERROR);
    
    char *value_content = g_strdup_value_contents(&value);
    g_message("Value: %s", value_content);
    g_free(value_content);

    g_assert(dbus_g_type_is_collection(G_VALUE_TYPE(&value)));
    dbus_g_type_collection_value_iterate(&value, _iterate_wpas_interfaces, wpas);
    g_message("Interfaces found: %d", wpas->ifaces->len);

    return WPAS_RESULT_OK;
}

int wpas_create(struct WpaSupplicant** o_wpas)
{
    g_debug("wpas_create(%p)", o_wpas);

    g_assert(NULL != o_wpas);
    g_assert(NULL == *o_wpas);

    *o_wpas = g_new0(struct WpaSupplicant, 1);

    g_return_val_if_fail(NULL != *o_wpas, WPAS_RESULT_ERROR);

    int result = WPAS_RESULT_OK;
    
    if (WPAS_RESULT_OK != (result = _wpas_init(*o_wpas)))
    {
        g_free(*o_wpas);
        *o_wpas = NULL;
    }

    return result;
}

int wpas_release(struct WpaSupplicant** wpas)
{
    g_debug("wpas_release(%p)", wpas);
    g_assert(wpas != NULL);

    _wpas_deinit(*wpas);
    g_free(*wpas);
    *wpas = NULL;

    return WPAS_RESULT_OK;
}

int wpas_get_interface(struct WpaSupplicant *wpas, struct WlanInterface **o_wlan)
{
    g_debug("wpas_get_interface(%p, %p)", wpas, o_wlan);
    g_assert(NULL != wpas);
    g_assert(NULL != o_wlan);
    g_assert(NULL == *o_wlan);

    int result = WPAS_RESULT_OK;

    if (wpas->ifaces->len == 0)
    {
        result = _wpas_update_ifaces(wpas);
    }

    if (WPAS_RESULT_OK == result && wpas->ifaces->len > 0)
    {
        result = wlan_create(o_wlan, g_ptr_array_index(wpas->ifaces, 0));
        g_warn_if_fail(WPAS_RESULT_OK == result);
    }

    return result;
}

int wpas_set_wfd_ie(struct WpaSupplicant* wpas, void* ie, size_t ie_size)
{
    g_message("wpas_set_wfd_ie(%p, %p, %zu)", wpas, ie, ie_size);
    g_assert(NULL != wpas);
    
    int result = WPAS_RESULT_OK;

    GError *error = NULL;
    /* GArray  array = { ie, ie_size }; */
    GArray *array = g_array_sized_new(FALSE, FALSE, sizeof(guchar), ie_size);
    g_array_append_vals(array, ie, ie_size);

    GValue  value = { 0 };
    /* g_value_set_boxed(&value, array); */

    g_assert(TRUE == org_freedesktop_DBus_Properties_set(
            wpas->props,
            WPAS_DBUS_SERVICE_IFACE,
            "WFDIEs",
            &value,
            &error));

    g_message("TEST");
    
    g_assert(NULL == error);

    return result;
}

void print_introspection(DBusGProxy *proxy)
{
    g_assert(NULL != proxy);
    const char* path = dbus_g_proxy_get_path(proxy);
    g_message("Creating introspectable proxy object for %s.", path);

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
            g_message("Introspection for \"%s\":\n%s", path, introspection);
            g_free(introspection);
        }
        g_object_unref(proxy_introspectable);
    }
}

int main(int arc, char* argv[])
{
    /* g_mem_set_vtable(glib_mem_profiler_table); */

    GMainLoop       *main_loop = NULL;
    GError          *error = NULL;

    g_type_init();
    /* g_log_set_always_fatal(); */

    g_message("Creating the GMainLoop.");
    main_loop = g_main_loop_new(NULL, FALSE);
    g_assert(NULL != main_loop);

    g_message("Connecting to the System bus.");
    wpas_dbus_g_connection = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
    if (error != NULL) g_error(error->message);

    struct WpaSupplicant *wpas = NULL;
    g_assert(WPAS_RESULT_OK == wpas_create(&wpas));
    g_assert(NULL != wpas);

    print_introspection(wpas->proxy);

    GValue value = {0};
    org_freedesktop_DBus_Properties_get(
            wpas->props,
            WPAS_DBUS_SERVICE_IFACE,
            "WFDIEs",
            &value,
            &error);
    g_message("Got property"); 
    char *value_content = g_strdup_value_contents(&value);
    g_message("Value: %s", value_content);
    g_free(value_content);
    g_message("Type: %s", G_VALUE_TYPE_NAME(&value));
    g_assert(TRUE == G_VALUE_HOLDS_BOXED(&value));
    GArray *wfdies = g_value_get_boxed(&value);
    g_message("Len: %d", wfdies->len);

    
    struct WlanInterface *wlan = NULL;
    g_assert(WPAS_RESULT_OK == wpas_get_interface(wpas, &wlan));
    g_assert(NULL != wlan);

    struct WlanDisplayInfo display_info =
        { .device_type  = WPAS_WFD_DISPLAY_PRIMARY_SINK
        , .available    = WPAS_WFD_SESSION_AVAILABLE
        , .connectivity = WPAS_WFD_CONNECTIVITY_P2P
        , .control_port = 0
        , .bandwidth    = 50
        };
    g_assert(WPAS_RESULT_OK == wlan_set_display(wlan, &display_info));
    
    /* [> g_message("Starting main loop."); <] */
    /* [> g_main_loop_run(main_loop); <] */

    wlan_release(&wlan);
    wpas_release(&wpas);

    g_main_loop_unref(main_loop);
    
    /* g_mem_profile(); */

    return EXIT_FAILURE;
}
