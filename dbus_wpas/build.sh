echo `pkg-config --libs dbus-1`
gcc -Wall -std=c99 -o gwpa \
    `pkg-config --cflags dbus-glib-1` \
    `pkg-config --libs dbus-glib-1`\
    gwpa.c wfd.c
# gcc `pkg-config --cflags dbus-1` `pkg-config --libs dbus-1` -o dbus-wpa dbus-wpa.c
