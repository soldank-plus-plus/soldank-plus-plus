#include "networking/GameServer.hpp"

#include "networking/interface/NetworkingInterface.hpp"
#include "networking/poll_groups/EntryPollGroup.hpp"

#include <steam/isteamnetworkingutils.h>

#include <memory>
#include <chrono>
#include <iostream>
#include <cassert>

namespace Soldat
{
GameServer::GameServer()
{

    NetworkingInterface::Init();
    NetworkingInterface::RegisterObserver(
      [this](SteamNetConnectionStatusChangedCallback_t* p_info) {
          OnSteamNetConnectionStatusChanged(p_info);
      });

    entry_poll_group_ = NetworkingInterface::CreatePollGroup<EntryPollGroup>();
    player_poll_group_ = NetworkingInterface::CreatePollGroup<PlayerPollGroup>();
    entry_poll_group_->RegisterPlayerPollGroup(player_poll_group_);
};

void GameServer::Run()
{
    while (true) {
        entry_poll_group_->PollIncomingMessages();
        player_poll_group_->PollIncomingMessages();
        NetworkingInterface::PollConnectionStateChanges();
    }
    NetworkingInterface::Free();
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
                player_poll_group_->CloseConnection(p_info);
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
} // namespace Soldat