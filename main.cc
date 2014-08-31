#include <iostream>
#include "wfd/wfd_connection_manager.h"


const char* WpaCtrl::kWpaSupplicantPath  = "/var/run/wpa_supplicant/wlan0";
const char* WpaCtrl::kWpaSupplicantIface = "wlan0";

class SimpleConnector: public WfdConnectionManager::ReceiverInterface {
 public:
  std::string macAddress;
  SimpleConnector(const std::string& mac): macAddress(mac) {}

  void OnConnectionRequest(const WfdDevice& device, bool& accept) {
    if (device.macAddress != macAddress) {
      char reply;
      std::cout << "[CONN] Unknown MAC address." << std::endl;
      while (reply != 'y' && reply != 'n') {
        std::cout << "[CONN] Accept connection? [y/n] (y): ";
        std::cin >> reply;
      }
      accept = (reply == 'y');
    }
    else {
      accept = true;
    }
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
