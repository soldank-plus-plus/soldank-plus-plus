module;

#include <cassert>
#include <cstdint>
#include <memory>
#include <span>

export module Networking.PollGroups.PlayerPollGroup;

export import Networking.PollGroups.PollGroupBase;
import Networking.Types.Connection;

import Shared.Core.IWorld;

import Shared.Networking.NetworkEventDispatcher;
import Shared.Networking.NetworkEvent;
import Shared.Networking.NetworkMessage;

import Extern.Spdlog;
import Extern.GameNetworkingSockets;

export namespace Soldank
{
class PlayerPollGroup : public PollGroupBase
{
public:
    PlayerPollGroup(GNS::ISteamNetworkingSockets* interface)
        : PollGroupBase(interface)
    {
    }

    void SetServerNetworkEventDispatcher(
      const std::shared_ptr<NetworkEventDispatcher>& network_event_dispatcher)
    {
        network_event_dispatcher_ = network_event_dispatcher;
    }

    void SetWorld(const std::shared_ptr<IWorld>& world) { world_ = world; }

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
                Spdlog::error("Error checking for messages");
                break;
            }
            assert(messages_count == 1 && incoming_message);
            assert(IsConnectionAssigned(incoming_message->m_conn));

            auto it_client = FindConnection(incoming_message->m_conn);

            std::span<char> received_bytes{ static_cast<char*>(incoming_message->m_pData),
                                            static_cast<unsigned int>(incoming_message->m_cbSize) };
            NetworkMessage network_message(received_bytes);

            unsigned int connection_id = incoming_message->m_conn;

            ConnectionMetadata connection_metadata{
                .connection_id = connection_id,
                .send_message_to_connection =
                  [&](const NetworkMessage& message) { SendNetworkMessage(connection_id, message); }
            };

            incoming_message->Release();
            network_event_dispatcher_->ProcessNetworkMessage(connection_metadata, network_message);
        }
    }

    void AcceptConnection(
      GNS::SteamNetConnectionStatusChangedCallback_t* /*new_connection_info*/) override
    {
    }

private:
    void OnAssignConnection(Connection& connection) override
    {
        world_->GetStateManager()->ForEachSoldier([&](const auto& soldier) {
            std::string player_nick =
              GetConnectionSoldierNick(FindConnectionHandleBySoldierId(soldier.id));
            NetworkMessage network_message(NetworkEvent::SoldierInfo, soldier.id, player_nick);
            SendReliableNetworkMessage(connection.connection_handle, network_message);
        });

        std::uint8_t soldier_id = world_->CreateSoldier().id;
        connection.soldier_id = soldier_id;
        Spdlog::info("OnAssignPlayerId: {}", soldier_id);
        NetworkMessage network_message(NetworkEvent::AssignPlayerId, soldier_id);
        SendReliableNetworkMessage(connection.connection_handle, network_message);

        network_message = { NetworkEvent::SoldierInfo, soldier_id, connection.nick };
        SendReliableNetworkMessage(connection.connection_handle, network_message);

        SendReliableNetworkMessageToAll(network_message, connection.connection_handle);

        world_->SpawnSoldier(soldier_id);
    }

    std::shared_ptr<NetworkEventDispatcher> network_event_dispatcher_;
    std::shared_ptr<IWorld> world_;
};
} // namespace Soldank
