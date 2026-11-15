module;

#include <algorithm>
#include <array>
#include <cstdint>
#include <list>
#include <memory>
#include <optional>
#include <vector>
#include <cmath>

export module Networking.SoldierStateNetworkEventHandler;

import Extern.Glm;
import Extern.Spdlog;

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
import Shared.Core.Entities.Weapon;
import Shared.Core.Map.Polygons.PolygonBackgroundState;
import Shared.Core.Physics.Polygons.PolygonContact;
import Shared.Core.Entities.WeaponParametersFactory;
import Shared.Core.Types.WeaponType;
import Shared.Core.Types.TeamType;
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
        if (soldier_state_packet.team > TeamType::Spectator ||
            !std::isfinite(soldier_state_packet.health) ||
            !std::isfinite(soldier_state_packet.vest) || soldier_state_packet.active_weapon >= 2 ||
            (soldier_state_packet.weapon_count != 0 && soldier_state_packet.weapon_count != 3)) {
            return NetworkEventHandlerResult::Failure;
        }
        if (soldier_state_packet.has_polygon_state &&
            (!IsValidBackgroundState(soldier_state_packet.polygon_background,
                                     world_->GetStateManager()->GetConstMap().GetPolygons()) ||
             soldier_state_packet.death_severity > DeathSeverity::Brutal ||
             (soldier_state_packet.polygon_background.map_fingerprint != 0 &&
              soldier_state_packet.polygon_background.map_fingerprint !=
                world_->GetStateManager()->GetConstMap().GetPolygonFingerprint()))) {
            return NetworkEventHandlerResult::Failure;
        }
        std::uint8_t soldier_id = soldier_state_packet.player_id;
        if (soldier_id >= latest_server_ticks_.size()) {
            return NetworkEventHandlerResult::Failure;
        }
        std::optional<std::uint32_t>& latest_server_tick = latest_server_ticks_.at(soldier_id);
        if (latest_server_tick.has_value() &&
            !IsSerialNumberNewer(soldier_state_packet.server_tick, *latest_server_tick)) {
            return NetworkEventHandlerResult::Success;
        }
        if (soldier_state_packet.weapon_count == 3 &&
            soldier_state_packet.weapon_config_hash != 0) {
            const auto expected = WeaponParametersFactory::GetConfigurationHash(
              soldier_state_packet.weapons[0].realistic);
            if (soldier_state_packet.weapon_config_hash != expected) {
                if (!client_state_->network.weapon_config_mismatch) {
                    Spdlog::error("Weapon settings differ from the server. Install matching weapon "
                                  "files and reconnect.");
                }
                client_state_->network.weapon_config_mismatch = true;
                if (!config_mismatch_tick_ ||
                    IsSerialNumberNewer(soldier_state_packet.server_tick, *config_mismatch_tick_)) {
                    config_mismatch_tick_ = soldier_state_packet.server_tick;
                }
                if (!soldier_state_packet.appearance.IsValid() ||
                    soldier_state_packet.wear_helmet > 2) {
                    return NetworkEventHandlerResult::Failure;
                }
                latest_server_tick = soldier_state_packet.server_tick;
                return NetworkEventHandlerResult::Failure;
            }
            if (config_mismatch_tick_ &&
                IsSerialNumberOlder(soldier_state_packet.server_tick, *config_mismatch_tick_)) {
                return NetworkEventHandlerResult::Success;
            }
            client_state_->network.weapon_config_mismatch = false;
        }
        std::vector<Weapon> restored_weapons;
        if (soldier_state_packet.weapon_count == 3) {
            for (const auto& state : soldier_state_packet.weapons) {
                if (state.kind < WeaponType::DesertEagles || state.kind > WeaponType::ThrownKnife) {
                    return NetworkEventHandlerResult::Failure;
                }
                Weapon weapon(WeaponParametersFactory::GetParameters(state.kind, state.realistic));
                if (!weapon.RestoreState(state)) {
                    return NetworkEventHandlerResult::Failure;
                }
                restored_weapons.push_back(weapon);
            }
        }
        if (!restored_weapons.empty() &&
            world_->GetStateManager()->IsRealistic() != soldier_state_packet.weapons[0].realistic) {
            world_->GetStateManager()->SetRealistic(soldier_state_packet.weapons[0].realistic);
        }
        if (!soldier_state_packet.appearance.IsValid() || soldier_state_packet.wear_helmet > 2) {
            return NetworkEventHandlerResult::Failure;
        }
        latest_server_tick = soldier_state_packet.server_tick;
        world_->GetStateManager()->TransformSoldier(soldier_id, [&](auto& soldier) {
            soldier.latest_authoritative_tick = soldier_state_packet.server_tick;
            soldier.SetTeam(soldier_state_packet.team);
            soldier.appearance = soldier_state_packet.appearance;
            soldier.wear_helmet = soldier_state_packet.wear_helmet;
            soldier.has_cigar = soldier_state_packet.has_cigar;
            soldier.on_fire = soldier_state_packet.on_fire;
            soldier.alpha = soldier_state_packet.visual_alpha;
            soldier.visible = soldier_state_packet.visual_visible;
            if (soldier_state_packet.has_polygon_state) {
                soldier.death_severity = soldier_state_packet.death_severity;
            }
            // Health/armor are not predicted and are applied even when movement needs no
            // correction.
            if (soldier_state_packet.weapon_count == 3) {
                soldier.health = soldier_state_packet.health;
                soldier.vest = soldier_state_packet.vest;
                soldier.dead_meat = soldier_state_packet.dead_meat;
                soldier.ticks_to_respawn = soldier_state_packet.ticks_to_respawn;
                soldier.has_authoritative_health_snapshot = true;
            }
        });

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
            client_state_->network.soldier_position_server_pov = soldier_position;
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
            PublishPresentation(soldier_id);
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
                                   is_soldier_id_me,
                                   soldier_state_packet);
        if (predicted_state_accurate) {
            RemoveAcknowledgedNetworkInputs(last_applied_input_id);
            PrunePredictionHistory(soldier_state_packet.server_tick);
            PublishPresentation(soldier_id);
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
            const auto snapshot_local_tick = static_cast<unsigned int>(
              std::max<std::int64_t>(0,
                                     static_cast<std::int64_t>(soldier_state_packet.server_tick) -
                                       client_state_->network.server_tick_offset.value_or(0)));
            soldier.RestoreProjectilePushesAfterTick(snapshot_local_tick);
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

            if ((float)soldier.control.mouse_aim_x >= soldier.particle.position[0]) {
                soldier.direction = 1;
            } else {
                soldier.direction = -1;
            }

            // TODO: there is a visual bug with feet when another soldier is using jets and going
            // backwards
            soldier.control.jets = using_jets;
            soldier.jets_count = jets_count;

            soldier.active_weapon = active_weapon;
            if (soldier_state_packet.has_polygon_state) {
                soldier.polygon_background = soldier_state_packet.polygon_background;
                soldier.is_holding_flags = soldier_state_packet.carrying_flag;
                soldier.life_sequence = soldier_state_packet.life_sequence;
            }
            if (!restored_weapons.empty()) {
                soldier.weapons = restored_weapons;
                soldier.weapon_generation = soldier_state_packet.weapon_generation;
                soldier.grenade_can_throw = soldier_state_packet.grenade_can_throw;
                soldier.hit_spray = soldier_state_packet.hit_spray;
                soldier.mounted_weapon = soldier_state_packet.mounted_weapon;
                soldier.combat_bonus = soldier_state_packet.combat_bonus;
                soldier.bonus_ticks = soldier_state_packet.bonus_ticks;
                soldier.fired = soldier_state_packet.fired;
                soldier.control = soldier_state_packet.control;
            }

            if (soldier.body_animation->GetType() != body_animation_type) {
                AnimationState::ExitParams body_exit_params{ false };
                soldier.body_animation->Exit(body_exit_params);
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

        PublishPresentation(soldier_id);
        return NetworkEventHandlerResult::Success;
    }

    void PublishPresentation(std::uint8_t soldier_id)
    {
        world_->GetWorldEvents().after_soldier_updates.Notify(
          world_->GetSoldier(soldier_id), world_->GetStateManager()->GetGameTick());
    }

    bool IsPredictedStateAccurate(std::uint32_t server_tick,
                                  const glm::vec2& server_position,
                                  const glm::vec2& server_velocity,
                                  bool server_on_ground,
                                  std::uint8_t server_stance,
                                  bool is_soldier_id_me,
                                  const SoldierStatePacket& packet) const
    {
        if (!is_soldier_id_me || !client_state_->network.client_side_prediction ||
            !client_state_->network.server_reconciliation) {
            return false;
        }

        const auto snapshot = FindPredictedSnapshot(server_tick);
        if (snapshot == client_state_->network.soldier_snapshot_history.end()) {
            return false;
        }

        const auto& polygon_prediction = snapshot->soldier_snapshot;
        if (packet.has_polygon_state &&
            (polygon_prediction.GetPolygonBackground() != packet.polygon_background ||
             polygon_prediction.IsCarryingFlag() != packet.carrying_flag ||
             polygon_prediction.GetLifeSequence() != packet.life_sequence)) {
            return false;
        }
        if (packet.weapon_count != 0) {
            const auto& predicted = snapshot->soldier_snapshot;
            const auto& control = predicted.GetControl();
            if (control.was_changing_weapon != packet.control.was_changing_weapon ||
                control.was_throwing_weapon != packet.control.was_throwing_weapon ||
                control.was_throwing_grenade != packet.control.was_throwing_grenade ||
                control.was_reloading_weapon != packet.control.was_reloading_weapon) {
                return false;
            }
            const auto animation = packet.body_animation_type;
            const bool action_animation =
              animation == AnimationType::Throw || animation == AnimationType::ThrowWeapon ||
              animation == AnimationType::Change || animation == AnimationType::Punch ||
              animation == AnimationType::Melee || animation == AnimationType::Reload ||
              animation == AnimationType::ReloadBow || animation == AnimationType::ClipOut ||
              animation == AnimationType::ClipIn || animation == AnimationType::SlideBack;
            if (action_animation &&
                (predicted.GetBodyAnimationType() != animation ||
                 predicted.GetBodyAnimationFrame() != packet.body_animation_frame ||
                 predicted.GetBodyAnimationCount() != packet.body_animation_count)) {
                return false;
            }
            if (predicted.GetWeaponGeneration() != packet.weapon_generation ||
                predicted.GetHitSpray() != packet.hit_spray ||
                predicted.GetMountedWeapon() != packet.mounted_weapon ||
                predicted.GetCombatBonus() != packet.combat_bonus ||
                predicted.GetBonusTicks() != packet.bonus_ticks ||
                predicted.GetActiveWeapon() != packet.active_weapon ||
                predicted.CanThrowGrenade() != packet.grenade_can_throw ||
                predicted.GetWeapons().size() != packet.weapon_count ||
                !std::equal(predicted.GetWeapons().begin(),
                            predicted.GetWeapons().end(),
                            packet.weapons.begin())) {
                return false;
            }
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
        struct ReplayGuard
        {
            bool& flag;
            bool previous;
            explicit ReplayGuard(bool& replaying)
                : flag(replaying)
                , previous(replaying)
            {
                flag = true;
            }
            ~ReplayGuard() { flag = previous; }
        } guard(client_state_->network.replaying_prediction);
        auto& snapshot_history = client_state_->network.soldier_snapshot_history;
        snapshot_history.remove_if([authoritative_server_tick](const auto& snapshot) {
            return IsSerialNumberNewer(snapshot.apply_server_tick, authoritative_server_tick);
        });

        const auto replay_inputs =
          SelectReplayInputs(client_state_->network.prediction_inputs, authoritative_server_tick);
        const auto original_tick = world_->GetStateManager()->GetGameTick();
        for (const auto& input : replay_inputs) {
            world_->GetStateManager()->SetGameTick(input.client_tick);
            const auto player_input = ProtocolConversions::ToPlayerInputCommand(soldier_id, input);
            ApplyPlayerInputCommand(*world_->GetStateManager(), player_input);
            world_->UpdateSoldier(soldier_id);

            snapshot_history.emplace_back(input.input_sequence_id,
                                          input.client_tick,
                                          input.apply_server_tick,
                                          world_->GetSoldier(soldier_id));
        }
        world_->GetStateManager()->SetGameTick(original_tick);
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

    std::optional<std::uint32_t> config_mismatch_tick_;
    std::shared_ptr<IWorld> world_;
    std::shared_ptr<ClientState> client_state_;
    std::array<std::optional<std::uint32_t>, Config::MAX_PLAYERS> latest_server_ticks_{};
};
} // namespace Soldank
