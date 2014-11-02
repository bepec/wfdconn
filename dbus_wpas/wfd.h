#pragma once

#include <stdint.h>
#include "wpas_dbus.h"

struct WlanInterface;
struct WpaSupplicant;

#define  WPAS_P2P_GROUP_NONE               0
#define  WPAS_P2P_GROUP_OWNER              1
#define  WPAS_P2P_GROUP_PEER               2

#define  WPAS_WFD_DISPLAY_SOURCE           0
#define  WPAS_WFD_DISPLAY_PRIMARY_SINK     1
#define  WPAS_WFD_DISPLAY_SECONDARY_SINK   2
#define  WPAS_WFD_DISPLAY_BOTH             3

#define  WPAS_WFD_SESSION_NOT_AVAILABLE    0
#define  WPAS_WFD_SESSION_AVAILABLE        1

#define  WPAS_WFD_CONNECTIVITY_P2P         0
#define  WPAS_WFD_CONNECTIVITY_TDLS        1

struct WlanDisplayInfo
{
  uint8_t device_type;
  uint8_t available;
  uint8_t connectivity;

  uint16_t control_port;
  uint16_t bandwidth;
};

struct WlanCallbacks
{
  void (*cb_group_started)(struct WlanInterface*);
  void (*cb_connection_request)(struct WlanInterface*, const char* p2p_mac, const char* device_name, const char* wfd_ie);
  void (*cb_device_connected)(struct WlanInterface*, const char* p2p_mac, const char* sta_mac);
  void (*cb_device_disconnected)(struct WlanInterface*, const char* p2p_mac, const char* sta_mac);
};

struct wpas_callbacks
{
  void (*cb_set_wfd_ie)(const void* bytes, int length);
};

struct DBusBase* wlan_get_dbus_base(struct WlanInterface*);

int wlan_create(struct WlanInterface**, const char* dbus_path);
int wlan_release(struct WlanInterface**);
int wlan_set_callbacks(struct WlanInterface*, struct WlanCallbacks*);
int wlan_set_config_methods(struct WlanInterface*, const char *methods);
int wlan_set_display(struct WlanInterface*, struct WlanDisplayInfo*);
int wlan_get_group_info(struct WlanInterface*, int* o_info);
int wlan_start_autonomous_group(struct WlanInterface*);
int wlan_flush(struct WlanInterface*);
int wlan_start_pbc(struct WlanInterface*, const char* mac_address);
int wlan_reject_connection(struct WlanInterface*, const char* mac_address);
