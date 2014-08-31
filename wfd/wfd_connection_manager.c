#pragma once

#include "wpa_types.h"

class WfdConnectionManager {
 public: 

  enum State {
    StateInitial,
    StateWaitForConnection,
    StateConnecting,
    StateConnected,
    StateStop
  };

  class ReceiverInterface {
   public:
    virtual void OnConnectionRequest(const WfdDevice& device, bool& accept) = 0;
    virtual void OnConnect(const WfdDevice& device) = 0;
    virtual void OnDisconnect(const WfdDevice& device) = 0;
  };


  WfdConnectionManager(ReceiverInterface& receiver);
  ~WfdConnectionManager();  
  void EnableWifiDisplay();
  void WaitForConnection();
  void WaitForDisconnection();
  void Disconnect();

 private:

  ReceiverInterface& receiver_;
  State state_;
  std::unique_ptr<WfdDevice> peerDevice_;
  WpaCtrl ctrl_;
  Event event;

  bool handleState();
  void switchState(State& state, State newState);
};

std::ostream& operator<<(std::ostream& stream, WfdConnectionManager::State state);
