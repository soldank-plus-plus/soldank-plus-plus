module;

#include <memory>
#include <cassert>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

export module Networking.GameServer;

import Application.ServerState;

import Networking.IGameServer;
import Networking.Interface.NetworkingInterface;
import Networking.PollGroups.EntryPollGroup;
import Networking.PollGroups.PlayerPollGroup;
import Networking.Transport.GnsServerTransport;
import Networking.Transport.TransportTypes;
#if defined(SOLDANK_ENABLE_WEBRTC_SERVER_TRANSPORT)
import Networking.Transport.IServerTransport;
import Networking.Transport.WebRtcServerTransport;
#endif

import Shared.Core.IWorld;

import Shared.Networking.NetworkMessage;
import Shared.Networking.NetworkEventDispatcher;
import Shared.Networking.NetworkEvent;

import Extern.Spdlog;
import Extern.GameNetworkingSockets;

export namespace Soldank
{
class GameServer : public IGameServer
{
public:
    GameServer(std::uint16_t port,
               const std::shared_ptr<NetworkEventDispatcher>& network_event_dispatcher,
               const std::shared_ptr<IWorld>& world,
               const std::shared_ptr<ServerState>& server_state)
        : world_(world)
        , server_state_(server_state)
        , network_event_dispatcher_(network_event_dispatcher)
    {
        // TODO: we shouldn't init NetworkingInterface here because it's global
        NetworkingInterface::Init(port);
        NetworkingInterface::RegisterObserver(
          [this](GNS::SteamNetConnectionStatusChangedCallback_t* p_info) {
              OnSteamNetConnectionStatusChanged(p_info);
          });

        entry_poll_group_ = NetworkingInterface::CreatePollGroup<EntryPollGroup>();
        player_poll_group_ = NetworkingInterface::CreatePollGroup<PlayerPollGroup>();
        entry_poll_group_->RegisterPlayerPollGroup(player_poll_group_);
        player_poll_group_->SetServerNetworkEventDispatcher(network_event_dispatcher);
        player_poll_group_->SetWorld(world);

#if defined(SOLDANK_ENABLE_WEBRTC_SERVER_TRANSPORT)
        transports_.push_back(std::make_unique<WebRtcServerTransport>());
        transports_.back()->Init(port);
#endif
    };
    ~GameServer() override { NetworkingInterface::Free(); }

    GameServer(GameServer&& other) = delete;
    GameServer& operator=(GameServer&& other) = delete;
    GameServer(GameServer& other) = delete;
    GameServer& operator=(GameServer& other) = delete;

    void Update() override
    {
        entry_poll_group_->PollIncomingMessages();
        player_poll_group_->PollIncomingMessages();
#if defined(SOLDANK_ENABLE_WEBRTC_SERVER_TRANSPORT)
        for (auto& transport : transports_) {
            ProcessWebRtcIncomingPackets(*transport);
            transport->PollConnectionStateChanges();
        }
#endif
        NetworkingInterface::PollConnectionStateChanges();
    }

    void SendNetworkMessage(unsigned int connection_id,
                            const NetworkMessage& network_message) override
    {
        if (entry_poll_group_->IsConnectionAssigned(connection_id)) {
            entry_poll_group_->SendNetworkMessage(connection_id, network_message);
        }
        if (player_poll_group_->IsConnectionAssigned(connection_id)) {
            player_poll_group_->SendNetworkMessage(connection_id, network_message);
        }
#if defined(SOLDANK_ENABLE_WEBRTC_SERVER_TRANSPORT)
        if (web_rtc_connections_.contains(connection_id)) {
            SendWebRtcNetworkMessage(connection_id, network_message, DeliveryMode::Unreliable);
        }
#endif
    }
    void SendNetworkMessageToAll(const NetworkMessage& network_message) override
    {
        player_poll_group_->SendNetworkMessageToAll(network_message);
#if defined(SOLDANK_ENABLE_WEBRTC_SERVER_TRANSPORT)
        for (const auto& [connection_id, connection] : web_rtc_connections_) {
            if (connection.soldier_id.has_value()) {
                SendWebRtcNetworkMessage(connection_id, network_message, DeliveryMode::Unreliable);
            }
        }
#endif
    }

    unsigned int GetSoldierIdFromConnectionId(unsigned int connection_id) override
    {
#if defined(SOLDANK_ENABLE_WEBRTC_SERVER_TRANSPORT)
        const auto web_rtc_connection = web_rtc_connections_.find(connection_id);
        if (web_rtc_connection != web_rtc_connections_.end() &&
            web_rtc_connection->second.soldier_id.has_value()) {
            return *web_rtc_connection->second.soldier_id;
        }
#endif
        return player_poll_group_->GetConnectionSoldierId(connection_id);
    }

private:
    std::unique_ptr<EntryPollGroup> entry_poll_group_;
    std::shared_ptr<PlayerPollGroup> player_poll_group_;
#if defined(SOLDANK_ENABLE_WEBRTC_SERVER_TRANSPORT)
    std::vector<std::unique_ptr<IServerTransport>> transports_;

    struct WebRtcConnection
    {
        ConnectionId connection_id;
        std::string nick;
        std::optional<unsigned int> soldier_id;
    };

    std::unordered_map<ConnectionId, WebRtcConnection> web_rtc_connections_;
#endif

    void OnSteamNetConnectionStatusChanged(GNS::SteamNetConnectionStatusChangedCallback_t* p_info)
    {
        switch (p_info->m_info.m_eState) {
            case GNS::ESteamNetworkingConnectionState::None:
                // NOTE: We will get callbacks here when we destroy connections.  You can ignore
                // these.
                break;

            case GNS::ESteamNetworkingConnectionState::ClosedByPeer:
            case GNS::ESteamNetworkingConnectionState::ProblemDetectedLocally: {
                const auto connection_id = GnsServerTransport::ToConnectionId(p_info->m_hConn);
                if (entry_poll_group_->IsConnectionAssigned(connection_id)) {
                    entry_poll_group_->CloseConnection(p_info);
                }
                if (player_poll_group_->IsConnectionAssigned(connection_id)) {
                    Spdlog::info("CLOSING CONNECTION conn: {}", connection_id);
                    std::uint8_t soldier_id =
                      player_poll_group_->GetConnectionSoldierId(connection_id);
                    Spdlog::info("CLOSING CONNECTION soldier_id: {}", soldier_id);
                    NetworkMessage network_message(NetworkEvent::PlayerLeave, soldier_id);
                    player_poll_group_->SendReliableNetworkMessageToAll(network_message,
                                                                        connection_id);
                    player_poll_group_->CloseConnection(p_info);

                    Spdlog::info("CLOSING CONNECTION soldiers size before: {}",
                                 world_->GetStateManager()->GetSoldiersCount());
                    world_->GetStateManager()->RemoveSoldier(soldier_id);

                    Spdlog::info("CLOSING CONNECTION soldiers size after: {}",
                                 world_->GetStateManager()->GetSoldiersCount());

                    server_state_->last_processed_input_id.at(soldier_id) = 0;
                }
                break;
            }

            case GNS::ESteamNetworkingConnectionState::Connecting: {
                entry_poll_group_->AcceptConnection(p_info);
                // player_poll_group_->AcceptConnection(p_info);
                break;
            }

            case GNS::ESteamNetworkingConnectionState::Connected:
                // We will get a callback immediately after accepting the connection.
                // Since we are the server, we can ignore this, it's not news to us.

            default:
                // Silences -Wswitch
                break;
        }
    }

    std::shared_ptr<IWorld> world_;
    std::shared_ptr<ServerState> server_state_;
    std::shared_ptr<NetworkEventDispatcher> network_event_dispatcher_;

#if defined(SOLDANK_ENABLE_WEBRTC_SERVER_TRANSPORT)
    void SendWebRtcNetworkMessage(ConnectionId connection_id,
                                  const NetworkMessage& network_message,
                                  DeliveryMode delivery_mode)
    {
        std::span<const char> payload{ network_message.GetData().data(),
                                       network_message.GetData().size() };
        for (auto& transport : transports_) {
            transport->Send(connection_id, payload, delivery_mode);
        }
    }

    void ProcessWebRtcIncomingPackets(IServerTransport& transport)
    {
        for (const auto& packet : transport.PollIncomingPackets()) {
            const auto connection = web_rtc_connections_.find(packet.connection_id);
            if (connection == web_rtc_connections_.end() ||
                !connection->second.soldier_id.has_value()) {
                AcceptWebRtcConnection(transport, packet);
                continue;
            }

            std::span<const char> received_bytes{
                reinterpret_cast<const char*>(packet.payload.data()), packet.payload.size()
            };
            NetworkMessage network_message(received_bytes);
            ConnectionMetadata connection_metadata{
                .connection_id = packet.connection_id,
                .send_message_to_connection = [this, connection_id = packet.connection_id](
                                                const NetworkMessage& message) {
                    SendWebRtcNetworkMessage(connection_id, message, DeliveryMode::Unreliable);
                }
            };
            network_event_dispatcher_->ProcessNetworkMessage(connection_metadata, network_message);
        }
    }

    void AcceptWebRtcConnection(IServerTransport& transport, const ReceivedPacket& first_packet)
    {
        std::string nick{ reinterpret_cast<const char*>(first_packet.payload.data()),
                          first_packet.payload.size() };
        if (nick.empty()) {
            nick = "Web player";
        }

        Spdlog::info("[GameServer] WebRTC connection {} nick: {}",
                     first_packet.connection_id,
                     nick);

        auto [connection, inserted] =
          web_rtc_connections_.try_emplace(first_packet.connection_id,
                                           WebRtcConnection{ .connection_id =
                                                               first_packet.connection_id,
                                                            .nick = nick,
                                                            .soldier_id = std::nullopt });
        if (!inserted && connection->second.soldier_id.has_value()) {
            return;
        }

        NetworkMessage welcome_message{ NetworkEvent::ChatMessage,
                                        "Welcome to the server " + nick };
        std::span<const char> welcome_payload{ welcome_message.GetData().data(),
                                               welcome_message.GetData().size() };
        Spdlog::info("[GameServer] WebRTC send initial ChatMessage: connection={}, size={}",
                     first_packet.connection_id,
                     welcome_message.GetData().size());
        transport.Send(first_packet.connection_id, welcome_payload, DeliveryMode::Reliable);

        world_->GetStateManager()->ForEachSoldier([&](const auto& soldier) {
            std::string player_nick = "Player";
            for (const auto& [_, web_rtc_connection] : web_rtc_connections_) {
                if (web_rtc_connection.soldier_id == soldier.id) {
                    player_nick = web_rtc_connection.nick;
                    break;
                }
            }
            NetworkMessage soldier_info{ NetworkEvent::SoldierInfo, soldier.id, player_nick };
            std::span<const char> soldier_info_payload{ soldier_info.GetData().data(),
                                                        soldier_info.GetData().size() };
            Spdlog::info(
              "[GameServer] WebRTC send initial SoldierInfo: connection={}, soldier={}, size={}",
              first_packet.connection_id,
              soldier.id,
              soldier_info.GetData().size());
            transport.Send(first_packet.connection_id,
                           soldier_info_payload,
                           DeliveryMode::Reliable);
        });

        const std::uint8_t soldier_id = world_->CreateSoldier().id;
        connection->second.soldier_id = soldier_id;
        Spdlog::info("[GameServer] WebRTC AssignPlayerId: {}", soldier_id);

        NetworkMessage assign_player_message{ NetworkEvent::AssignPlayerId, soldier_id };
        std::span<const char> assign_player_payload{ assign_player_message.GetData().data(),
                                                     assign_player_message.GetData().size() };
        Spdlog::info(
          "[GameServer] WebRTC send initial AssignPlayerId: connection={}, soldier={}, size={}",
          first_packet.connection_id,
          soldier_id,
          assign_player_message.GetData().size());
        transport.Send(first_packet.connection_id, assign_player_payload, DeliveryMode::Reliable);

        NetworkMessage soldier_info_message{ NetworkEvent::SoldierInfo, soldier_id, nick };
        std::span<const char> own_soldier_info_payload{ soldier_info_message.GetData().data(),
                                                        soldier_info_message.GetData().size() };
        Spdlog::info(
          "[GameServer] WebRTC send own SoldierInfo: connection={}, soldier={}, size={}",
          first_packet.connection_id,
          soldier_id,
          soldier_info_message.GetData().size());
        transport.Send(first_packet.connection_id,
                       own_soldier_info_payload,
                       DeliveryMode::Reliable);

        SendNetworkMessageToAll(soldier_info_message);

        const auto spawn_position = world_->SpawnSoldier(soldier_id);
        NetworkMessage spawn_soldier_message{ NetworkEvent::SpawnSoldier,
                                              soldier_id,
                                              spawn_position.x,
                                              spawn_position.y };
        std::span<const char> spawn_soldier_payload{ spawn_soldier_message.GetData().data(),
                                                     spawn_soldier_message.GetData().size() };
        Spdlog::info(
          "[GameServer] WebRTC send own SpawnSoldier: connection={}, soldier={}, size={}",
          first_packet.connection_id,
          soldier_id,
          spawn_soldier_message.GetData().size());
        transport.Send(first_packet.connection_id,
                       spawn_soldier_payload,
                       DeliveryMode::Reliable);
    }
#endif
};
} // namespace Soldank
