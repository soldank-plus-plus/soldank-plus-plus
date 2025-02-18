#include "networking/event_handlers/SoldierStateNetworkEventHandler.hpp"

#include "communication/NetworkPackets.hpp"

#include "core/entities/Soldier.hpp"
#include "core/physics/SoldierSkeletonPhysics.hpp"

#include "spdlog/spdlog.h"

namespace Soldank
{
SoldierStateNetworkEventHandler::SoldierStateNetworkEventHandler(
  const std::shared_ptr<IWorld>& world,
  const std::shared_ptr<ClientState>& client_state)
    : world_(world)
    , client_state_(client_state)
{
}

NetworkEventHandlerResult SoldierStateNetworkEventHandler::HandleNetworkMessageImpl(
  unsigned int /*sender_connection_id*/,
  SoldierStatePacket soldier_state_packet)
{
    std::uint8_t soldier_id = soldier_state_packet.player_id;
    glm::vec2 soldier_position = { soldier_state_packet.position_x,
                                   soldier_state_packet.position_y };
    glm::vec2 soldier_old_position = { soldier_state_packet.old_position_x,
                                       soldier_state_packet.old_position_y };
    AnimationType body_animation_type = soldier_state_packet.body_animation_type;
    std::uint32_t body_animation_frame = soldier_state_packet.body_animation_frame;
    std::int32_t body_animation_speed = soldier_state_packet.body_animation_speed;
    AnimationType legs_animation_type = soldier_state_packet.legs_animation_type;
    std::uint32_t legs_animation_frame = soldier_state_packet.legs_animation_frame;
    std::int32_t legs_animation_speed = soldier_state_packet.legs_animation_speed;
    glm::vec2 soldier_velocity = { soldier_state_packet.velocity_x,
                                   soldier_state_packet.velocity_y };
    glm::vec2 soldier_force = { soldier_state_packet.force_x, soldier_state_packet.force_y };
    bool on_ground = soldier_state_packet.on_ground;
    bool on_ground_for_law = soldier_state_packet.on_ground_for_law;
    bool on_ground_last_frame = soldier_state_packet.on_ground_last_frame;
    bool on_ground_permanent = soldier_state_packet.on_ground_permanent;
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
    auto& state = world_->GetStateManager()->GetState();

    for (auto& soldier : state.soldiers) {
        if (soldier.id == soldier_id) {
            soldier.particle.old_position = soldier_old_position;
            soldier.particle.position = soldier_position;
            soldier.body_animation->Exit(soldier, world_->GetPhysicsEvents());
            soldier.body_animation = world_->GetBodyAnimationState(body_animation_type);
            soldier.body_animation->Enter(soldier);
            soldier.body_animation->SetFrame(body_animation_frame);
            soldier.body_animation->SetSpeed(body_animation_speed);

            soldier.legs_animation->Exit(soldier, world_->GetPhysicsEvents());
            soldier.legs_animation = world_->GetLegsAnimationState(legs_animation_type);
            soldier.legs_animation->Enter(soldier);
            soldier.legs_animation->SetFrame(legs_animation_frame);
            soldier.legs_animation->SetSpeed(legs_animation_speed);

            soldier.particle.SetVelocity(soldier_velocity);
            soldier.particle.SetForce(soldier_force);

            soldier.on_ground = on_ground;
            soldier.on_ground_for_law = on_ground_for_law;
            soldier.on_ground_last_frame = on_ground_last_frame;
            soldier.on_ground_permanent = on_ground_permanent;

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

                if (!soldier.dead_meat) {
                    soldier.body_animation->DoAnimation();
                    soldier.legs_animation->DoAnimation();
                    soldier.skeleton->DoVerletTimestepFor(22, 29);
                    soldier.skeleton->DoVerletTimestepFor(24, 30);
                }

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
                    // TODO: It looks like body_animation_frame and
                    // legs_animation_frame are always behind the server by 1 frame
                    it->second.CompareAndLog(soldier);
                }
            }
        }
    }

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
                    it->mouse_map_position_y }); // TODO: smooth camera handling, probably need to
                                                 // send mouse aim instead of cursor pos in packets
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
} // namespace Soldank
