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
import Networking.Transport.GnsServerTransport;
import Networking.Transport.TransportTypes;

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
            const auto connection_id =
              GnsServerTransport::ToConnectionId(incoming_message->m_conn);
            assert(IsConnectionAssigned(connection_id));
            auto it_client = FindConnection(connection_id);

            std::string message_from_client;
            message_from_client.assign(static_cast<char*>(incoming_message->m_pData),
                                       incoming_message->m_cbSize);
            it_client->second.nick = message_from_client;
            SetClientNick(it_client->second.connection_id, it_client->second.nick);
            Spdlog::info("[EntryPollGroup] Name assigned to connection {}: {}",
                         it_client->second.connection_id,
                         it_client->second.nick);
            incoming_message->Release();

            SendNetworkMessage(
              it_client->second.connection_id,
              { NetworkEvent::ChatMessage, "Welcome to the server " + it_client->second.nick });

            player_poll_group_->AssignConnection(it_client->second);
            EraseConnection(it_client);
        }
    }

    void AcceptConnection(
      GNS::SteamNetConnectionStatusChangedCallback_t* new_connection_info) override
    {
        // This must be a new connection
        const auto connection_id =
          GnsServerTransport::ToConnectionId(new_connection_info->m_hConn);
        assert(!IsConnectionAssigned(connection_id));

        Spdlog::info("[EntryPollGroup] Connection request from {}",
                     std::span{ new_connection_info->m_info.m_szConnectionDescription }.data());

        if (GetInterface()->AcceptConnection(new_connection_info->m_hConn) != GNS::EResult::OK) {
            GetInterface()->CloseConnection(new_connection_info->m_hConn, 0, nullptr, false);
            Spdlog::warn("[EntryPollGroup] Can't accept connection. (It was already closed?)");
            return;
        }

        Spdlog::info("[EntryPollGroup] Connection accepted: {}\n",
                     std::span{ new_connection_info->m_info.m_szConnectionDescription }.data());

        if (!AssignConnection({ .connection_id = connection_id,
                                .nick = "NEW CONNECTION PLACEHOLDER" })) {
            return;
        }

        Spdlog::info("[EntryPollGroup] Connection assigned: {}",
                     std::span{ new_connection_info->m_info.m_szConnectionDescription }.data());

        SetClientNick(connection_id, "NEW CONNECTION PLACEHOLDER");
    }

    void RegisterPlayerPollGroup(std::shared_ptr<IPollGroup> poll_group)
    {
        player_poll_group_ = std::move(poll_group);
    }

private:
    std::shared_ptr<IPollGroup> player_poll_group_;
};
} // namespace Soldank
