#include <glib.h>
#include <dbus/dbus-glib.h>
#include "wpas_dbus.h"
#include "proxy_wpa_supplicant1.h"
#include "wfd.h"

struct WpaSupplicant
{
    const char      *opath;
    DBusGConnection *bus;
    DBusGProxy      *proxy;
    DBusGProxy      *props;

    GPtrArray       *ifaces;
};

struct DBusBase* wpas_get_dbus_base(struct WpaSupplicant *wpas)
{
    return (struct DBusBase*)wpas;
}

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

    g_debug("Getting Interfaces property.");
    org_freedesktop_DBus_Properties_get(wpas->props, WPAS_DBUS_SERVICE_IFACE, "Interfaces", &value, &error);
    if (NULL != error)
    {
        g_print(error->message);
    }
    g_return_val_if_fail(NULL == error, WPAS_RESULT_ERROR);
    
    char *value_content = g_strdup_value_contents(&value);
    g_debug("Value: %s", value_content);
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
    g_debug("wpas_set_wfd_ie(%p, %p, %zu)", wpas, ie, ie_size);
    g_assert(NULL != wpas);
    
    int result = WPAS_RESULT_OK;

    GError *error = NULL;
    GByteArray *array = g_byte_array_new_take(ie, ie_size);

    GValue  value = {0,};
    g_value_init(&value, dbus_g_type_get_collection("GArray", G_TYPE_UCHAR));
    g_value_take_boxed(&value, array);
    g_assert(TRUE == G_VALUE_HOLDS_BOXED(&value));

    g_assert(TRUE == org_freedesktop_DBus_Properties_set(
            wpas->props,
            WPAS_DBUS_SERVICE_IFACE,
            "WFDIEs",
            &value,
            &error));

    g_assert(NULL == error);

    g_byte_array_free(array, FALSE);

    return result;
}
