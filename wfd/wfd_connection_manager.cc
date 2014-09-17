#include "wfd_connection_manager.h"
#include <iostream>
#include <map>
#include <stdexcept>
#include <cassert>
#include "wpa_types.h"

std::map<WfdConnectionManager::State, std::string> StateStringMap{
  {WfdConnectionManager::StateInitial, "StateInitial"},
  {WfdConnectionManager::StateWaitForConnection, "StateWaitForConnection"},
  {WfdConnectionManager::StateConnecting, "StateConnecting"},
  {WfdConnectionManager::StateConnected, "StateConnected"},
  {WfdConnectionManager::StateStop, "StateStop"}
};

std::ostream& operator<<(std::ostream& stream, WfdConnectionManager::State state) {
  return stream << StateStringMap[state];
}

WfdConnectionManager::WfdConnectionManager(ReceiverInterface& receiver)
: receiver_(receiver)
, state_(StateStop)
, peerDevice_(0) {
  assert("PONG" == ctrl_.request("PING"));
  if (ctrl_.request("PING") != "PONG") {
    throw std::runtime_error("PING PONG test failed.");
  }
}

WfdConnectionManager::~WfdConnectionManager() {
  ctrl_.request("FLUSH");
}

void WfdConnectionManager::EnableWifiDisplay() {
  if (state_ == StateStop) {
    state_ = StateInitial;
    while (!handleState()) {}
  }
}

void WfdConnectionManager::WaitForConnection() {
  if (state_ == StateWaitForConnection) {
    while (state_ != StateConnected && state_ != StateStop) {
      handleState();
    }
  }
}

void WfdConnectionManager::WaitForDisconnection() {
  if (state_ == StateConnecting || state_ == StateConnected) {
    while (state_ != StateStop) {
      !handleState();
    }
  }
}

void WfdConnectionManager::Disconnect() {
  if (state_ == StateConnected) {
  }
}

bool WfdConnectionManager::handleState() {
  switch(state_) {

    case StateInitial:
      ctrl_.request("FLUSH");
      ctrl_.request("SET wifi_display 1");
      ctrl_.request("SET config_methods pbc");
      ctrl_.request("WFD_SUBELEM_SET 0 00060011022a0100");
      ctrl_.request("P2P_GROUP_ADD");
      switchState(state_, StateWaitForConnection);
      return true;
      break;

    case StateWaitForConnection: {
      ctrl_.waitForEvents(std::set<std::string>{"P2P-DEVICE-FOUND", "P2P-PROV-DISC-PBC-REQ", "P2P-PROV-DISC-ENTER-PIN"}, event_);
      std::string p2pAddress = event_.listParameters[0];
      if (event_.name == "P2P-DEVICE-FOUND") {
        if (peerDevices_.count(p2pAddress) == 0) {
          std::unique_ptr<WfdDevice> device(new WfdDevice(event_));
          std::cout << "[WFD] New device registered: " << device->to_string() << std::endl;
          peerDevices_.insert(std::make_pair(device->macAddress, std::move(device)));
        }
      }
      else {
        if (peerDevices_.count(p2pAddress) == 0)  {
          std::cout << "[WFD] ERROR: unknown device address: " << p2pAddress << std::endl;
          switchState(state_, StateStop);
          return true;
        }

        bool accept = false;
        if (event_.name == "P2P-PROV-DISC-PBC-REQ") {
          receiver_.OnConnectionRequestPbc(*peerDevices_[p2pAddress], accept);
          if (accept) {
            assert("OK" == ctrl_.request("WPS_PBC"));
          }
        }
        else if (event_.name == "P2P-PROV-DISC-ENTER-PIN") {
          std::string pin;
          receiver_.OnConnectionRequestPin(*peerDevices_[p2pAddress], accept, pin);
          if (accept) {
            ctrl_.request(std::string("WPS_PIN any ") + pin);
          }
        }

        if (accept) {
          peerDevice_ = peerDevices_[p2pAddress].get();
          std::cout << "[WFD] Connection request accepted." << std::endl;
          switchState(state_, StateConnecting);
          return true;
        }
        else {
          std::cout << "[WFD] Connection request rejected." << std::endl;
          switchState(state_, StateStop);
          return true;
        }
      }
      break;
    }

    case StateConnecting:
      assert(peerDevice_ != 0);
      std::cout << "[WFD] Connecting with device " << peerDevice_->to_string() << std::endl;
      // std::cout << "[WFD] Wait for PBC activation..." << std::endl;
      // ctrl_.waitForEvents(std::set<std::string>{"WPS-PBC-ACTIVE"}, event_);
      std::cout << "[WFD] Wait for WPS to complete..." << std::endl;
      ctrl_.waitForEvents(std::set<std::string>{"WPS-SUCCESS"}, event_);
      std::cout << "[WFD] Wait for network is up..." << std::endl;
      ctrl_.waitForEvents(std::set<std::string>{"AP-STA-CONNECTED"}, event_);
      std::cout << "[WFD] STA " << event_.listParameters[0] << " connected." << std::endl;
      if (peerDevices_.count(event_.keyParameters["p2p_dev_addr"])) {
        std::cout << "[WFD] Corresponding p2p device: " << peerDevices_[event_.keyParameters["p2p_dev_addr"]]->to_string() << std::endl;
      }
      switchState(state_, StateConnected);
      receiver_.OnConnect(*peerDevice_);
      return true;
      break;

    case StateConnected:
      std::cout << "[WFD] Wait for remote device disconnected..." << std::endl;
      ctrl_.waitForEvents(std::set<std::string>{"AP-STA-DISCONNECTED"}, event_);
      if (event_.keyParameters["p2p_dev_addr"] == peerDevice_->properties["p2p_dev_addr"]) {
        std::cout << "[WFD] Remote device disconnected: " << peerDevice_->to_string() << std::endl;
        switchState(state_, StateStop);
        receiver_.OnDisconnect(*peerDevice_);
        peerDevice_ = 0;
        return true;
      }
      break;

    case StateStop:
      break;
  }
  return false;
}

void WfdConnectionManager::switchState(State& state, State newState) {
  if (state == newState)
    return;
  std::cout << "[STATE] " << state << " >>> " << newState << std::endl;
  state = newState;
}

