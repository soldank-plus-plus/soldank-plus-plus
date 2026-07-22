module;

#include <algorithm>
#include <array>
#include <cstdint>
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

        const bool awaiting_timeline_resynchronization =
          is_soldier_id_me && client_state_->network.client_side_prediction &&
          client_state_->network.server_reconciliation &&
          client_state_->network.reconciliation_resume_server_tick.has_value() &&
          IsSerialNumberOlder(soldier_state_packet.server_tick,
                              *client_state_->network.reconciliation_resume_server_tick);
        if (awaiting_timeline_resynchronization) {
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
            RemoveAcknowledgedNetworkInputs(last_applied_input_id);
            PrunePredictionHistory(soldier_state_packet.server_tick);
            return NetworkEventHandlerResult::Success;
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

        if (client_state_->network.server_reconciliation && is_soldier_id_me) {
            RemoveAcknowledgedNetworkInputs(last_applied_input_id);
            ReplayPredictedInputsAfterTick(soldier_id, soldier_state_packet.server_tick);
            PrunePredictionHistory(soldier_state_packet.server_tick);
        }

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

    void ReplayPredictedInputsAfterTick(std::uint8_t soldier_id,
                                        std::uint32_t authoritative_server_tick)
    {
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
        }
    }

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
    std::array<std::optional<std::uint32_t>, Config::MAX_PLAYERS> latest_server_ticks_{};
};
} // namespace Soldank
