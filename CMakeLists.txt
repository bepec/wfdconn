cmake_minimum_required(VERSION 2.8)

project(WfdConnectionManager)

include_directories(.)
include_directories(wpa_ctrl)

add_definitions(-std=c++11)
add_definitions(-fno-rtti)
add_definitions(-D_GLIBCXX_USE_NANOSLEEP)
add_definitions(-Wall)
add_definitions(-Wextra)
add_definitions(-Wno-format)
add_definitions(-Wno-unused-parameter)
add_definitions(-g)
add_definitions(-DCONFIG_CTRL_IFACE)
add_definitions(-DCONFIG_CTRL_IFACE_UNIX)

set(WPA_CTRL_LIB wpa_ctrl)
set(WPA_CTRL_SOURCES
	wpa_ctrl/wpa_ctrl.c
	wpa_ctrl/os_unix.c)
add_library(${WPA_CTRL_LIB} STATIC ${WPA_CTRL_SOURCES})

set(WFD_CONNMNGR_LIB wfd_connmngr)
set(WFD_CONNMNGR_SOURCES
	wfd/wpa_types.cc
	wfd/wfd_connection_manager.cc)
add_library(${WFD_CONNMNGR_LIB} STATIC ${WFD_CONNMNGR_SOURCES})

add_executable(wfd_connect main.cc) 
target_link_libraries(${WFD_CONNMNGR_LIB} ${WPA_CTRL_LIB})
target_link_libraries(wfd_connect ${WFD_CONNMNGR_LIB})
