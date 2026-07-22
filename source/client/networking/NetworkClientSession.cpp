module;

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>

export module Networking.NetworkClientSession;

import Extern.Glm;

import ClientState;
import Networking.INetworkingClient;
import Networking.InputApplicationTimeline;

import Shared.Core.IWorld;
import Shared.Core.State.StateManager;
import Shared.Core.Entities.Soldier;

import Shared.Networking.NetworkEventDispatcher;
import Shared.Networking.NetworkEvent;
import Shared.Networking.NetworkPackets;
import Shared.Networking.DeliveryMode;

import Extern.Spdlog;

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
        UpdateTargetInputDelay();

        if ((world_.GetStateManager()->GetGameTick() % 60 == 0)) {
            SendPingCheck();
        }
    }

    void SendSoldierInput(std::uint8_t soldier_id, glm::vec2 mouse_map_position)
    {
        const std::uint32_t client_tick = world_.GetStateManager()->GetGameTick();
        const auto apply_server_tick =
          input_application_timeline_.Schedule(client_tick,
                                               client_state_.network.server_tick_offset,
                                               client_state_.network.target_input_delay_ticks);
        if (!apply_server_tick.has_value()) {
            return;
        }

        client_state_.network.active_input_delay_ticks =
          input_application_timeline_.GetActiveInputDelayTicks();
        if (input_application_timeline_.WasLastScheduleResynchronized()) {
            client_state_.network.prediction_inputs.clear();
            client_state_.network.soldier_snapshot_history.clear();
            client_state_.network.reconciliation_resume_server_tick = *apply_server_tick;
            client_state_.network.input_timeline_resync_count++;
            Spdlog::info("Input timeline resynchronized at server tick {} with target delay {}",
                         *apply_server_tick,
                         client_state_.network.target_input_delay_ticks);
        }

        SoldierInputPacket update_soldier_state_packet{
            .input_sequence_id = input_sequence_id_,
            .client_tick = client_tick,
            .apply_server_tick = *apply_server_tick,
            .position_x = world_.GetSoldier(soldier_id).particle.position.x,
            .position_y = world_.GetSoldier(soldier_id).particle.position.y,
            .mouse_map_position_x = mouse_map_position.x,
            .mouse_map_position_y = mouse_map_position.y,
            .control = world_.GetSoldier(soldier_id).control
        };
        last_sent_input_sequence_id_ = input_sequence_id_;
        last_sent_input_client_tick_ = client_tick;
        last_sent_input_apply_server_tick_ = update_soldier_state_packet.apply_server_tick;
        input_sequence_id_++;
        if (client_state_.network.server_reconciliation) {
            client_state_.network.pending_inputs.push_back(update_soldier_state_packet);
            client_state_.network.prediction_inputs.push_back(update_soldier_state_packet);
        }
        networking_client_.SendNetworkMessage(
          { NetworkEvent::SoldierInput, update_soldier_state_packet }, DeliveryMode::Unreliable);
    }

    void StorePredictedSoldierSnapshot(std::uint8_t soldier_id)
    {
        if (!client_state_.network.server_reconciliation ||
            !last_sent_input_sequence_id_.has_value()) {
            return;
        }

        client_state_.network.soldier_snapshot_history.emplace_back(
          *last_sent_input_sequence_id_,
          *last_sent_input_client_tick_,
          *last_sent_input_apply_server_tick_,
          world_.GetSoldier(soldier_id));
        last_sent_input_sequence_id_.reset();
        last_sent_input_client_tick_.reset();
        last_sent_input_apply_server_tick_.reset();
    }

    void SendKillCommand()
    {
        networking_client_.SendNetworkMessage({ NetworkEvent::KillCommand },
                                              DeliveryMode::Unreliable);
    }

private:
    void UpdateTargetInputDelay()
    {
        const auto completed_round_trip_time =
          client_state_.network.ping_timer.GetLastCompletedPingMeasure();
        if (!completed_round_trip_time.has_value()) {
            return;
        }

        client_state_.network.target_input_delay_ticks =
          std::max(client_state_.network.target_input_delay_ticks,
                   CalculateInputDelayTicks(*completed_round_trip_time));
    }

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
    InputApplicationTimeline input_application_timeline_;
    unsigned int input_sequence_id_ = 1;
    std::optional<std::uint32_t> last_sent_input_sequence_id_;
    std::optional<std::uint32_t> last_sent_input_client_tick_;
    std::optional<std::uint32_t> last_sent_input_apply_server_tick_;
};
} // namespace Soldank
