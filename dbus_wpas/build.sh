echo `pkg-config --libs dbus-1`
gcc -Wall -std=c99 -o gwpa -g \
    `pkg-config --cflags dbus-glib-1` \
    `pkg-config --libs dbus-glib-1`\
    dbus_base.c wpas_dbus.c main.c wfd.c wfd_peer.c
# gcc `pkg-config --cflags dbus-1` `pkg-config --libs dbus-1` -o dbus-wpa dbus-wpa.c
