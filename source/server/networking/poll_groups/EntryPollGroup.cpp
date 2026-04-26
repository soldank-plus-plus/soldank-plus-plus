module;

#include <utility>
#include <span>
#include <cassert>
#include <unordered_map>
#include <string>
#include <memory>

export module Networking.PollGroups.EntryPollGroup;

export import Networking.PollGroups.PollGroupBase;
import Networking.Types.Connection;

import Shared.Networking.NetworkEvent;

import Extern.Spdlog;
import Extern.GameNetworkingSockets;

export namespace Soldank
{
class EntryPollGroup : public PollGroupBase
{
public:
    EntryPollGroup(GNS::ISteamNetworkingSockets* interface)
        : PollGroupBase(interface)
    {
    }

    void PollIncomingMessages() override
    {
        while (true) {
            GNS::ISteamNetworkingMessage* incoming_message = nullptr;
            int messages_count = GetInterface()->ReceiveMessagesOnPollGroup(
              GetPollGroupHandle(), &incoming_message, 1);
            if (messages_count == 0) {
                break;
            }
            if (messages_count < 0) {
                Spdlog::error("[EntryPollGroup] Error checking for messages");
            }
            assert(messages_count == 1 && incoming_message);
            assert(IsConnectionAssigned(incoming_message->m_conn));
            auto it_client = FindConnection(incoming_message->m_conn);

            std::string message_from_client;
            message_from_client.assign(static_cast<char*>(incoming_message->m_pData),
                                       incoming_message->m_cbSize);
            it_client->second.nick = message_from_client;
            SetClientNick(it_client->second.connection_handle, it_client->second.nick);
            Spdlog::info("[EntryPollGroup] Name assigned to connection {}: {}",
                         it_client->second.connection_handle,
                         it_client->second.nick);
            incoming_message->Release();

            SendNetworkMessage(
              it_client->second.connection_handle,
              { NetworkEvent::ChatMessage, "Welcome to the server " + it_client->second.nick });

            player_poll_group_->AssignConnection(it_client->second);
            EraseConnection(it_client);
        }
    }

    void AcceptConnection(
      GNS::SteamNetConnectionStatusChangedCallback_t* new_connection_info) override
    {
        // This must be a new connection
        assert(!IsConnectionAssigned(new_connection_info->m_hConn));

        Spdlog::info("[EntryPollGroup] Connection request from {}",
                     std::span{ new_connection_info->m_info.m_szConnectionDescription }.data());

        if (GetInterface()->AcceptConnection(new_connection_info->m_hConn) != GNS::EResult::OK) {
            GetInterface()->CloseConnection(new_connection_info->m_hConn, 0, nullptr, false);
            Spdlog::warn("[EntryPollGroup] Can't accept connection. (It was already closed?)");
            return;
        }

        Spdlog::info("[EntryPollGroup] Connection accepted: {}\n",
                     std::span{ new_connection_info->m_info.m_szConnectionDescription }.data());

        if (!AssignConnection({ .connection_handle = new_connection_info->m_hConn,
                                .nick = "NEW CONNECTION PLACEHOLDER" })) {
            return;
        }

        Spdlog::info("[EntryPollGroup] Connection assigned: {}",
                     std::span{ new_connection_info->m_info.m_szConnectionDescription }.data());

        SetClientNick(new_connection_info->m_hConn, "NEW CONNECTION PLACEHOLDER");
    }

    void RegisterPlayerPollGroup(std::shared_ptr<IPollGroup> poll_group)
    {
        player_poll_group_ = std::move(poll_group);
    }

private:
    std::shared_ptr<IPollGroup> player_poll_group_;
};
} // namespace Soldank
