#pragma once

#include "wpas_dbus.h"

struct WfdPeer;

int peer_create(struct WfdPeer**, const char* dbus_path);
int peer_release(struct WfdPeer**);
char* peer_trace(struct WfdPeer*);
const char* peer_get_mac(struct WfdPeer*);
const char* peer_get_name(struct WfdPeer*);
const char* peer_get_wfd_ie(struct WfdPeer*);
