#pragma once

#include <stdint.h>
#include "wpas_dbus.h"

struct WlanInterface;
struct WpaSupplicant;

// struct peer_device
// {
  // char* device_name;
  // char* mac_address;

  // char*
// };

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
  void (*cb_connection_request)(const char* mac_address, const char* device_name, int display_type);
  void (*cb_display_source_connected)(const char* mac_address, const char* ip_address, const uint16_t port);
  void (*cb_device_disconnected)(const char* mac_address);
};

struct wpas_callbacks
{
  void (*cb_set_wfd_ie)(const void* bytes, int length);
};

int wlan_create(struct WlanInterface**, const char* dbus_path);
int wlan_release(struct WlanInterface**);
int wlan_set_callbacks(struct WlanInterface*, struct WlanCallbacks*);
int wlan_set_display(struct WlanInterface*, struct WlanDisplayInfo*);
int wlan_start_autonomous_group(struct WlanInterface*);
int wlan_stop_autonomous_group(struct WlanInterface*);
int wlan_accept_connection(struct WlanInterface*, const char* mac_address);
int wlan_reject_connection(struct WlanInterface*, const char* mac_address);
