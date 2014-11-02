#pragma once

#include "wpas_dbus.h"

struct WfdGroup;

int group_create(struct WfdGroup**, const char* dbus_path);
int group_release(struct WfdGroup**);
