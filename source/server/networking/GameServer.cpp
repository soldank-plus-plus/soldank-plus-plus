#include "networking/GameServer.hpp"

#include "networking/interface/NetworkingInterface.hpp"
#include "networking/poll_groups/EntryPollGroup.hpp"

#include <steam/isteamnetworkingutils.h>

#include "spdlog/spdlog.h"

#include <memory>
#include <chrono>
#include <cassert>

namespace Soldank
{
GameServer::GameServer(std::uint16_t port,
                       const std::shared_ptr<NetworkEventDispatcher>& network_event_dispatcher,
                       const std::shared_ptr<IWorld>& world,
                       const std::shared_ptr<ServerState>& server_state)
    : world_(world)
    , server_state_(server_state)
{
    // TODO: we shouldn't init NetworkingInterface here because it's global
    NetworkingInterface::Init(port);
    NetworkingInterface::RegisterObserver(
      [this](SteamNetConnectionStatusChangedCallback_t* p_info) {
          OnSteamNetConnectionStatusChanged(p_info);
      });

    entry_poll_group_ = NetworkingInterface::CreatePollGroup<EntryPollGroup>();
    player_poll_group_ = NetworkingInterface::CreatePollGroup<PlayerPollGroup>();
    entry_poll_group_->RegisterPlayerPollGroup(player_poll_group_);
    player_poll_group_->SetServerNetworkEventDispatcher(network_event_dispatcher);
    player_poll_group_->SetWorld(world);
};

GameServer::~GameServer()
{
    NetworkingInterface::Free();
}

void GameServer::Update()
{
    entry_poll_group_->PollIncomingMessages();
    player_poll_group_->PollIncomingMessages();
    NetworkingInterface::PollConnectionStateChanges();
}

void GameServer::SendNetworkMessage(unsigned int connection_id,
                                    const NetworkMessage& network_message)
{
    if (entry_poll_group_->IsConnectionAssigned(connection_id)) {
        entry_poll_group_->SendNetworkMessage(connection_id, network_message);
    }
    if (player_poll_group_->IsConnectionAssigned(connection_id)) {
        player_poll_group_->SendNetworkMessage(connection_id, network_message);
    }
}

void GameServer::SendNetworkMessageToAll(const NetworkMessage& network_message)
{
    player_poll_group_->SendNetworkMessageToAll(network_message);
}

unsigned int GameServer::GetSoldierIdFromConnectionId(unsigned int connection_id)
{
    return player_poll_group_->GetConnectionSoldierId(connection_id);
}

void GameServer::OnSteamNetConnectionStatusChanged(
  SteamNetConnectionStatusChangedCallback_t* p_info)
{
    switch (p_info->m_info.m_eState) {
        case k_ESteamNetworkingConnectionState_None:
            // NOTE: We will get callbacks here when we destroy connections.  You can ignore these.
            break;

        case k_ESteamNetworkingConnectionState_ClosedByPeer:
        case k_ESteamNetworkingConnectionState_ProblemDetectedLocally: {
            if (entry_poll_group_->IsConnectionAssigned(p_info->m_hConn)) {
                entry_poll_group_->CloseConnection(p_info);
            }
            if (player_poll_group_->IsConnectionAssigned(p_info->m_hConn)) {
                spdlog::info("CLOSING CONNECTION conn: {}", p_info->m_hConn);
                std::uint8_t soldier_id =
                  player_poll_group_->GetConnectionSoldierId(p_info->m_hConn);
                spdlog::info("CLOSING CONNECTION soldier_id: {}", soldier_id);
                NetworkMessage network_message(NetworkEvent::PlayerLeave, soldier_id);
                player_poll_group_->SendReliableNetworkMessageToAll(network_message,
                                                                    p_info->m_hConn);
                player_poll_group_->CloseConnection(p_info);

                spdlog::info("CLOSING CONNECTION soldiers size before: {}",
                             world_->GetStateManager()->GetSoldiersCount());
                world_->GetStateManager()->RemoveSoldier(soldier_id);

                spdlog::info("CLOSING CONNECTION soldiers size after: {}",
                             world_->GetStateManager()->GetSoldiersCount());

                server_state_->last_processed_input_id.at(soldier_id) = 0;
            }
            break;
        }

        case k_ESteamNetworkingConnectionState_Connecting: {
            entry_poll_group_->AcceptConnection(p_info);
            // player_poll_group_->AcceptConnection(p_info);
            break;
        }

        case k_ESteamNetworkingConnectionState_Connected:
            // We will get a callback immediately after accepting the connection.
            // Since we are the server, we can ignore this, it's not news to us.

        default:
            // Silences -Wswitch
            break;
    }
}
} // namespace Soldank
