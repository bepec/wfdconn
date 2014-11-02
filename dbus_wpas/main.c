#include <stdlib.h>
#include "wpas_dbus.h"
#include "wfd.h"

DBusGConnection *wpas_dbus_g_connection = NULL;

DBusGConnection *wpas_get_dbus_g_connection()
{
    return wpas_dbus_g_connection;
}

static void log_handler(
        const gchar   *log_domain,
        GLogLevelFlags log_level,
        const gchar   *message,
        gpointer       user_data)
{
  g_print("DEBUG: %s\n", message);
}

void handle_wlan_connection_request(struct WlanInterface* wlan, const char* mac_address, const char* device_name, const char* wfd_ie)
{
  g_message("WLAN connection request: mac=\'%s\', name=\'%s\', ie=\'%s\'",
            mac_address, device_name, wfd_ie);

  /* wlan_start_pbc(wlan, mac_address); */
  wlan_reject_peer(wlan, mac_address);
}

void handle_wlan_device_connected(struct WlanInterface* wlan, const char* p2p_mac, const char* sta_mac)
{
  g_message("Device connected: p2p_mac=\'%s\', sta_mac=\'%s\'",
            p2p_mac, sta_mac);
}

int main(int arc, char* argv[])
{
    /* g_mem_set_vtable(glib_mem_profiler_table); */
    g_log_set_handler(NULL, G_LOG_LEVEL_DEBUG, log_handler, NULL);

    GMainLoop  *main_loop = NULL;
    GError     *error = NULL;

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

    struct WlanInterface *wlan = NULL;
    g_assert(WPAS_RESULT_OK == wpas_get_interface(wpas, &wlan));
    g_assert(NULL != wlan);

    struct WlanCallbacks wlan_cb = {0,};
    wlan_cb.cb_connection_request = handle_wlan_connection_request;
    wlan_cb.cb_device_connected = handle_wlan_device_connected;
    wlan_set_callbacks(wlan, &wlan_cb);

    g_message("Setting config methods.");
    g_assert(WPAS_RESULT_OK == wlan_set_config_methods(wlan, "pbc"));

    g_message("Setting display info.");
    struct WlanDisplayInfo display_info =
        { .device_type  = WPAS_WFD_DISPLAY_PRIMARY_SINK
        , .available    = WPAS_WFD_SESSION_AVAILABLE
        , .connectivity = WPAS_WFD_CONNECTIVITY_P2P
        , .control_port = 0
        , .bandwidth    = 5000
        };
    g_assert(WPAS_RESULT_OK == wlan_set_display(wlan, &display_info));

    int group_info = 0;
    g_assert(WPAS_RESULT_OK == wlan_get_group_info(wlan, &group_info));

    if (group_info != WPAS_P2P_GROUP_NONE)
    {
      g_message("Waiting for group state reset...");
      while (group_info != WPAS_P2P_GROUP_NONE)
      {
        g_message("Reset P2P interface state.");
        g_assert(WPAS_RESULT_OK == wlan_flush(wlan));

        g_usleep(1000000);
        g_assert(WPAS_RESULT_OK == wlan_get_group_info(wlan, &group_info));
      }
    }

    g_assert(WPAS_P2P_GROUP_NONE == group_info);
 
    if (group_info == WPAS_P2P_GROUP_NONE)
    {
      g_message("Starting autonomous P2P group...");
      g_assert(WPAS_RESULT_OK == wlan_start_autonomous_group(wlan));

      g_message("Waiting for group state update...");
      while (group_info != WPAS_P2P_GROUP_OWNER)
      {
        g_usleep(1000000);
        g_assert(WPAS_RESULT_OK == wlan_get_group_info(wlan, &group_info));
      }
    }

    g_assert(WPAS_P2P_GROUP_OWNER == group_info);
    g_message("Is a group owner now.");

    g_message("Starting main loop.");
    g_main_loop_run(main_loop);

    wlan_release(&wlan);
    wpas_release(&wpas);

    g_main_loop_unref(main_loop);
    
    /* g_mem_profile(); */

    return EXIT_FAILURE;
}
