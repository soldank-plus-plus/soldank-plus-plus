module;

#include "core/math/Glm.hpp"

#include <cstdint>
#include <memory>

export module Networking.SoldierStateNetworkEventHandler;

import ClientState;

import Shared.Core.IWorld;

import Shared.Networking.NetworkEventDispatcher;
import Shared.Networking.NetworkPackets;
import Shared.Networking.NetworkEvent;

import Shared.Core.Physics.SoldierSkeletonPhysics;
import Shared.Core.Animations;
import Shared.Core.Entities.Soldier;
import Shared.Core.State.Control;

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
        std::uint32_t last_processed_input_id = soldier_state_packet.last_processed_input_id;

        bool is_soldier_id_me = false;
        if (client_state_->client_soldier_id.has_value()) {
            is_soldier_id_me = *client_state_->client_soldier_id == soldier_id;
        }
        if (is_soldier_id_me) {
            client_state_->soldier_position_server_pov = { soldier_position.x, soldier_position.y };
        }

        world_->GetStateManager()->TransformSoldier(soldier_id, [&](auto& soldier) {
            soldier.particle.old_position = soldier_old_position;
            soldier.particle.position = soldier_position;
            AnimationState::ExitParams body_exit_params{ false };
            soldier.body_animation->Exit(body_exit_params);
            if (body_exit_params.should_throw_active_weapon) {
                world_->GetPhysicsEvents().soldier_throws_active_weapon.Notify(soldier);
            }
            soldier.body_animation = world_->GetBodyAnimationState(body_animation_type);
            AnimationState::EnterParams body_enter_params{ soldier.on_ground,
                                                           soldier.direction,
                                                           soldier.particle.GetForce(),
                                                           soldier.grenade_can_throw,
                                                           soldier.weapons,
                                                           soldier.active_weapon };
            soldier.body_animation->Enter(body_enter_params);
            soldier.particle.SetForce(body_enter_params.force);
            soldier.grenade_can_throw = body_enter_params.grenade_can_throw;
            soldier.body_animation->SetFrame(body_animation_frame);
            soldier.body_animation->SetSpeed(body_animation_speed);
            soldier.body_animation->SetCount(body_animation_count);

            AnimationState::ExitParams legs_exit_params{ false };
            soldier.legs_animation->Exit(legs_exit_params);
            if (legs_exit_params.should_throw_active_weapon) {
                world_->GetPhysicsEvents().soldier_throws_active_weapon.Notify(soldier);
            }
            soldier.legs_animation = world_->GetLegsAnimationState(legs_animation_type);
            AnimationState::EnterParams legs_enter_params{ soldier.on_ground,
                                                           soldier.direction,
                                                           soldier.particle.GetForce(),
                                                           soldier.grenade_can_throw,
                                                           soldier.weapons,
                                                           soldier.active_weapon };
            soldier.legs_animation->Enter(legs_enter_params);
            soldier.particle.SetForce(legs_enter_params.force);
            soldier.grenade_can_throw = legs_enter_params.grenade_can_throw;
            soldier.legs_animation->SetFrame(legs_animation_frame);
            soldier.legs_animation->SetSpeed(legs_animation_speed);
            soldier.legs_animation->SetCount(legs_animation_count);

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

            if (!is_soldier_id_me || !client_state_->client_side_prediction) {
                RepositionSoldierSkeletonParts(soldier);

                // TODO: Figure out if the below section is needed, I have a hunch that it is not
                if (soldier.dead_meat) {
                    soldier.skeleton->DoVerletTimestep();
                    soldier.particle.position = soldier.skeleton->GetPos(12);
                    // CheckSkeletonOutOfBounds;
                }
            }
            for (auto it = client_state_->soldier_snapshot_history.begin();
                 it != client_state_->soldier_snapshot_history.end();
                 ++it) {
                if (it->first == last_processed_input_id + 1) {
                    it->second.CompareAndLog(soldier);
                }
            }
        });

        if (client_state_->server_reconciliation && is_soldier_id_me) {
            for (auto it = client_state_->soldier_snapshot_history.begin();
                 it != client_state_->soldier_snapshot_history.end();) {
                if (it->first <= last_processed_input_id) {
                    it = client_state_->soldier_snapshot_history.erase(it);
                } else {
                    break;
                }
            }
            for (auto it = client_state_->pending_inputs.begin();
                 it != client_state_->pending_inputs.end();) {
                if (it->input_sequence_id <= last_processed_input_id) {
                    it = client_state_->pending_inputs.erase(it);
                } else {
                    const auto& player_control = it->control;
                    world_->GetStateManager()->ChangeSoldierControlActionState(
                      soldier_id, ControlActionType::MoveLeft, player_control.left);
                    world_->GetStateManager()->ChangeSoldierControlActionState(
                      soldier_id, ControlActionType::MoveRight, player_control.right);
                    world_->GetStateManager()->ChangeSoldierControlActionState(
                      soldier_id, ControlActionType::Jump, player_control.up);
                    world_->GetStateManager()->ChangeSoldierControlActionState(
                      soldier_id, ControlActionType::Crouch, player_control.down);
                    world_->GetStateManager()->ChangeSoldierControlActionState(
                      soldier_id, ControlActionType::ChangeWeapon, player_control.change);
                    world_->GetStateManager()->ChangeSoldierControlActionState(
                      soldier_id, ControlActionType::ThrowGrenade, player_control.throw_grenade);
                    world_->GetStateManager()->ChangeSoldierControlActionState(
                      soldier_id, ControlActionType::DropWeapon, player_control.drop);
                    world_->GetStateManager()->ChangeSoldierControlActionState(
                      soldier_id, ControlActionType::Prone, player_control.prone);

                    world_->GetStateManager()->ChangeSoldierMouseMapPosition(
                      soldier_id,
                      { it->mouse_map_position_x,
                        it->mouse_map_position_y }); // TODO: smooth camera handling, probably need
                                                     // to send mouse aim instead of cursor pos in
                                                     // packets
                    world_->GetStateManager()->ChangeSoldierControlActionState(
                      soldier_id, ControlActionType::UseJets, player_control.jets);

                    // TODO: is it needed?
                    world_->GetStateManager()->ChangeSoldierControlActionState(
                      soldier_id, ControlActionType::Fire, player_control.fire);

                    world_->UpdateSoldier(soldier_id);
                    it++;
                }
            }
        }

        return NetworkEventHandlerResult::Success;
    }

    std::shared_ptr<IWorld> world_;
    std::shared_ptr<ClientState> client_state_;
};
} // namespace Soldank
