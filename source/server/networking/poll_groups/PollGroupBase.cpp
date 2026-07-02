module;

#include <cassert>
#include <string>
#include <optional>
#include <unordered_map>
#include <format>

export module Networking.PollGroups.PollGroupBase;

export import Networking.PollGroups.IPollGroup;
import Networking.Types.Connection;
import Networking.Transport.GnsServerTransport;
import Networking.Transport.TransportTypes;

import Shared.Networking.NetworkMessage;

import Extern.Spdlog;
import Extern.GameNetworkingSockets;

export namespace Soldank
{
class PollGroupBase : public IPollGroup
{
public:
    PollGroupBase(GNS::ISteamNetworkingSockets* interface)
        : IPollGroup(interface)
        , poll_group_handle_(interface->CreatePollGroup())
    {
        if (poll_group_handle_ == GNS::HSteamNetPollGroup_Enum::Invalid) {
            Spdlog::error("Failed to create a player poll group");
        }
    }

    ~PollGroupBase() override
    {
        GetInterface()->DestroyPollGroup(poll_group_handle_);
        poll_group_handle_ = GNS::HSteamNetPollGroup_Enum::Invalid;
    }

    PollGroupBase(PollGroupBase&& other) = delete;
    PollGroupBase& operator=(PollGroupBase&& other) = delete;
    PollGroupBase(PollGroupBase& other) = delete;
    PollGroupBase& operator=(PollGroupBase& other) = delete;

    void CloseConnection(GNS::SteamNetConnectionStatusChangedCallback_t* connection_info) override
    {
        // Ignore if they were not previously connected. (If they disconnected
        // before we accepted the connection.)
        if (connection_info->m_eOldState == GNS::ESteamNetworkingConnectionState::Connected) {
            const auto connection_id =
              GnsServerTransport::ToConnectionId(connection_info->m_hConn);
            auto it_client = connections_.find(connection_id);
            assert(it_client != connections_.end());

            std::string debug_log_action;
            if (connection_info->m_info.m_eState ==
                GNS::ESteamNetworkingConnectionState::ProblemDetectedLocally) {
                debug_log_action = "problem detected locally";
            } else {
                debug_log_action = "closed by peer";
            }

            Spdlog::info("Connection {} {}, reason {}: {}",
                         connection_info->m_info.m_szConnectionDescription,
                         debug_log_action,
                         connection_info->m_info.m_eEndReason,
                         connection_info->m_info.m_szEndDebug);

            SendStringToAllClients(
              std::format("{} {}", it_client->second.nick, "has left the server"),
              it_client->first);
            connections_.erase(it_client);
        } else {
            assert(connection_info->m_eOldState ==
                   GNS::ESteamNetworkingConnectionState::Connecting);
        }

        auto it_client =
          FindConnection(GnsServerTransport::ToConnectionId(connection_info->m_hConn));

        GnsServerTransport::CloseConnection(
          GetInterface(), GnsServerTransport::ToConnectionId(connection_info->m_hConn));

        if (it_client != connections_.end()) {
            EraseConnection(it_client);
        }
    }

    bool AssignConnection(const Connection& connection) override
    {
        if (!GnsServerTransport::SetConnectionPollGroup(
              GetInterface(), connection.connection_id, poll_group_handle_)) {
            GnsServerTransport::CloseConnection(GetInterface(), connection.connection_id);
            Spdlog::warn("Failed to set poll group?");
            return false;
        }
        connections_[connection.connection_id] = connection;
        OnAssignConnection(connections_[connection.connection_id]);

        return true;
    }

    bool IsConnectionAssigned(ConnectionId connection_id) override
    {
        auto it_client = connections_.find(connection_id);
        return it_client != connections_.end();
    }

    unsigned int GetConnectionSoldierId(ConnectionId connection_id) override
    {
        auto it_client = connections_.find(connection_id);
        return it_client->second.soldier_id;
    }

    std::string GetConnectionSoldierNick(ConnectionId connection_id) override
    {
        auto it_client = connections_.find(connection_id);
        return it_client->second.nick;
    }

    void SendNetworkMessage(ConnectionId connection_id,
                            const NetworkMessage& network_message) override
    {
        // TODO: handle result
        GnsServerTransport::SendNetworkMessage(
          GetInterface(), connection_id, network_message, DeliveryMode::Unreliable);
    }

    void SendNetworkMessageToAll(
      const NetworkMessage& network_message,
      std::optional<ConnectionId> except_connection_id = std::nullopt) override
    {
        for (auto& connection : connections_) {
            if (except_connection_id.has_value() &&
                *except_connection_id == connection.second.connection_id) {
                continue;
            }
            // TODO: handle result
            GnsServerTransport::SendNetworkMessage(GetInterface(),
                                                   connection.second.connection_id,
                                                   network_message,
                                                   DeliveryMode::Unreliable);
        }
    }

    void SendReliableNetworkMessage(ConnectionId connection_id,
                                    const NetworkMessage& network_message) override
    {
        // TODO: handle result
        GnsServerTransport::SendNetworkMessage(
          GetInterface(), connection_id, network_message, DeliveryMode::Reliable);
    }

    void SendReliableNetworkMessageToAll(
      const NetworkMessage& network_message,
      std::optional<ConnectionId> except_connection_id = std::nullopt) override
    {
        for (auto& connection : connections_) {
            if (except_connection_id.has_value() &&
                *except_connection_id == connection.second.connection_id) {
                continue;
            }
            // TODO: handle result
            GnsServerTransport::SendNetworkMessage(GetInterface(),
                                                   connection.second.connection_id,
                                                   network_message,
                                                   DeliveryMode::Reliable);
        }
    }

protected:
    virtual void OnAssignConnection(Connection& /* connection */) {}

    GNS::HSteamNetPollGroup GetPollGroupHandle() const { return poll_group_handle_; }

    std::unordered_map<ConnectionId, Connection>::iterator FindConnection(
      ConnectionId connection_id)
    {
        return connections_.find(connection_id);
    }

    ConnectionId FindConnectionIdBySoldierId(unsigned int soldier_id)
    {
        for (const auto& connection : connections_) {
            if (connection.second.soldier_id == soldier_id) {
                return connection.second.connection_id;
            }
        }

        Spdlog::critical("[FindConnectionIdBySoldierId] Wrong soldier_id");
        return 0;
    }

    void EraseConnection(std::unordered_map<ConnectionId, Connection>::iterator it_connection)
    {
        connections_.erase(it_connection);
    }

    void SendStringToClient(ConnectionId connection_id, const std::string& message)
    {
        GnsServerTransport::SendString(
          GetInterface(), connection_id, message, DeliveryMode::Reliable);
    }

    void SendStringToAllClients(const std::string& message, ConnectionId except)
    {
        for (auto& c : connections_) {
            if (c.first != except) {
                SendStringToClient(c.first, message);
            }
        }
    }

    void SetClientNick(ConnectionId connection_id, const std::string& nick)
    {
        connections_[connection_id].nick = nick;
        // Set the connection name, too, which is useful for debugging
        GnsServerTransport::SetConnectionName(GetInterface(), connection_id, nick);
    }

private:
    GNS::HSteamNetPollGroup poll_group_handle_;

    std::unordered_map<ConnectionId, Connection> connections_;
};
} // namespace Soldank
