#include "application/ClientNetworkEventObserver.hpp"
#include "communication/NetworkEventDispatcher.hpp"
#include "core/math/Calc.hpp"
#include "core/physics/SoldierSkeletonPhysics.hpp"

#include "spdlog/spdlog.h"

namespace Soldat
{
ClientNetworkEventObserver::ClientNetworkEventObserver(
  const std::shared_ptr<IWorld>& world,
  const std::shared_ptr<ClientState>& client_state)
    : world_(world)
    , client_state_(client_state)
{
}

NetworkEventObserverResult ClientNetworkEventObserver::OnAssignPlayerId(
  const ConnectionMetadata& /*connection_metadata*/,
  unsigned int assigned_player_id)
{
    const auto& soldier = world_->CreateSoldier(assigned_player_id);
    client_state_->client_soldier_id = soldier.id;
    spdlog::info("OnAssignPlayerId: {} {}", assigned_player_id, *client_state_->client_soldier_id);
    return NetworkEventObserverResult::Success;
}

NetworkEventObserverResult ClientNetworkEventObserver::OnChatMessage(
  const ConnectionMetadata& /*connection_metadata*/,
  const std::string& chat_message)
{
    spdlog::info("OnChatMessage: {}", chat_message);
    return NetworkEventObserverResult::Success;
}

NetworkEventObserverResult ClientNetworkEventObserver::OnSpawnSoldier(
  const ConnectionMetadata& /*connection_metadata*/,
  unsigned int soldier_id,
  glm::vec2 spawn_position)
{
    spdlog::info("OnSpawnSoldier: {}, ({}, {})", soldier_id, spawn_position.x, spawn_position.y);
    world_->SpawnSoldier(soldier_id, spawn_position);
    return NetworkEventObserverResult::Success;
}

NetworkEventObserverResult ClientNetworkEventObserver::OnSoldierInput(
  const ConnectionMetadata& connection_metadata,
  unsigned int input_sequence_id,
  unsigned int soldier_id,
  glm::vec2 soldier_position,
  glm::vec2 mouse_position,
  const Control& player_control)
{
    return NetworkEventObserverResult::Success;
}

NetworkEventObserverResult ClientNetworkEventObserver::OnSoldierState(
  const ConnectionMetadata& connection_metadata,
  unsigned int soldier_id,
  glm::vec2 soldier_position,
  glm::vec2 soldier_old_position,
  AnimationType body_animation_type,
  unsigned int body_animation_frame,
  int body_animation_speed,
  AnimationType legs_animation_type,
  unsigned int legs_animation_frame,
  int legs_animation_speed,
  glm::vec2 soldier_velocity,
  glm::vec2 soldier_force,
  bool on_ground,
  bool on_ground_for_law,
  bool on_ground_last_frame,
  bool on_ground_permanent,
  std::uint8_t stance,
  float mouse_position_x,
  float mouse_position_y,
  bool using_jets,
  std::int32_t jets_count,
  unsigned int active_weapon,
  unsigned int last_processed_input_id)
{
    bool is_soldier_id_me = false;
    if (client_state_->client_soldier_id.has_value()) {
        is_soldier_id_me = *client_state_->client_soldier_id == soldier_id;
    }
    if (is_soldier_id_me) {
        client_state_->soldier_position_server_pov = { soldier_position.x, soldier_position.y };
    }
    const auto& state = world_->GetState();

    for (auto& soldier : state->soldiers) {
        if (soldier.id == soldier_id) {
            soldier.particle.old_position = soldier_old_position;
            soldier.particle.position = soldier_position;
            soldier.body_animation = AnimationState(body_animation_type);
            soldier.body_animation.SetFrame(body_animation_frame);
            soldier.body_animation.SetSpeed(body_animation_speed);

            soldier.legs_animation = AnimationState(legs_animation_type);
            soldier.legs_animation.SetFrame(legs_animation_frame);
            soldier.legs_animation.SetSpeed(legs_animation_speed);

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
            soldier.mouse.x = mouse_position_x;
            soldier.mouse.y = mouse_position_y;

            soldier.camera.x =
              soldier.particle.position.x + (float)(soldier.mouse.x - (soldier.game_width / 2));
            soldier.camera.y = soldier.particle.position.y -
                               (float)((480.0F - soldier.mouse.y) - (soldier.game_height / 2));

            soldier.control.mouse_aim_x =
              (soldier.mouse.x - (float)soldier.game_width / 2.0F + soldier.camera.x);
            soldier.control.mouse_aim_y =
              (soldier.mouse.y - (float)soldier.game_height / 2.0F + soldier.camera.y);

            // TODO: there is a visual bug with feet when another soldier is using jets and going
            // backwards
            soldier.control.jets = using_jets;
            soldier.jets_count = jets_count;

            soldier.active_weapon = active_weapon;

            if (!is_soldier_id_me || !client_state_->client_side_prediction) {
                RepositionSoldierSkeletonParts(soldier);

                if (!soldier.dead_meat) {
                    soldier.body_animation.DoAnimation();
                    soldier.legs_animation.DoAnimation();
                    soldier.skeleton->DoVerletTimestepFor(22, 29);
                    soldier.skeleton->DoVerletTimestepFor(24, 30);
                }

                if (soldier.dead_meat) {
                    soldier.skeleton->DoVerletTimestep();
                    soldier.particle.position = soldier.skeleton->GetPos(12);
                    // CheckSkeletonOutOfBounds;
                }
            }
            // spdlog::info(
            //   "{}, Soldier {} pos: {}, {}; old_pod: {}, {}; velocity: {}, {}; force: {}, {}",
            //   last_processed_input_id,
            //   soldier_id,
            //   soldier_position.x,
            //   soldier_position.y,
            //   soldier_old_position.x,
            //   soldier_old_position.y,
            //   soldier_velocity.x,
            //   soldier_velocity.y,
            //   soldier_force.x,
            //   soldier_force.y);
        }
    }

    if (client_state_->server_reconciliation && is_soldier_id_me) {
        for (auto it = client_state_->pending_inputs.begin();
             it != client_state_->pending_inputs.end();) {
            if (it->input_sequence_id <= last_processed_input_id) {
                it = client_state_->pending_inputs.erase(it);
            } else {
                const auto& player_control = it->control;
                world_->UpdateRightButtonState(soldier_id, player_control.right);
                world_->UpdateLeftButtonState(soldier_id, player_control.left);
                world_->UpdateJumpButtonState(soldier_id, player_control.up);
                world_->UpdateCrouchButtonState(soldier_id, player_control.down);
                world_->UpdateProneButtonState(soldier_id, player_control.prone);
                world_->UpdateChangeButtonState(soldier_id, player_control.change);
                world_->UpdateThrowGrenadeButtonState(soldier_id, player_control.throw_grenade);
                world_->UpdateDropButtonState(soldier_id, player_control.drop);

                world_->UpdateMousePosition(soldier_id,
                                            { it->mouse_position_x, it->mouse_position_y });
                // world_->UpdateFireButtonState(soldier_id, player_control.fire); // TODO: is
                // it needed?
                world_->UpdateJetsButtonState(soldier_id, player_control.jets);
                world_->UpdateSoldier(soldier_id);
                // spdlog::info(
                //   "{}, Soldier {} pos: {}, {}; old_pos: {}, {}; velocity: {}, {}; force: {},
                //   {}", it->input_sequence_id, soldier_id,
                //   world_->GetSoldier(soldier_id).particle.position.x,
                //   world_->GetSoldier(soldier_id).particle.position.y,
                //   world_->GetSoldier(soldier_id).particle.old_position.x,
                //   world_->GetSoldier(soldier_id).particle.old_position.y,
                //   world_->GetSoldier(soldier_id).particle.GetVelocity().x,
                //   world_->GetSoldier(soldier_id).particle.GetVelocity().y,
                //   world_->GetSoldier(soldier_id).particle.GetForce().x,
                //   world_->GetSoldier(soldier_id).particle.GetForce().y);
                it++;
            }
        }
    }
    return NetworkEventObserverResult::Success;
}

NetworkEventObserverResult ClientNetworkEventObserver::OnSoldierInfo(
  const ConnectionMetadata& connection_metadata,
  unsigned int soldier_id)
{
    bool is_soldier_id_me = false;
    if (client_state_->client_soldier_id.has_value()) {
        is_soldier_id_me = *client_state_->client_soldier_id == soldier_id;
    }

    if (!is_soldier_id_me) {
        world_->CreateSoldier(soldier_id);
        world_->SpawnSoldier(soldier_id);
    }

    return NetworkEventObserverResult::Success;
}

NetworkEventObserverResult ClientNetworkEventObserver::OnPlayerLeave(
  const ConnectionMetadata& connection_metadata,
  unsigned int soldier_id)
{
    const auto& state = world_->GetState();
    for (auto it = state->soldiers.begin(); it != state->soldiers.end();) {
        if (it->id == soldier_id) {
            it = state->soldiers.erase(it);
        } else {
            it++;
        }
    }

    return NetworkEventObserverResult::Success;
}

NetworkEventObserverResult ClientNetworkEventObserver::OnPingCheck(
  const ConnectionMetadata& connection_metadata)
{
    if (client_state_->last_ping_check_time.has_value()) {
        auto current_time = std::chrono::system_clock::now();
        std::chrono::duration<double> diff = (current_time - *client_state_->last_ping_check_time);

        client_state_->last_ping = (std::uint16_t)(diff.count() * 1000.0);
        client_state_->last_ping_check_time = std::nullopt;
        spdlog::info("Ping: {}", client_state_->last_ping);
    }

    return NetworkEventObserverResult::Success;
}

NetworkEventObserverResult ClientNetworkEventObserver::OnProjectileSpawn(
  const ConnectionMetadata& connection_metadata,
  unsigned int projectile_id,
  BulletType style,
  WeaponType weapon,
  float position_x,
  float position_y,
  float velocity_x,
  float velocity_y,
  std::int16_t timeout,
  float hit_multiply,
  TeamType team)
{
    return NetworkEventObserverResult::Success;
}
} // namespace Soldat
