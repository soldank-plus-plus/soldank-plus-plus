module;

#include <optional>
#include <string>

export module Networking.PollGroups.IPollGroup;

import Networking.Types.Connection;
import Networking.Transport.TransportTypes;

import Shared.Networking.NetworkMessage;

import Extern.GameNetworkingSockets;

export namespace Soldank
{
class IPollGroup
{
public:
    virtual ~IPollGroup() = default;

    IPollGroup(IPollGroup&& other) = delete;
    IPollGroup& operator=(IPollGroup&& other) = delete;
    IPollGroup(IPollGroup& other) = delete;
    IPollGroup& operator=(IPollGroup& other) = delete;

    virtual void PollIncomingMessages() = 0;
    virtual void AcceptConnection(
      GNS::SteamNetConnectionStatusChangedCallback_t* new_connection_info) = 0;
    virtual void CloseConnection(
      GNS::SteamNetConnectionStatusChangedCallback_t* connection_info) = 0;
    virtual bool AssignConnection(const Connection& connection) = 0;
    virtual bool IsConnectionAssigned(ConnectionId connection_id) = 0;
    virtual unsigned int GetConnectionSoldierId(ConnectionId connection_id) = 0;
    virtual std::string GetConnectionSoldierNick(ConnectionId connection_id) = 0;

    virtual void SendNetworkMessage(ConnectionId connection_id,
                                    const NetworkMessage& network_message) = 0;
    virtual void SendNetworkMessageToAll(
      const NetworkMessage& network_message,
      std::optional<ConnectionId> except_connection_id = std::nullopt) = 0;

    virtual void SendReliableNetworkMessage(ConnectionId connection_id,
                                            const NetworkMessage& network_message) = 0;
    virtual void SendReliableNetworkMessageToAll(
      const NetworkMessage& network_message,
      std::optional<ConnectionId> except_connection_id = std::nullopt) = 0;

protected:
    IPollGroup(GNS::ISteamNetworkingSockets* interface)
        : interface_(interface)
    {
    }

    GNS::ISteamNetworkingSockets* GetInterface() const { return interface_; }

private:
    GNS::ISteamNetworkingSockets* interface_;
};
} // namespace Soldank
