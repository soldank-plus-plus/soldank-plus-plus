module;

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <list>
#include <memory>
#include <optional>

export module Networking.SoldierStateNetworkEventHandler;

import Extern.Glm;

import ClientState;
import Networking.ReconciliationTimeline;

import Shared.Core.IWorld;
import Shared.Core.Config.Config;

import Shared.Networking.NetworkEventDispatcher;
import Shared.Networking.NetworkPackets;
import Shared.Networking.NetworkEvent;

import Shared.Core.Physics.SoldierSkeletonPhysics;
import Shared.Core.Animations;
import Shared.Core.Entities.Soldier;
import Shared.Core.Simulation.PlayerInputApplication;
import Shared.Core.State.Control;
import Shared.Core.Utility.SerialNumber;
import Shared.Networking.ProtocolConversions;

export namespace Soldank
{
class SoldierStateNetworkEventHandler : public NetworkEventHandlerBase<SoldierStatePacket>
{
public:
    SoldierStateNetworkEventHandler(const std::shared_ptr<IWorld>& world,
                                    const std::shared_ptr<ClientState>& client_state)
        : world_(world)
        , client_state_(client_state)
    {
#ifndef NDEBUG
        WriteClientTimelineLogHeader();
#endif
    }

private:
    NetworkEvent GetTargetNetworkEvent() const override { return NetworkEvent::SoldierState; }

    NetworkEventHandlerResult HandleNetworkMessageImpl(
      unsigned int /*sender_connection_id*/,
      SoldierStatePacket soldier_state_packet) override
    {
        std::uint8_t soldier_id = soldier_state_packet.player_id;
        std::optional<std::uint32_t>& latest_server_tick = latest_server_ticks_.at(soldier_id);
        if (latest_server_tick.has_value() &&
            !IsSerialNumberNewer(soldier_state_packet.server_tick, *latest_server_tick)) {
            return NetworkEventHandlerResult::Success;
        }
        latest_server_tick = soldier_state_packet.server_tick;

        const std::uint32_t local_tick = world_->GetStateManager()->GetGameTick();
        client_state_->network.server_tick_offset =
          SerialNumberSignedDistance(local_tick, soldier_state_packet.server_tick);

        glm::vec2 soldier_position = { soldier_state_packet.position_x,
                                       soldier_state_packet.position_y };
        glm::vec2 soldier_old_position = { soldier_state_packet.old_position_x,
                                           soldier_state_packet.old_position_y };
        AnimationType body_animation_type = soldier_state_packet.body_animation_type;
        std::uint32_t body_animation_frame = soldier_state_packet.body_animation_frame;
        std::int32_t body_animation_speed = soldier_state_packet.body_animation_speed;
        std::int32_t body_animation_count = soldier_state_packet.body_animation_count;
        AnimationType legs_animation_type = soldier_state_packet.legs_animation_type;
        std::uint32_t legs_animation_frame = soldier_state_packet.legs_animation_frame;
        std::int32_t legs_animation_speed = soldier_state_packet.legs_animation_speed;
        std::int32_t legs_animation_count = soldier_state_packet.legs_animation_count;
        glm::vec2 soldier_velocity = { soldier_state_packet.velocity_x,
                                       soldier_state_packet.velocity_y };
        glm::vec2 soldier_force = { soldier_state_packet.force_x, soldier_state_packet.force_y };
        bool on_ground = soldier_state_packet.on_ground;
        bool on_ground_for_law = soldier_state_packet.on_ground_for_law;
        bool on_ground_last_frame = soldier_state_packet.on_ground_last_frame;
        bool on_ground_permanent = soldier_state_packet.on_ground_permanent;
        std::int8_t old_direction = soldier_state_packet.old_direction;
        std::uint8_t stance = soldier_state_packet.stance;
        float mouse_map_position_x = soldier_state_packet.mouse_map_position_x;
        float mouse_map_position_y = soldier_state_packet.mouse_map_position_y;
        bool using_jets = soldier_state_packet.using_jets;
        std::int32_t jets_count = soldier_state_packet.jets_count;
        std::uint8_t active_weapon = soldier_state_packet.active_weapon;
        std::uint32_t last_applied_input_id = soldier_state_packet.last_applied_input_id;

        bool is_soldier_id_me = false;
        if (client_state_->client_soldier_id.has_value()) {
            is_soldier_id_me = *client_state_->client_soldier_id == soldier_id;
        }
        if (is_soldier_id_me) {
            client_state_->network.soldier_position_server_pov = { soldier_position.x,
                                                                   soldier_position.y };
        }

#ifndef NDEBUG
        const glm::vec2 local_position_before_reconciliation =
          is_soldier_id_me ? world_->GetSoldier(soldier_id).particle.position : glm::vec2{};
        std::optional<ClientNetworkState::PredictedSoldierSnapshot> diagnostic_snapshot;
        if (is_soldier_id_me && client_state_->network.client_side_prediction &&
            client_state_->network.server_reconciliation) {
            const auto snapshot = FindPredictedSnapshot(soldier_state_packet.server_tick);
            if (snapshot != client_state_->network.soldier_snapshot_history.end()) {
                diagnostic_snapshot = *snapshot;
            }
        }
#endif

        const bool awaiting_timeline_resynchronization =
          is_soldier_id_me && client_state_->network.client_side_prediction &&
          client_state_->network.server_reconciliation &&
          client_state_->network.reconciliation_resume_server_tick.has_value() &&
          IsSerialNumberOlder(soldier_state_packet.server_tick,
                              *client_state_->network.reconciliation_resume_server_tick);
        if (awaiting_timeline_resynchronization) {
#ifndef NDEBUG
            LogClientTimelineSnapshot(soldier_state_packet,
                                      local_tick,
                                      diagnostic_snapshot,
                                      "timeline_resync_wait",
                                      {},
                                      local_position_before_reconciliation,
                                      local_position_before_reconciliation);
#endif
            RemoveAcknowledgedNetworkInputs(last_applied_input_id);
            PrunePredictionHistory(soldier_state_packet.server_tick);
            return NetworkEventHandlerResult::Success;
        }
        if (is_soldier_id_me &&
            client_state_->network.reconciliation_resume_server_tick.has_value() &&
            !IsSerialNumberOlder(soldier_state_packet.server_tick,
                                 *client_state_->network.reconciliation_resume_server_tick)) {
            client_state_->network.reconciliation_resume_server_tick.reset();
        }

        const bool predicted_state_accurate =
          IsPredictedStateAccurate(soldier_state_packet.server_tick,
                                   soldier_position,
                                   soldier_velocity,
                                   on_ground,
                                   stance,
                                   is_soldier_id_me);
        if (predicted_state_accurate) {
#ifndef NDEBUG
            if (is_soldier_id_me) {
                LogClientTimelineSnapshot(soldier_state_packet,
                                          local_tick,
                                          diagnostic_snapshot,
                                          "accurate",
                                          {},
                                          local_position_before_reconciliation,
                                          local_position_before_reconciliation);
            }
#endif
            RemoveAcknowledgedNetworkInputs(last_applied_input_id);
            PrunePredictionHistory(soldier_state_packet.server_tick);
            return NetworkEventHandlerResult::Success;
        }

        if (is_soldier_id_me && client_state_->network.client_side_prediction &&
            client_state_->network.server_reconciliation) {
            LogLocalCorrection(soldier_state_packet.server_tick,
                               last_applied_input_id,
                               soldier_position,
                               soldier_velocity,
                               on_ground,
                               stance,
                               jets_count,
                               using_jets);
        }

#ifndef NDEBUG
        if (is_soldier_id_me && client_state_->network.client_side_prediction &&
            client_state_->network.server_reconciliation) {
            const float correction_distance =
              glm::distance(world_->GetSoldier(soldier_id).particle.position, soldier_position);
            client_state_->network.last_local_correction_distance = correction_distance;
            client_state_->network.maximum_local_correction_distance = std::max(
              client_state_->network.maximum_local_correction_distance, correction_distance);
            client_state_->network.local_correction_count++;
        }
#endif

        world_->GetStateManager()->TransformSoldier(soldier_id, [&](auto& soldier) {
            soldier.particle.old_position = soldier_old_position;
            soldier.particle.position = soldier_position;
            soldier.particle.SetVelocity(soldier_velocity);
            soldier.particle.SetForce(soldier_force);

            soldier.on_ground = on_ground;
            soldier.on_ground_for_law = on_ground_for_law;
            soldier.on_ground_last_frame = on_ground_last_frame;
            soldier.on_ground_permanent = on_ground_permanent;
            soldier.old_direction = old_direction;

            soldier.stance = stance;

            // TODO: make mouse position not game width and game height dependent
            soldier.game_width = 640.0;
            soldier.game_height = 480.0;
            world_->GetStateManager()->ChangeSoldierMouseMapPosition(
              soldier_id, { mouse_map_position_x, mouse_map_position_y });

            if ((float)soldier.control.mouse_aim_x >= soldier.particle.position.x) {
                soldier.direction = 1;
            } else {
                soldier.direction = -1;
            }

            // TODO: there is a visual bug with feet when another soldier is using jets and going
            // backwards
            soldier.control.jets = using_jets;
            soldier.jets_count = jets_count;

            soldier.active_weapon = active_weapon;

            if (soldier.body_animation->GetType() != body_animation_type) {
                AnimationState::ExitParams body_exit_params{ false };
                soldier.body_animation->Exit(body_exit_params);
                if (body_exit_params.should_throw_active_weapon) {
                    world_->GetPhysicsEvents().soldier_throws_active_weapon.Notify(soldier);
                }
                soldier.body_animation = world_->GetBodyAnimationState(body_animation_type);
                AnimationState::EnterParams body_enter_params{
                    soldier.on_ground,         soldier.direction, soldier.particle.GetForce(),
                    soldier.grenade_can_throw, soldier.weapons,   soldier.active_weapon
                };
                soldier.body_animation->Enter(body_enter_params);
                soldier.grenade_can_throw = body_enter_params.grenade_can_throw;
            }
            soldier.body_animation->SetFrame(body_animation_frame);
            soldier.body_animation->SetSpeed(body_animation_speed);
            soldier.body_animation->SetCount(body_animation_count);

            if (soldier.legs_animation->GetType() != legs_animation_type) {
                AnimationState::ExitParams legs_exit_params{ false };
                soldier.legs_animation->Exit(legs_exit_params);
                if (legs_exit_params.should_throw_active_weapon) {
                    world_->GetPhysicsEvents().soldier_throws_active_weapon.Notify(soldier);
                }
                soldier.legs_animation = world_->GetLegsAnimationState(legs_animation_type);
                AnimationState::EnterParams legs_enter_params{
                    soldier.on_ground,         soldier.direction, soldier.particle.GetForce(),
                    soldier.grenade_can_throw, soldier.weapons,   soldier.active_weapon
                };
                soldier.legs_animation->Enter(legs_enter_params);
                soldier.grenade_can_throw = legs_enter_params.grenade_can_throw;
            }
            soldier.legs_animation->SetFrame(legs_animation_frame);
            soldier.legs_animation->SetSpeed(legs_animation_speed);
            soldier.legs_animation->SetCount(legs_animation_count);

            // The snapshot's force is authoritative. Animation entry may calculate a temporary
            // transition force, but it must not overwrite the replicated simulation state.
            soldier.particle.SetForce(soldier_force);

            RepositionSoldierSkeletonParts(soldier);

            // TODO: Figure out if the below section is needed, I have a hunch that it is not
            if (soldier.dead_meat) {
                soldier.skeleton->DoVerletTimestep();
                soldier.particle.position = soldier.skeleton->GetPos(12);
                // CheckSkeletonOutOfBounds;
            }
        });

        ReplayDiagnostics replay_diagnostics;
        if (client_state_->network.server_reconciliation && is_soldier_id_me) {
            RemoveAcknowledgedNetworkInputs(last_applied_input_id);
            replay_diagnostics =
              ReplayPredictedInputsAfterTick(soldier_id, soldier_state_packet.server_tick);
            PrunePredictionHistory(soldier_state_packet.server_tick);
        }

#ifndef NDEBUG
        if (is_soldier_id_me) {
            const bool reconciliation_enabled = client_state_->network.client_side_prediction &&
                                                client_state_->network.server_reconciliation;
            const char* decision =
              reconciliation_enabled
                ? (diagnostic_snapshot.has_value() ? "correction" : "timeline_missing")
                : "authoritative";
            LogClientTimelineSnapshot(soldier_state_packet,
                                      local_tick,
                                      diagnostic_snapshot,
                                      decision,
                                      replay_diagnostics,
                                      local_position_before_reconciliation,
                                      world_->GetSoldier(soldier_id).particle.position);
        }
#endif

        return NetworkEventHandlerResult::Success;
    }

    bool IsPredictedStateAccurate(std::uint32_t server_tick,
                                  const glm::vec2& server_position,
                                  const glm::vec2& server_velocity,
                                  bool server_on_ground,
                                  std::uint8_t server_stance,
                                  bool is_soldier_id_me) const
    {
        if (!is_soldier_id_me || !client_state_->network.client_side_prediction ||
            !client_state_->network.server_reconciliation) {
            return false;
        }

        const auto snapshot = FindPredictedSnapshot(server_tick);
        if (snapshot == client_state_->network.soldier_snapshot_history.end()) {
            return false;
        }

        const float position_difference =
          glm::distance(snapshot->soldier_snapshot.GetPosition(), server_position);
        const float velocity_difference =
          glm::distance(snapshot->soldier_snapshot.GetVelocity(), server_velocity);
        if (position_difference > POSITION_RECONCILIATION_EPSILON ||
            velocity_difference > VELOCITY_RECONCILIATION_EPSILON) {
            return false;
        }

        if (snapshot->soldier_snapshot.IsOnGround() == server_on_ground &&
            snapshot->soldier_snapshot.GetStance() == server_stance) {
            return true;
        }

        // Ground contact and stance can differ for one frame at an edge or landing. Do not reset
        // an otherwise stationary prediction for that transient discrepancy.
        return position_difference <= STATE_RECONCILIATION_POSITION_EPSILON;
    }

    std::list<ClientNetworkState::PredictedSoldierSnapshot>::const_iterator FindPredictedSnapshot(
      std::uint32_t server_tick) const
    {
        return std::find_if(client_state_->network.soldier_snapshot_history.begin(),
                            client_state_->network.soldier_snapshot_history.end(),
                            [server_tick](const auto& predicted_snapshot) {
                                return predicted_snapshot.apply_server_tick == server_tick;
                            });
    }

    void LogLocalCorrection(std::uint32_t server_tick,
                            std::uint32_t last_applied_input_id,
                            const glm::vec2& server_position,
                            const glm::vec2& server_velocity,
                            bool server_on_ground,
                            std::uint8_t server_stance,
                            std::int32_t server_jets_count,
                            bool server_using_jets)
    {
        const auto snapshot = FindPredictedSnapshot(server_tick);
        reconciliation_log_ << "server_tick=" << server_tick
                            << " last_applied_input=" << last_applied_input_id;
        if (snapshot == client_state_->network.soldier_snapshot_history.end()) {
            reconciliation_log_ << " snapshot=missing\n";
            reconciliation_log_.flush();
            return;
        }

        const SoldierSnapshot& predicted = snapshot->soldier_snapshot;
        reconciliation_log_
          << " predicted_input=" << snapshot->input_sequence_id
          << " predicted_client_tick=" << snapshot->client_tick
          << " position_delta=" << glm::distance(predicted.GetPosition(), server_position)
          << " velocity_delta=" << glm::distance(predicted.GetVelocity(), server_velocity)
          << " predicted_position=(" << predicted.GetPosition().x << ','
          << predicted.GetPosition().y << ") server_position=(" << server_position.x << ','
          << server_position.y << ')' << " predicted_velocity=(" << predicted.GetVelocity().x << ','
          << predicted.GetVelocity().y << ") server_velocity=(" << server_velocity.x << ','
          << server_velocity.y << ')' << " on_ground=" << predicted.IsOnGround() << '/'
          << server_on_ground << " stance=" << static_cast<int>(predicted.GetStance()) << '/'
          << static_cast<int>(server_stance) << " jets_count=" << predicted.GetJetsCount() << '/'
          << server_jets_count << " using_jets=" << predicted.IsUsingJets() << '/'
          << server_using_jets << '\n';
        reconciliation_log_.flush();
    }

    struct ReplayDiagnostics
    {
        std::size_t count = 0;
        std::optional<std::uint32_t> first_apply_server_tick;
        std::optional<std::uint32_t> last_apply_server_tick;
    };

    ReplayDiagnostics ReplayPredictedInputsAfterTick(std::uint8_t soldier_id,
                                                     std::uint32_t authoritative_server_tick)
    {
        ReplayDiagnostics diagnostics;
        auto& snapshot_history = client_state_->network.soldier_snapshot_history;
        snapshot_history.remove_if([authoritative_server_tick](const auto& snapshot) {
            return IsSerialNumberNewer(snapshot.apply_server_tick, authoritative_server_tick);
        });

        const auto replay_inputs =
          SelectReplayInputs(client_state_->network.prediction_inputs, authoritative_server_tick);
        for (const auto& input : replay_inputs) {
            const auto player_input = ProtocolConversions::ToPlayerInputCommand(soldier_id, input);
            ApplyPlayerInputCommand(*world_->GetStateManager(), player_input);
            world_->UpdateSoldier(soldier_id);

            snapshot_history.emplace_back(input.input_sequence_id,
                                          input.client_tick,
                                          input.apply_server_tick,
                                          world_->GetSoldier(soldier_id));

            diagnostics.count++;
            if (!diagnostics.first_apply_server_tick.has_value()) {
                diagnostics.first_apply_server_tick = input.apply_server_tick;
            }
            diagnostics.last_apply_server_tick = input.apply_server_tick;
        }

        return diagnostics;
    }

#ifndef NDEBUG
    void WriteClientTimelineLogHeader()
    {
        client_timeline_log_
          << "receive_local_tick,server_tick,player_id,last_applied_input_id,raw_offset_sample,"
             "round_trip_time_ms,target_input_delay_ticks,active_input_delay_ticks,"
             "input_timeline_resync_count,reconciliation_resume_server_tick,"
             "predicted_found,predicted_input_id,predicted_client_tick,"
             "predicted_apply_server_tick,exact_timeline_match,decision,position_delta,"
             "velocity_delta,replay_count,replay_first_apply_tick,replay_last_apply_tick,"
             "local_before_x,local_before_y,local_after_x,local_after_y,"
             "predicted_position_x,predicted_position_y,server_position_x,server_position_y,"
             "predicted_old_position_x,predicted_old_position_y,server_old_position_x,"
             "server_old_position_y,predicted_velocity_x,predicted_velocity_y,"
             "server_velocity_x,server_velocity_y,predicted_force_x,predicted_force_y,"
             "server_force_x,server_force_y,predicted_on_ground,server_on_ground,"
             "predicted_on_ground_for_law,server_on_ground_for_law,"
             "predicted_on_ground_last_frame,server_on_ground_last_frame,"
             "predicted_on_ground_permanent,server_on_ground_permanent,predicted_stance,"
             "server_stance,predicted_direction,predicted_old_direction,server_old_direction,"
             "predicted_jets_count,server_jets_count,predicted_using_jets,server_using_jets,"
             "predicted_body_animation_type,server_body_animation_type,"
             "predicted_body_animation_frame,server_body_animation_frame,"
             "predicted_body_animation_speed,server_body_animation_speed,"
             "predicted_body_animation_count,server_body_animation_count,"
             "predicted_legs_animation_type,server_legs_animation_type,"
             "predicted_legs_animation_frame,server_legs_animation_frame,"
             "predicted_legs_animation_speed,server_legs_animation_speed,"
             "predicted_legs_animation_count,server_legs_animation_count,"
             "predicted_grenade_can_throw,predicted_control_left,predicted_control_right,"
             "predicted_control_up,predicted_control_down,predicted_control_fire,"
             "predicted_control_jets,predicted_control_change,predicted_control_throw_grenade,"
             "predicted_control_drop,predicted_control_reload,predicted_control_prone,"
             "predicted_control_flag_throw,predicted_control_mouse_aim_x,"
             "predicted_control_mouse_aim_y,predicted_control_mouse_dist,"
             "predicted_control_was_running_left,predicted_control_was_jumping,"
             "predicted_control_was_throwing_weapon,predicted_control_was_changing_weapon,"
             "predicted_control_was_throwing_grenade,predicted_control_was_reloading_weapon,"
             "server_mouse_aim_x,server_mouse_aim_y\n";
        client_timeline_log_.flush();
    }

    void LogClientTimelineSnapshot(
      const SoldierStatePacket& packet,
      std::int64_t local_tick,
      const std::optional<ClientNetworkState::PredictedSoldierSnapshot>& predicted_snapshot,
      const char* decision,
      const ReplayDiagnostics& replay_diagnostics,
      const glm::vec2& local_position_before_reconciliation,
      const glm::vec2& local_position_after_reconciliation)
    {
        const SoldierSnapshot* predicted =
          predicted_snapshot.has_value() ? &predicted_snapshot->soldier_snapshot : nullptr;
        const Control* predicted_control =
          predicted != nullptr ? &predicted->GetControl() : nullptr;
        const glm::vec2 server_position{ packet.position_x, packet.position_y };
        const glm::vec2 server_velocity{ packet.velocity_x, packet.velocity_y };
        const float position_delta =
          predicted != nullptr ? glm::distance(predicted->GetPosition(), server_position) : 0.0F;
        const float velocity_delta =
          predicted != nullptr ? glm::distance(predicted->GetVelocity(), server_velocity) : 0.0F;
        const std::int64_t raw_offset_sample =
          SerialNumberSignedDistance(static_cast<std::uint32_t>(local_tick), packet.server_tick);
        const auto completed_round_trip_time =
          client_state_->network.ping_timer.GetLastCompletedPingMeasure();

        client_timeline_log_
          << local_tick << ',' << packet.server_tick << ',' << static_cast<int>(packet.player_id)
          << ',' << packet.last_applied_input_id << ',' << raw_offset_sample << ','
          << completed_round_trip_time.value_or(0) << ','
          << client_state_->network.target_input_delay_ticks << ','
          << client_state_->network.active_input_delay_ticks << ','
          << client_state_->network.input_timeline_resync_count << ','
          << client_state_->network.reconciliation_resume_server_tick.value_or(0) << ','
          << predicted_snapshot.has_value() << ','
          << (predicted_snapshot.has_value() ? predicted_snapshot->input_sequence_id : 0) << ','
          << (predicted_snapshot.has_value() ? predicted_snapshot->client_tick : 0) << ','
          << (predicted_snapshot.has_value() ? predicted_snapshot->apply_server_tick : 0) << ','
          << (predicted_snapshot.has_value() &&
              predicted_snapshot->apply_server_tick == packet.server_tick)
          << ',' << decision << ',' << position_delta << ',' << velocity_delta << ','
          << replay_diagnostics.count << ','
          << replay_diagnostics.first_apply_server_tick.value_or(0) << ','
          << replay_diagnostics.last_apply_server_tick.value_or(0) << ','
          << local_position_before_reconciliation.x << ',' << local_position_before_reconciliation.y
          << ',' << local_position_after_reconciliation.x << ','
          << local_position_after_reconciliation.y << ','
          << (predicted != nullptr ? predicted->GetPosition().x : 0.0F) << ','
          << (predicted != nullptr ? predicted->GetPosition().y : 0.0F) << ',' << packet.position_x
          << ',' << packet.position_y << ','
          << (predicted != nullptr ? predicted->GetOldPosition().x : 0.0F) << ','
          << (predicted != nullptr ? predicted->GetOldPosition().y : 0.0F) << ','
          << packet.old_position_x << ',' << packet.old_position_y << ','
          << (predicted != nullptr ? predicted->GetVelocity().x : 0.0F) << ','
          << (predicted != nullptr ? predicted->GetVelocity().y : 0.0F) << ',' << packet.velocity_x
          << ',' << packet.velocity_y << ','
          << (predicted != nullptr ? predicted->GetForce().x : 0.0F) << ','
          << (predicted != nullptr ? predicted->GetForce().y : 0.0F) << ',' << packet.force_x << ','
          << packet.force_y << ',' << (predicted != nullptr && predicted->IsOnGround()) << ','
          << packet.on_ground << ',' << (predicted != nullptr && predicted->IsOnGroundForLaw())
          << ',' << packet.on_ground_for_law << ','
          << (predicted != nullptr && predicted->WasOnGroundLastFrame()) << ','
          << packet.on_ground_last_frame << ','
          << (predicted != nullptr && predicted->IsOnGroundPermanent()) << ','
          << packet.on_ground_permanent << ','
          << (predicted != nullptr ? static_cast<int>(predicted->GetStance()) : 0) << ','
          << static_cast<int>(packet.stance) << ','
          << (predicted != nullptr ? static_cast<int>(predicted->GetDirection()) : 0) << ','
          << (predicted != nullptr ? static_cast<int>(predicted->GetOldDirection()) : 0) << ','
          << static_cast<int>(packet.old_direction) << ','
          << (predicted != nullptr ? predicted->GetJetsCount() : 0) << ',' << packet.jets_count
          << ',' << (predicted != nullptr && predicted->IsUsingJets()) << ',' << packet.using_jets
          << ',' << (predicted != nullptr ? static_cast<int>(predicted->GetBodyAnimationType()) : 0)
          << ',' << static_cast<int>(packet.body_animation_type) << ','
          << (predicted != nullptr ? predicted->GetBodyAnimationFrame() : 0) << ','
          << packet.body_animation_frame << ','
          << (predicted != nullptr ? predicted->GetBodyAnimationSpeed() : 0) << ','
          << packet.body_animation_speed << ','
          << (predicted != nullptr ? predicted->GetBodyAnimationCount() : 0) << ','
          << packet.body_animation_count << ','
          << (predicted != nullptr ? static_cast<int>(predicted->GetLegsAnimationType()) : 0) << ','
          << static_cast<int>(packet.legs_animation_type) << ','
          << (predicted != nullptr ? predicted->GetLegsAnimationFrame() : 0) << ','
          << packet.legs_animation_frame << ','
          << (predicted != nullptr ? predicted->GetLegsAnimationSpeed() : 0) << ','
          << packet.legs_animation_speed << ','
          << (predicted != nullptr ? predicted->GetLegsAnimationCount() : 0) << ','
          << packet.legs_animation_count << ','
          << (predicted != nullptr && predicted->CanThrowGrenade()) << ','
          << (predicted_control != nullptr && predicted_control->left) << ','
          << (predicted_control != nullptr && predicted_control->right) << ','
          << (predicted_control != nullptr && predicted_control->up) << ','
          << (predicted_control != nullptr && predicted_control->down) << ','
          << (predicted_control != nullptr && predicted_control->fire) << ','
          << (predicted_control != nullptr && predicted_control->jets) << ','
          << (predicted_control != nullptr && predicted_control->change) << ','
          << (predicted_control != nullptr && predicted_control->throw_grenade) << ','
          << (predicted_control != nullptr && predicted_control->drop) << ','
          << (predicted_control != nullptr && predicted_control->reload) << ','
          << (predicted_control != nullptr && predicted_control->prone) << ','
          << (predicted_control != nullptr && predicted_control->flag_throw) << ','
          << (predicted_control != nullptr ? predicted_control->mouse_aim_x : 0) << ','
          << (predicted_control != nullptr ? predicted_control->mouse_aim_y : 0) << ','
          << (predicted_control != nullptr ? predicted_control->mouse_dist : 0) << ','
          << (predicted_control != nullptr && predicted_control->was_running_left) << ','
          << (predicted_control != nullptr && predicted_control->was_jumping) << ','
          << (predicted_control != nullptr && predicted_control->was_throwing_weapon) << ','
          << (predicted_control != nullptr && predicted_control->was_changing_weapon) << ','
          << (predicted_control != nullptr && predicted_control->was_throwing_grenade) << ','
          << (predicted_control != nullptr && predicted_control->was_reloading_weapon) << ','
          << packet.mouse_map_position_x << ',' << packet.mouse_map_position_y << '\n';
        client_timeline_log_.flush();
    }
#endif

    void RemoveAcknowledgedNetworkInputs(std::uint32_t last_applied_input_id)
    {
        client_state_->network.pending_inputs.remove_if([last_applied_input_id](const auto& input) {
            return !IsSerialNumberNewer(input.input_sequence_id, last_applied_input_id);
        });
    }

    void PrunePredictionHistory(std::uint32_t authoritative_server_tick)
    {
        const std::uint32_t oldest_tick_to_keep =
          authoritative_server_tick - PREDICTION_HISTORY_TICK_GRACE;
        client_state_->network.prediction_inputs.remove_if(
          [oldest_tick_to_keep](const auto& input) {
              return IsSerialNumberOlder(input.apply_server_tick, oldest_tick_to_keep);
          });
        client_state_->network.soldier_snapshot_history.remove_if(
          [oldest_tick_to_keep](const auto& snapshot) {
              return IsSerialNumberOlder(snapshot.apply_server_tick, oldest_tick_to_keep);
          });
    }

    static constexpr float POSITION_RECONCILIATION_EPSILON = 1.0F;
    static constexpr float VELOCITY_RECONCILIATION_EPSILON = 0.15F;
    static constexpr float STATE_RECONCILIATION_POSITION_EPSILON = 0.25F;
    static constexpr std::uint32_t PREDICTION_HISTORY_TICK_GRACE = 4;

    std::shared_ptr<IWorld> world_;
    std::shared_ptr<ClientState> client_state_;
    std::ofstream reconciliation_log_{ "network-reconciliation.log", std::ios::trunc };
#ifndef NDEBUG
    std::ofstream client_timeline_log_{ "network-reconciliation-client.csv", std::ios::trunc };
#endif
    std::array<std::optional<std::uint32_t>, Config::MAX_PLAYERS> latest_server_ticks_{};
};
} // namespace Soldank
