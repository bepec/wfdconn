#include <iostream>
#include "wfd/wfd_connection_manager.h"


const char* WpaCtrl::kWpaSupplicantPath  = "/var/run/wpa_supplicant/wlan0";
const char* WpaCtrl::kWpaSupplicantIface = "wlan0";

class SimpleConnector: public WfdConnectionManager::ReceiverInterface {
 public:
  std::string macAddress;
  SimpleConnector(const std::string& mac): macAddress(mac) {}

  void OnConnectionRequestPbc(const WfdDevice& device, bool& accept) {
    if (device.macAddress != macAddress) {
      std::string reply;
      std::cout << "[CONN] Unknown MAC address." << std::endl;
      do {
        std::cout << "[CONN] Accept connection? [y/n] (y): ";
        getline(std::cin, reply);
      }
      while (!reply.empty() && reply != "y" && reply != "n");
      accept = (reply == "y" || reply.empty());
    }
    else {
      accept = true;
    }
  }

  void OnConnectionRequestPin(const WfdDevice& device, bool& accept, std::string& pin) {
    std::cout << "[CONN] Please enter 8-digit PIN or skip to reject connection: ";
    getline(std::cin, pin);
    accept = (pin != "");
  }

  void OnConnect(const WfdDevice& device) {
    std::cout << "[CONN] OnConnect()" << std::endl;
  }

  void OnDisconnect(const WfdDevice& device) {
    std::cout << "[CONN] OnDisconnect()" << std::endl;
  }
};

int main(int argc, char* argv[]) {
  std::string mac = argc > 1 ? argv[1] : "ce:3a:61:4c:c3:04";

  SimpleConnector connector(mac);
  WfdConnectionManager wfd(connector);

  wfd.EnableWifiDisplay();
  wfd.WaitForConnection();
  wfd.WaitForDisconnection();
}
