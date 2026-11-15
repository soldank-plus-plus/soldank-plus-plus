module;

#include <memory>
#include <cassert>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

export module Networking.GameServer;

import Networking.IGameServer;
import Networking.ChatSendRequestNetworkEventHandler;
import Networking.Interface.NetworkingInterface;
import Networking.PollGroups.EntryPollGroup;
import Networking.PollGroups.PlayerPollGroup;
import Networking.Transport.GnsServerTransport;
import Networking.Transport.TransportTypes;
import Sessions.PlayerSessionManager;
#ifdef SOLDANK_ENABLE_WEBRTC_SERVER_TRANSPORT
import Networking.Transport.IServerTransport;
import Networking.Transport.WebRtcServerTransport;
#endif

import Shared.Core.IWorld;

import Shared.Networking.NetworkMessage;
import Shared.Networking.NetworkEventDispatcher;
import Shared.Networking.NetworkEvent;
import Shared.Networking.ClientHello;
import Shared.Networking.GameModeProtocol;

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
               PlayerSessionManager& player_session_manager)
        : world_(world)
        , player_session_manager_(player_session_manager)
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
        chat_service_ = std::make_unique<ChatService>(
          [this] { return GetChatPeers(); },
          [this](unsigned int connection, const NetworkMessage& message) {
              SendChatMessage(connection, message);
          });
        network_event_dispatcher_->AddNetworkEventHandler(
          std::make_shared<ChatSendRequestNetworkEventHandler>(*chat_service_));
        player_poll_group_->SetChatJoinHandler(
          [this](unsigned int connection) { WelcomeChatPlayer(connection); });

#ifdef SOLDANK_ENABLE_WEBRTC_SERVER_TRANSPORT
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
#ifdef SOLDANK_ENABLE_WEBRTC_SERVER_TRANSPORT
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
#ifdef SOLDANK_ENABLE_WEBRTC_SERVER_TRANSPORT
        if (web_rtc_connections_.contains(connection_id)) {
            SendWebRtcNetworkMessage(connection_id, network_message, DeliveryMode::Unreliable);
        }
#endif
    }
    void SendNetworkMessageToAll(const NetworkMessage& network_message) override
    {
        const auto event = network_message.GetNetworkEvent();
        const bool reliable =
          event &&
          (*event == NetworkEvent::SpawnSoldier || *event == NetworkEvent::ProjectileSpawn ||
           *event == NetworkEvent::ProjectilePhysics || *event == NetworkEvent::HitSoldier ||
           *event == NetworkEvent::KillSoldier || *event == NetworkEvent::ItemState ||
           *event == NetworkEvent::MatchState);
        if (reliable) {
            player_poll_group_->SendReliableNetworkMessageToAll(network_message);
        } else {
            player_poll_group_->SendNetworkMessageToAll(network_message);
        }
#ifdef SOLDANK_ENABLE_WEBRTC_SERVER_TRANSPORT
        for (const auto& [connection_id, connection] : web_rtc_connections_) {
            if (connection.soldier_id.has_value()) {
                SendWebRtcNetworkMessage(connection_id,
                                         network_message,
                                         reliable ? DeliveryMode::Reliable
                                                  : DeliveryMode::Unreliable);
            }
        }
#endif
    }

    unsigned int GetSoldierIdFromConnectionId(unsigned int connection_id) override
    {
#ifdef SOLDANK_ENABLE_WEBRTC_SERVER_TRANSPORT
        const auto web_rtc_connection = web_rtc_connections_.find(connection_id);
        if (web_rtc_connection != web_rtc_connections_.end() &&
            web_rtc_connection->second.soldier_id.has_value()) {
            return *web_rtc_connection->second.soldier_id;
        }
#endif
        return player_poll_group_->GetConnectionSoldierId(connection_id);
    }

    std::vector<ChatPeer> GetChatPeers() const override
    {
        std::vector<ChatPeer> peers;
        const auto add =
          [&](unsigned int connection, unsigned int soldier, const std::string& name) {
              const auto state = world_->GetStateManager();
              const auto session = chat_sessions_.find(connection);
              if (session == chat_sessions_.end()) {
                  return;
              }
              const auto* mode = state->GetGameMode();
              const auto team = mode != nullptr && mode->Settings().team_count > 0
                                  ? state->GetSoldier(soldier).team
                                  : TeamType::None;
              peers.push_back({
                .connection_id = connection,
                .session = session->second,
                .team = team,
                .name = name,
              });
          };
        for (const auto& connection : player_poll_group_->GetConnections()) {
            add(connection.connection_id, connection.soldier_id, connection.nick);
        }
#ifdef SOLDANK_ENABLE_WEBRTC_SERVER_TRANSPORT
        for (const auto& [id, connection] : web_rtc_connections_) {
            if (connection.soldier_id) {
                add(id, *connection.soldier_id, connection.nick);
            }
        }
#endif
        return peers;
    }
    bool BroadcastSystemMessage(std::string_view text) override
    {
        return chat_service_->BroadcastSystemMessage(text);
    }
    bool SendSystemMessage(std::span<const ChatPeer> recipients, std::string_view text) override
    {
        return chat_service_->SendSystemMessage(recipients, text);
    }

private:
    void SendChatMessage(unsigned int connection, const NetworkMessage& message)
    {
        if (player_poll_group_->IsConnectionAssigned(connection)) {
            player_poll_group_->SendReliableNetworkMessage(connection, message);
        }
#ifdef SOLDANK_ENABLE_WEBRTC_SERVER_TRANSPORT
        if (web_rtc_connections_.contains(connection)) {
            SendWebRtcNetworkMessage(connection, message, DeliveryMode::Reliable);
        }
#endif
    }
    void WelcomeChatPlayer(unsigned int connection)
    {
        chat_sessions_[connection] = next_chat_session_++;
        for (const auto& peer : GetChatPeers()) {
            if (peer.connection_id == connection) {
                chat_service_->SendSystemMessage(std::span(&peer, 1),
                                                 "Welcome to the server " + peer.name);
                break;
            }
        }
    }
    std::unordered_map<unsigned int, std::uint64_t> chat_sessions_;
    std::uint64_t next_chat_session_{ 1 };
    std::unique_ptr<ChatService> chat_service_;
    std::unique_ptr<EntryPollGroup> entry_poll_group_;
    std::shared_ptr<PlayerPollGroup> player_poll_group_;
#ifdef SOLDANK_ENABLE_WEBRTC_SERVER_TRANSPORT
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
                    chat_sessions_.erase(connection_id);
                    player_poll_group_->CloseConnection(p_info);

                    Spdlog::info("CLOSING CONNECTION soldiers size before: {}",
                                 world_->GetStateManager()->GetSoldiersCount());
                    world_->GetStateManager()->RemoveSoldier(soldier_id);

                    Spdlog::info("CLOSING CONNECTION soldiers size after: {}",
                                 world_->GetStateManager()->GetSoldiersCount());

                    player_session_manager_.ClearSoldier(soldier_id);
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
    PlayerSessionManager& player_session_manager_;
    std::shared_ptr<NetworkEventDispatcher> network_event_dispatcher_;

#ifdef SOLDANK_ENABLE_WEBRTC_SERVER_TRANSPORT
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
                .send_message_to_connection =
                  [this, connection_id = packet.connection_id](const NetworkMessage& message) {
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
        const auto parsed_nick = ParseClientHello(nick);
        if (!parsed_nick) {
            transport.Close(first_packet.connection_id);
            return;
        }
        nick = *parsed_nick;

        Spdlog::info(
          "[GameServer] WebRTC connection {} nick: {}", first_packet.connection_id, nick);

        auto [connection, inserted] = web_rtc_connections_.try_emplace(
          first_packet.connection_id,
          WebRtcConnection{ .connection_id = first_packet.connection_id,
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
            transport.Send(
              first_packet.connection_id, soldier_info_payload, DeliveryMode::Reliable);
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
        Spdlog::info("[GameServer] WebRTC send own SoldierInfo: connection={}, soldier={}, size={}",
                     first_packet.connection_id,
                     soldier_id,
                     soldier_info_message.GetData().size());
        transport.Send(
          first_packet.connection_id, own_soldier_info_payload, DeliveryMode::Reliable);

        SendNetworkMessageToAll(soldier_info_message);

        WelcomeChatPlayer(first_packet.connection_id);
        const auto spawn_position = world_->SpawnSoldier(soldier_id);
        NetworkMessage spawn_soldier_message{
            NetworkEvent::SpawnSoldier, soldier_id, spawn_position.x, spawn_position.y
        };
        std::span<const char> spawn_soldier_payload{ spawn_soldier_message.GetData().data(),
                                                     spawn_soldier_message.GetData().size() };
        Spdlog::info(
          "[GameServer] WebRTC send own SpawnSoldier: connection={}, soldier={}, size={}",
          first_packet.connection_id,
          soldier_id,
          spawn_soldier_message.GetData().size());
        transport.Send(first_packet.connection_id, spawn_soldier_payload, DeliveryMode::Reliable);
        if (world_->GetStateManager()->GetGameMode()) {
            const NetworkMessage match_message{ NetworkEvent::MatchState,
                                                MakeMatchStatePacket(*world_->GetStateManager()) };
            transport.Send(
              first_packet.connection_id, match_message.GetData(), DeliveryMode::Reliable);
        }
    }
#endif
};
} // namespace Soldank
