module;

#include "communication/NetworkMessage.hpp"

#include <steam/steamnetworkingsockets.h>
#include "spdlog/spdlog.h"

#include <cassert>
#include <utility>
#include <unordered_map>

export module Networking.PollGroups.PollGroupBase;

export import Networking.PollGroups.IPollGroup;
import Networking.Types.Connection;

export namespace Soldank
{
class PollGroupBase : public IPollGroup
{
public:
    PollGroupBase(ISteamNetworkingSockets* interface)
        : IPollGroup(interface)
        , poll_group_handle_(interface->CreatePollGroup())
    {
        if (poll_group_handle_ == k_HSteamNetPollGroup_Invalid) {
            spdlog::error("Failed to create a player poll group");
        }
    }

    ~PollGroupBase() override
    {
        GetInterface()->DestroyPollGroup(poll_group_handle_);
        poll_group_handle_ = k_HSteamNetPollGroup_Invalid;
    }

    PollGroupBase(PollGroupBase&& other) = delete;
    PollGroupBase& operator=(PollGroupBase&& other) = delete;
    PollGroupBase(PollGroupBase& other) = delete;
    PollGroupBase& operator=(PollGroupBase& other) = delete;

    void CloseConnection(SteamNetConnectionStatusChangedCallback_t* connection_info) override
    {
        // Ignore if they were not previously connected. (If they disconnected
        // before we accepted the connection.)
        if (connection_info->m_eOldState == k_ESteamNetworkingConnectionState_Connected) {
            auto it_client = connections_.find(connection_info->m_hConn);
            assert(it_client != connections_.end());

            std::string debug_log_action;
            if (connection_info->m_info.m_eState ==
                k_ESteamNetworkingConnectionState_ProblemDetectedLocally) {
                debug_log_action = "problem detected locally";
            } else {
                debug_log_action = "closed by peer";
            }

            spdlog::info("Connection {} {}, reason {}: {}",
                         connection_info->m_info.m_szConnectionDescription,
                         debug_log_action,
                         connection_info->m_info.m_eEndReason,
                         connection_info->m_info.m_szEndDebug);

            SendStringToAllClients(
              std::format("{} {}", it_client->second.nick, "has left the server"),
              it_client->first);
            connections_.erase(it_client);
        } else {
            assert(connection_info->m_eOldState == k_ESteamNetworkingConnectionState_Connecting);
        }

        auto it_client = FindConnection(connection_info->m_hConn);

        GetInterface()->CloseConnection(connection_info->m_hConn, 0, nullptr, false);

        if (it_client != connections_.end()) {
            EraseConnection(it_client);
        }
    }

    bool AssignConnection(const Connection& connection) override
    {
        if (!GetInterface()->SetConnectionPollGroup(connection.connection_handle,
                                                    poll_group_handle_)) {
            GetInterface()->CloseConnection(connection.connection_handle, 0, nullptr, false);
            spdlog::warn("Failed to set poll group?");
            return false;
        }
        connections_[connection.connection_handle] = connection;
        OnAssignConnection(connections_[connection.connection_handle]);

        return true;
    }

    bool IsConnectionAssigned(HSteamNetConnection steam_net_connection_handle) override
    {
        auto it_client = connections_.find(steam_net_connection_handle);
        return it_client != connections_.end();
    }

    unsigned int GetConnectionSoldierId(HSteamNetConnection steam_net_connection_handle) override
    {
        auto it_client = connections_.find(steam_net_connection_handle);
        return it_client->second.soldier_id;
    }

    std::string GetConnectionSoldierNick(HSteamNetConnection steam_net_connection_handle) override
    {
        auto it_client = connections_.find(steam_net_connection_handle);
        return it_client->second.nick;
    }

    void SendNetworkMessage(HSteamNetConnection connection_id,
                            const NetworkMessage& network_message) override
    {
        // TODO: handle result
        auto result = GetInterface()->SendMessageToConnection(connection_id,
                                                              network_message.GetData().data(),
                                                              network_message.GetData().size(),
                                                              k_nSteamNetworkingSend_Unreliable,
                                                              nullptr);
    }

    void SendNetworkMessageToAll(
      const NetworkMessage& network_message,
      std::optional<unsigned int> except_connection_id = std::nullopt) override
    {
        for (auto& connection : connections_) {
            if (except_connection_id.has_value() &&
                *except_connection_id == connection.second.connection_handle) {
                continue;
            }
            // TODO: handle result
            auto result =
              GetInterface()->SendMessageToConnection(connection.second.connection_handle,
                                                      network_message.GetData().data(),
                                                      network_message.GetData().size(),
                                                      k_nSteamNetworkingSend_Unreliable,
                                                      nullptr);
        }
    }

    void SendReliableNetworkMessage(unsigned int connection_id,
                                    const NetworkMessage& network_message) override
    {
        // TODO: handle result
        auto result = GetInterface()->SendMessageToConnection(connection_id,
                                                              network_message.GetData().data(),
                                                              network_message.GetData().size(),
                                                              k_nSteamNetworkingSend_Reliable,
                                                              nullptr);
    }

    void SendReliableNetworkMessageToAll(
      const NetworkMessage& network_message,
      std::optional<unsigned int> except_connection_id = std::nullopt) override
    {
        for (auto& connection : connections_) {
            if (except_connection_id.has_value() &&
                *except_connection_id == connection.second.connection_handle) {
                continue;
            }
            // TODO: handle result
            auto result =
              GetInterface()->SendMessageToConnection(connection.second.connection_handle,
                                                      network_message.GetData().data(),
                                                      network_message.GetData().size(),
                                                      k_nSteamNetworkingSend_Reliable,
                                                      nullptr);
        }
    }

protected:
    virtual void OnAssignConnection(Connection& /* connection */) {}

    HSteamNetPollGroup GetPollGroupHandle() const { return poll_group_handle_; }

    std::unordered_map<HSteamNetConnection, Connection>::iterator FindConnection(
      HSteamNetConnection steam_net_connection_handle)
    {
        return connections_.find(steam_net_connection_handle);
    }

    HSteamNetConnection FindConnectionHandleBySoldierId(unsigned int soldier_id)
    {
        for (const auto& connection : connections_) {
            if (connection.second.soldier_id == soldier_id) {
                return connection.second.connection_handle;
            }
        }

        spdlog::critical("[FindConnectionHandleBySoldierId] Wrong soldier_id");
        return 0;
    }

    void EraseConnection(
      std::unordered_map<HSteamNetConnection, Connection>::iterator it_connection)
    {
        connections_.erase(it_connection);
    }

    void SendStringToClient(HSteamNetConnection conn, const std::string& message)
    {
        GetInterface()->SendMessageToConnection(
          conn, message.c_str(), message.size(), k_nSteamNetworkingSend_Reliable, nullptr);
    }

    void SendStringToAllClients(const std::string& message, HSteamNetConnection except)
    {
        for (auto& c : connections_) {
            if (c.first != except) {
                SendStringToClient(c.first, message);
            }
        }
    }

    void SetClientNick(HSteamNetConnection h_conn, const std::string& nick)
    {
        connections_[h_conn].nick = nick;
        // Set the connection name, too, which is useful for debugging
        GetInterface()->SetConnectionName(h_conn, nick.c_str());
    }

private:
    HSteamNetPollGroup poll_group_handle_;

    std::unordered_map<HSteamNetConnection, Connection> connections_;
};
} // namespace Soldank
