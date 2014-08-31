#include "wfd_connection_manager.h"
#include <iostream>
#include <map>
#include <stdexcept>
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
, peerDevice_() {
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
      ctrl_.request("WFD_SUBELEM_SET 0 00060011022a0100");
      ctrl_.request("P2P_GROUP_ADD");
      switchState(state_, StateWaitForConnection);
      return true;
      break;

    case StateWaitForConnection:
      ctrl_.waitForEvents(std::set<std::string>{"P2P-DEVICE-FOUND", "P2P-PROV-DISC-PBC-REQ"}, event_);
      if (event_.name == "P2P-DEVICE-FOUND") {
        std::unique_ptr<WfdDevice> device(new WfdDevice(event_));
        std::cout << device->to_string() << std::endl;
        bool accept = false;
        receiver_.OnConnectionRequest(*device, accept);
        if (accept) {
          std::cout << "[WFD] Connection request accepted." << std::endl;
          peerDevice_ = std::move(device);
        }
        else {
          std::cout << "[WFD] Connection request rejected." << std::endl;
        }
      }
      else if (event_.name == "P2P-PROV-DISC-PBC-REQ") {
        if (peerDevice_ == 0) {
          std::cout << "[WFD] ERROR: Device not ready" << std::endl;
          switchState(state_, StateStop);
          return true;
        }
        else {
          switchState(state_, StateConnecting);
          return true;
        }
      }
      break;

    case StateConnecting:
      std::cout << "[WFD] Connecting with device " << peerDevice_->to_string() << std::endl;
      ctrl_.request("WPS_PBC");
      std::cout << "[WFD] Wait for PBC activation..." << std::endl;
      ctrl_.waitForEvents(std::set<std::string>{"WPS-PBC-ACTIVE"}, event_);
      std::cout << "[WFD] Wait for WPS to complete..." << std::endl;
      ctrl_.waitForEvents(std::set<std::string>{"WPS-SUCCESS"}, event_);
      std::cout << "[WFD] Wait for network is up..." << std::endl;
      ctrl_.waitForEvents(std::set<std::string>{"AP-STA-CONNECTED"}, event_);
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
        peerDevice_.reset();
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

