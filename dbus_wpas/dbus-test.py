#!/usr/bin/python

import dbus
import sys, os
import time
import gobject
from dbus.mainloop.glib import DBusGMainLoop
from dhcp_leases import parse_lease_file

WPAS_DBUS_SERVICE = "fi.w1.wpa_supplicant1"
WPAS_DBUS_INTERFACE = "fi.w1.wpa_supplicant1"
WPAS_DBUS_OPATH = "/fi/w1/wpa_supplicant1"

WPAS_DBUS_INTERFACES_INTERFACE = "fi.w1.wpa_supplicant1.Interface"
WPAS_DBUS_INTERFACES_OPATH = "/fi/w1/wpa_supplicant1/Interfaces"
WPAS_DBUS_BSS_INTERFACE = "fi.w1.wpa_supplicant1.BSS"
WPAS_DBUS_PEER_INTERFACE = "fi.w1.wpa_supplicant1.Peer"
WPAS_DBUS_GROUP_INTERFACE = "fi.w1.wpa_supplicant1.Group"
WPAS_WLAN_PATH = "/fi/w1/wpa_supplicant1/Interfaces/0"
WPAS_DBUS_INTERFACES_P2P = "fi.w1.wpa_supplicant1.Interface.P2PDevice"
WPAS_DBUS_INTERFACES_WPS = "fi.w1.wpa_supplicant1.Interface.WPS"


interfaces = []
p2p_devices = {}
group_obj = None
bus = None
wlan = None
ip = ""
port = 0



class DbusObject:
    global bus
    service = WPAS_DBUS_SERVICE

    def __init__(self, path):
        self.obj = bus.get_object(DbusObject.service, path)
        self.path = path
        self.properties = self.get_if(dbus.PROPERTIES_IFACE)

    def get_if(self, if_name):
        return dbus.Interface(self.obj, if_name)

    def introspect(self):
        return self.obj.Introspect(dbus_interface=dbus.INTROSPECTABLE_IFACE)

    def get_properties(self, if_name):
        return self.obj.GetAll(if_name, dbus_interface=dbus.PROPERTIES_IFACE)

    def print_properties(self, if_name):
        print "PROPERTIES OF {0}:".format(if_name)
        properties = self.get_properties(if_name)
        for key, value in properties.iteritems():
            print "\t", key, ":", value


class Wlan(DbusObject):

    def __init__(self, path):
        DbusObject.__init__(self, path)
        self.iface = self.get_if(WPAS_DBUS_INTERFACES_INTERFACE)
        self.wps = self.get_if(WPAS_DBUS_INTERFACES_WPS)
        self.p2p = self.get_if(WPAS_DBUS_INTERFACES_P2P)


class WpaSupplicant(DbusObject):

    def __init__(self):
        DbusObject.__init__(self, WPAS_DBUS_OPATH)
        if_paths = self.properties.Get(WPAS_DBUS_INTERFACE, 'Interfaces')
        self.interfaces = [Wlan(path) for path in if_paths]

    def get_interfaces(self):
        return self.properties.Get(WPAS_DBUS_INTERFACE, 'Interfaces')

    def print_interfaces(self):
        for iface in self.interfaces:
            print iface.path, ":", iface.properties.Get(WPAS_DBUS_INTERFACES_INTERFACE, "Ifname")

def groupPeerJoined(peer):
    print "PEER JOINED:", peer
    print ""

def groupPeerDisconnected(peer):
    print "PEER DISONNECTED:", peer
    print ""

def group_detected(group_path):
    print "GROUP DETECTED:", group_path
    global bus
    global group_obj
    if not group_obj:
        group_obj = DbusObject(group_path)
        group_obj.introspect()
        bus.add_signal_receiver(groupPeerJoined,
                dbus_interface=WPAS_DBUS_GROUP_INTERFACE,
                signal_name="PeerJoined")
        bus.add_signal_receiver(groupPeerDisconnected,
                dbus_interface=WPAS_DBUS_GROUP_INTERFACE,
                signal_name="PeerDisconnected")
    print ""

def propertiesChanged(*args):
    print "propertiesChanged:", args
    # if properties.has_key("State"):
        # print "PropertiesChanged: State: %s" % (properties["State"])
    print ""


def wlanStaAuthorized(name):
    print "wlanStaAuthorized: name={0}\n".format(name)
    global ip, port
    ip_assigned = False
    print "*** WAITING FOR IP ADDRESS ASSIGNED ***"
    while not ip_assigned:
        leases = parse_lease_file("/var/lib/dhcp/db/dhcpd.leases")
        print "\tFound {0} leases.".format(len(leases))
        for lease in leases:
            if lease["mac"] == name:
                print "\t!!! Found IP address:", lease["ip"]
                ip_assigned = True
                ip = lease["ip"]
                break
    print "\n*** DEVICE IS READY FOR CONNECT {0}:{1} ***\n".format(ip, port)


def wlanStaDeauthorized(name):
    print "wlanStaDeauthorized: name={0}\n".format(name)


def p2pStateChanged(states):
    print "p2pStateChanged:", states
    print ""


def p2pDeviceFound(path):
    print "p2pDeviceFound", path
    dev_obj = DbusObject(path)
    dev_obj.mac = path.split("/")[-1]
    dev_obj.print_properties(WPAS_DBUS_PEER_INTERFACE)
    print "Device {0} registered.".format(dev_obj.mac)
    global p2p_devices
    global port
    p2p_devices[path] = dev_obj
    ies = dev_obj.properties.Get(WPAS_DBUS_PEER_INTERFACE, "IEs")
    ies = [int(ies_byte) for ies_byte in ies]
    if ies[0] == 0 and ies[1] == 0 and ies[2] == 6:
        assert(len(ies) == 9)
        port = (ies[5] << 8) | ies[6]
        bandwidth = (ies[7] << 8) | ies[8]
        print "*** FOUND WFD IE: {0} ***".format(ies)
        print "\tDevice type:", ["SRC", "PSN", "SSN", "DUA"][ies[4] & 0x3] 
        print "\tSource coupling:", ["NOT SUPPORTED", "SUPPORTED"][(ies[4] & 0x4) >> 2] 
        print "\tSink coupling:", ["NOT SUPPORTED", "SUPPORTED"][(ies[4] & 0x8) >> 3] 
        print "\tSession availability:", ["NO", "YES", "10", "11"][(ies[4] & 0x30) >> 4] 
        print "\tControl port:", port
        print "\tBandwidth:", bandwidth



def p2pDeviceLost(path):
    print "p2pDeviceLost", path
    global p2p_devices
    global bus
    if path not in p2p_devices:
        print 'Unknown device'
    else:
        dev_obj = p2p_devices[path]
        print "Device {0} unregistered.".format(dev_obj.mac)


def p2pGroupStarted(properties):
    print "p2pGroupStarted"
    print properties
    group_detected(properties["group_object"])
    print ""


def p2pGroupFinished(properties):
    print "p2pGroupFinished"
    print properties
    print ""


def p2pProvisionDiscoveryPBCRequest(peer_object):
    print "ProvisionDiscoveryPBCRequest"
    print peer_object
    print ""
    global wlan
    wlan.wps.Start({"Role": "enrollee", "Type": "pbc"})


def wpsEvent(name, args):
    print "WPS.Event"
    print properties
    print args
    print ""


def main():
    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)

    global bus
    bus = dbus.SystemBus()

    bus.add_signal_receiver(propertiesChanged,
            dbus_interface=WPAS_DBUS_INTERFACES_INTERFACE,
            signal_name="PropertiesChanged")
    bus.add_signal_receiver(propertiesChanged,
            dbus_interface=WPAS_DBUS_PEER_INTERFACE,
            signal_name="PropertiesChanged")
    bus.add_signal_receiver(propertiesChanged,
            dbus_interface=WPAS_DBUS_INTERFACES_P2P,
            signal_name="PropertiesChanged")

    bus.add_signal_receiver(wlanStaAuthorized,
            dbus_interface=WPAS_DBUS_INTERFACES_INTERFACE,
            signal_name="StaAuthorized")
    bus.add_signal_receiver(wlanStaDeauthorized,
            dbus_interface=WPAS_DBUS_INTERFACES_INTERFACE,
            signal_name="StaDeauthorized")
    bus.add_signal_receiver(p2pStateChanged,
            dbus_interface=WPAS_DBUS_INTERFACES_P2P,
            signal_name="P2PStateChanged")
    bus.add_signal_receiver(p2pDeviceFound,
            dbus_interface=WPAS_DBUS_INTERFACES_P2P,
            signal_name="DeviceFound")
    bus.add_signal_receiver(p2pDeviceLost,
            dbus_interface=WPAS_DBUS_INTERFACES_P2P,
            signal_name="DeviceLost")
    bus.add_signal_receiver(p2pGroupStarted,
            dbus_interface=WPAS_DBUS_INTERFACES_P2P,
            signal_name="GroupStarted")
    bus.add_signal_receiver(p2pGroupFinished,
            dbus_interface=WPAS_DBUS_INTERFACES_P2P,
            signal_name="GroupFinished")
    bus.add_signal_receiver(p2pProvisionDiscoveryPBCRequest,
            dbus_interface=WPAS_DBUS_INTERFACES_P2P,
            signal_name="ProvisionDiscoveryPBCRequest")
    bus.add_signal_receiver(wpsEvent,
            dbus_interface=WPAS_DBUS_INTERFACES_WPS,
            signal_name="Event")


    WFDIE = "000006001100000100"

    wpas = WpaSupplicant()
    wpas.properties.Set(WPAS_DBUS_SERVICE, "WFDIEs", dbus.ByteArray(WFDIE.decode("hex")))
    wpas.print_properties(WPAS_DBUS_INTERFACE)
    print "INTERFACES DETECTED:"
    wpas.print_interfaces()

    global wlan
    wlan = wpas.interfaces[0]
    print "Working with interface '{0}'".format(wlan.path)
    print ""

    wlan.properties.Set(WPAS_DBUS_INTERFACES_WPS, "ConfigMethods", "pbc")
    wlan.print_properties(WPAS_DBUS_INTERFACES_INTERFACE)
    wlan.print_properties(WPAS_DBUS_INTERFACES_WPS)
    wlan.print_properties(WPAS_DBUS_INTERFACES_P2P)
    print ""

    group_path = wlan.properties.Get(WPAS_DBUS_INTERFACES_P2P, "Group")
    print "\nGROUP_PATH: {0}\n".format(group_path)
    if not group_path or group_path == '/':
        print("ADD_GROUP")
        wlan.p2p.Flush(dbus_interface=WPAS_DBUS_INTERFACES_P2P)
        wlan.p2p.GroupAdd({}, dbus_interface=WPAS_DBUS_INTERFACES_P2P)
    else:
        group_detected(group_path)

    print ""
    # print introspect(wlan)
    print ""

    # wlan.Find({}, dbus_interface=WPAS_DBUS_INTERFACES_P2P)

    gobject.MainLoop().run()

    # wlan = wpas_obj._bus.get_object(WPAS_DBUS_SERVICE, path)

main()
