#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

class Event {
 public:
  std::map<std::string, std::string> keyParameters;
  std::vector<std::string> listParameters;
  std::string name;
  
  Event() {}
  void parse(const std::string& event);
  std::string to_string();
};

class WfdDevice {
 public:
  explicit WfdDevice(const Event& event);
  WfdDevice(const WfdDevice& device) = delete;
  WfdDevice(WfdDevice&& device) = default;
  std::string to_string();

  std::string macAddress;
  std::map<std::string, std::string> properties;
  bool isWifiDisplay;

 private:
  void parseWfdParameter(std::string parameter);
};

class WpaCtrl {
  public:
    WpaCtrl();
    virtual ~WpaCtrl();
    std::string request(const std::string & query);
    bool hasPendingEvent();
    void waitForEvent(Event& event);
    void waitForEvents(const std::set<std::string>& eventSet, Event& event);

  private:
    std::string receive();

    static const char* kWpaSupplicantPath;
    static const char* kWpaSupplicantIface;
    struct wpa_ctrl* ctrl;
    struct wpa_ctrl* mon;
    char reply[2048];
    char event[2048];
    Event eventParser;
};
