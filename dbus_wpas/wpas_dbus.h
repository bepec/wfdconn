#pragma once

#include <dbus/dbus-glib.h>

#define WPAS_RESULT_OK     0
#define WPAS_RESULT_ERROR  1

#define WPAS_DBUS_SERVICE_NAME   "fi.w1.wpa_supplicant1"
#define WPAS_DBUS_SERVICE_OPATH  "/fi/w1/wpa_supplicant1"
#define WPAS_DBUS_SERVICE_IFACE  "fi.w1.wpa_supplicant1"

struct WpaSupplicant;
struct WlanInterface;

DBusGConnection* wpas_get_dbus_g_connection();

int wpas_create(struct WpaSupplicant**);
int wpas_release(struct WpaSupplicant**);
int wpas_get_interface(struct WpaSupplicant*, struct WlanInterface**);
int wpas_set_wfd_ie(struct WpaSupplicant*, void*, size_t);
