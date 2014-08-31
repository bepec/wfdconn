#include "wpa_types.h"
#include <sstream>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include "wpa_ctrl.h"

// class Event implementation

void Event::parse(const std::string& event) {
  keyParameters.clear();
  listParameters.clear();

  boost::tokenizer<boost::escaped_list_separator<char> > parameter_tokenizer(event,
      boost::escaped_list_separator<char>('\\', ' ', '\"'));
  std::vector<std::string> split_vector;
  bool first = true;

  for (auto parameter : parameter_tokenizer) {
    if (first) {
      name = parameter.substr(3);
      first = false;
    }
    else {
      boost::split(split_vector, parameter, boost::is_any_of("="));
      if (split_vector.empty()) {
        throw std::runtime_error("Empy event parameter.");
      }
      else if (split_vector.size() == 1) {
        listParameters.push_back(split_vector[0]);
      }
      else {
        keyParameters[split_vector[0]] = split_vector[1];
      }
    }
  }
}

std::string Event::to_string() {
  std::ostringstream oss;
  oss << "[EVENT] " << name << " [";
  bool first = true;
  for (const std::string& parameter : listParameters) {
    if (first) first = false;
    else oss << ", ";
    oss << parameter;
  }
  oss << "] {";
  first = true;
  for (auto parameter : keyParameters) {
    if (first) first = false;
    else oss << ", ";
    oss << parameter.first << "=" << parameter.second;
  }
  oss << "}";
  return oss.str();
}

// class WfdDevice implementation

WfdDevice::WfdDevice(const Event& event)
: isWifiDisplay(false) {
  if (event.name != "P2P-DEVICE-FOUND") {
    throw std::runtime_error("Can create device only form P2P-DEVICE-FOUND event");
  }

  macAddress = event.listParameters[0];
  properties = event.keyParameters;
  if (properties.count("wfd_dev_info") > 0) {
    parseWfdParameter(properties["wfd_dev_info"]);
    isWifiDisplay = true;
  }
}

std::string WfdDevice::to_string() {
  std::ostringstream oss;
  oss << "[DEVICE] ";
  oss << macAddress;
  oss << " {";
  bool first = true;
  for (auto property : properties) {
    if (first) first = false;
    else oss << ", ";
    oss << property.first << "=" << property.second;
  }
  oss << "}";
  return oss.str();
}

void WfdDevice::parseWfdParameter(std::string parameter) {
  unsigned long long data = std::stoull(parameter, 0, 16);
  std::cout << "CONV: " << parameter << " to " << data << std::endl;
  // 00101c440028
  properties["wfd_device_type"] = std::to_string((data >> 4*8) & 0x3);
  properties["wfd_port_number"] = std::to_string((data >> 2*8) & 0xffff);
  properties["wfd_bandwidth"] = std::to_string(data & 0xffff);
}

// class WpaCtrl implementation

WpaCtrl::WpaCtrl(): ctrl(0), mon(0) {
  std::cout << "[WPA] Open wpa_supplicant controller: " << kWpaSupplicantPath << std::endl;
  ctrl = wpa_ctrl_open(kWpaSupplicantPath);
  mon = wpa_ctrl_open(kWpaSupplicantPath);
  assert(ctrl != 0);
  assert(mon != 0);
  std::cout << "[WPA] Attach WPA monitor." << std::endl;
  assert(wpa_ctrl_attach(mon) == 0);
}

WpaCtrl::~WpaCtrl() {
  std::cout << "[WPA] Detach WPA monitor." << std::endl;
  assert(wpa_ctrl_detach(mon) == 0);
  std::cout << "[WPA] Close wpa_supplicant controller." << std::endl;
  wpa_ctrl_close(ctrl);
}

std::string WpaCtrl::request(const std::string & query) {
  size_t read = sizeof(reply);
  std::cout << "[WPA] write \"" << query.c_str() << "\" [" << query.length() << " bytes]." << std::endl;
  assert(0 == wpa_ctrl_request(ctrl, query.c_str(), query.length(), reply, &read, 0));
  // NOTE: in all observable cases response ends with \n
  read--;
  reply[read] = 0;
  std::cout << "[WPA] Read \"" << reply << "\" [" << read << " bytes]." << std::endl;
  return std::string(reply, read);
}

bool WpaCtrl::hasPendingEvent() {
  int result = wpa_ctrl_pending(mon);
  assert(result != -1);
  return result == 1; 
}

void WpaCtrl::waitForEvent(Event& event) {
  while (!hasPendingEvent()) {}
  event.parse(receive());
}

void WpaCtrl::waitForEvents(const std::set<std::string>& eventSet, Event& event) {
  Event currentEvent;
  do {
    waitForEvent(currentEvent);
  } while (eventSet.count(currentEvent.name) == 0);
  event = currentEvent;
}

std::string WpaCtrl::receive() {
  size_t read = sizeof(event);
  // if (0 != wpa_ctrl_recv(mon, event, &read)) {
  // return "";
  // }
  assert(0 == wpa_ctrl_recv(mon, event, &read));
  event[read] = 0;
  std::cout << "[WPA] Read \"" << event << "\" [" << read << " bytes]." << std::endl;
  return std::string(event, read);
}
