module;

#include <cstdint>
#include <memory>

export module Networking.NetworkClientSession;

import Extern.Glm;

import ClientState;
import Networking.INetworkingClient;

import Shared.Core.IWorld;
import Shared.Core.State.StateManager;
import Shared.Core.Entities.Soldier;

import Shared.Networking.NetworkEventDispatcher;
import Shared.Networking.NetworkEvent;
import Shared.Networking.NetworkPackets;
import Shared.Networking.DeliveryMode;

export namespace Soldank
{
class NetworkClientSession
{
public:
    NetworkClientSession(INetworkingClient& networking_client,
                         std::shared_ptr<NetworkEventDispatcher> network_event_dispatcher,
                         IWorld& world,
                         ClientState& client_state)
        : networking_client_(networking_client)
        , network_event_dispatcher_(std::move(network_event_dispatcher))
        , world_(world)
        , client_state_(client_state)
    {
    }

    void UpdateBeforeWorldTick()
    {
        networking_client_.SetLag(client_state_.network.network_lag);
        networking_client_.Update(network_event_dispatcher_);

        if ((world_.GetStateManager()->GetGameTick() % 60 == 0)) {
            SendPingCheck();
        }
    }

    void SendSoldierInput(std::uint8_t soldier_id, glm::vec2 mouse_map_position)
    {
        SoldierInputPacket update_soldier_state_packet{
            .input_sequence_id = input_sequence_id_,
            .game_tick = world_.GetStateManager()->GetGameTick(),
            .position_x = world_.GetSoldier(soldier_id).particle.position.x,
            .position_y = world_.GetSoldier(soldier_id).particle.position.y,
            .mouse_map_position_x = mouse_map_position.x,
            .mouse_map_position_y = mouse_map_position.y,
            .control = world_.GetSoldier(soldier_id).control
        };
        if (client_state_.network.server_reconciliation) {
            client_state_.network.soldier_snapshot_history.emplace_back(
              input_sequence_id_, world_.GetSoldier(soldier_id));
        }
        input_sequence_id_++;
        if (client_state_.network.server_reconciliation) {
            client_state_.network.pending_inputs.push_back(update_soldier_state_packet);
        }
        networking_client_.SendNetworkMessage(
          { NetworkEvent::SoldierInput, update_soldier_state_packet }, DeliveryMode::Unreliable);
    }

    void SendKillCommand()
    {
        networking_client_.SendNetworkMessage({ NetworkEvent::KillCommand },
                                              DeliveryMode::Unreliable);
    }

private:
    void SendPingCheck()
    {
        if (client_state_.network.ping_timer.IsRunning()) {
            client_state_.network.ping_timer.Update();
            if (client_state_.network.ping_timer.IsOverThreshold()) {
                networking_client_.SendNetworkMessage({ NetworkEvent::PingCheck },
                                                      DeliveryMode::Unreliable);
            }
            return;
        }

        client_state_.network.ping_timer.Start();
        networking_client_.SendNetworkMessage({ NetworkEvent::PingCheck },
                                              DeliveryMode::Unreliable);
    }

    INetworkingClient& networking_client_;
    std::shared_ptr<NetworkEventDispatcher> network_event_dispatcher_;
    IWorld& world_;
    ClientState& client_state_;
    unsigned int input_sequence_id_ = 1;
};
} // namespace Soldank
