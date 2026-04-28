module;

#include <memory>
#include <cassert>
#include <cstdint>

export module Networking.GameServer;

import Application.ServerState;

import Networking.IGameServer;
import Networking.Interface.NetworkingInterface;
import Networking.PollGroups.EntryPollGroup;
import Networking.PollGroups.PlayerPollGroup;

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
    }
    void SendNetworkMessageToAll(const NetworkMessage& network_message) override
    {
        player_poll_group_->SendNetworkMessageToAll(network_message);
    }

    unsigned int GetSoldierIdFromConnectionId(unsigned int connection_id) override
    {
        return player_poll_group_->GetConnectionSoldierId(connection_id);
    }

private:
    std::unique_ptr<EntryPollGroup> entry_poll_group_;
    std::shared_ptr<PlayerPollGroup> player_poll_group_;

    void OnSteamNetConnectionStatusChanged(GNS::SteamNetConnectionStatusChangedCallback_t* p_info)
    {
        switch (p_info->m_info.m_eState) {
            case GNS::ESteamNetworkingConnectionState::None:
                // NOTE: We will get callbacks here when we destroy connections.  You can ignore
                // these.
                break;

            case GNS::ESteamNetworkingConnectionState::ClosedByPeer:
            case GNS::ESteamNetworkingConnectionState::ProblemDetectedLocally: {
                if (entry_poll_group_->IsConnectionAssigned(p_info->m_hConn)) {
                    entry_poll_group_->CloseConnection(p_info);
                }
                if (player_poll_group_->IsConnectionAssigned(p_info->m_hConn)) {
                    Spdlog::info("CLOSING CONNECTION conn: {}", p_info->m_hConn);
                    std::uint8_t soldier_id =
                      player_poll_group_->GetConnectionSoldierId(p_info->m_hConn);
                    Spdlog::info("CLOSING CONNECTION soldier_id: {}", soldier_id);
                    NetworkMessage network_message(NetworkEvent::PlayerLeave, soldier_id);
                    player_poll_group_->SendReliableNetworkMessageToAll(network_message,
                                                                        p_info->m_hConn);
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
};
} // namespace Soldank
