module;

#include <memory>

export module Networking.IConnection;

import Shared.Networking.NetworkEventDispatcher;
import Shared.Networking.NetworkMessage;

import Extern.GameNetworkingSockets;

export namespace Soldank
{
class IConnection
{
public:
    virtual ~IConnection() = default;

    virtual void PollIncomingMessages(
      const std::shared_ptr<NetworkEventDispatcher>& network_event_dispatcher) = 0;

    virtual void CloseConnection() = 0;

    virtual void AssertConnectionInfo(
      GNS::SteamNetConnectionStatusChangedCallback_t* connection_info) const = 0;

    virtual void SendNetworkMessage(const NetworkMessage& network_message) = 0;
};
} // namespace Soldank
