add_definitions(-DCONFIG_CTRL_IFACE)
add_definitions(-DCONFIG_CTRL_IFACE_UNIX)

set(WPA_CTRL_SOURCES wpa_ctrl.c os_unix.c)
add_library(${WPA_CTRL_LIB} STATIC ${WPA_CTRL_SOURCES})
