#include "networking/event_handlers/ProjectileSpawnNetworkEventHandler.hpp"

#include "communication/NetworkPackets.hpp"
#include "core/math/Calc.hpp"
#include "spdlog/spdlog.h"

#include <limits>

namespace Soldank
{
ProjectileSpawnNetworkEventHandler::ProjectileSpawnNetworkEventHandler(
  const std::shared_ptr<IWorld>& world,
  const std::shared_ptr<ClientState>& client_state)
    : world_(world)
    , client_state_(client_state)
{
}

NetworkEventHandlerResult ProjectileSpawnNetworkEventHandler::HandleNetworkMessageImpl(
  unsigned int /*sender_connection_id*/,
  ProjectileSpawnPacket projectile_spawn_packet)
{
    unsigned int projectile_id = projectile_spawn_packet.projectile_id;
    BulletType style = projectile_spawn_packet.style;
    WeaponType weapon = projectile_spawn_packet.weapon;
    float position_x = projectile_spawn_packet.position_x;
    float position_y = projectile_spawn_packet.position_y;
    float velocity_x = projectile_spawn_packet.velocity_x;
    float velocity_y = projectile_spawn_packet.velocity_y;
    std::int16_t timeout = projectile_spawn_packet.timeout;
    float hit_multiply = projectile_spawn_packet.hit_multiply;
    TeamType team = projectile_spawn_packet.team;
    std::uint8_t owner_id = projectile_spawn_packet.owner_id;
    std::uint32_t last_processed_input_id = projectile_spawn_packet.last_processed_input_id;
    std::int32_t server_creation_order = projectile_spawn_packet.creation_order;

    bool found_similar_projectile = false;
    unsigned int similar_projectile_id = 0;
    float similar_projectile_distance = std::numeric_limits<float>::infinity();

    for (const auto& [input_sequence_id, client_creation_order, past_projectile] :
         client_state_->created_bullets_history) {
        if (input_sequence_id != last_processed_input_id) {
            continue;
        }

        if (past_projectile.owner_id != owner_id) {
            continue;
        }

        if (style != past_projectile.style) {
            continue;
        }

        if (weapon != past_projectile.weapon) {
            continue;
        }

        if (client_creation_order != server_creation_order) {
            continue;
        }

        float position_distance =
          Calc::SquareDistance({ position_x, position_y }, past_projectile.particle.position);

        if (position_distance < similar_projectile_distance) {
            found_similar_projectile = true;
            similar_projectile_id = past_projectile.id;
            similar_projectile_distance = position_distance;
        }
    }

    // remove created bullets from history if they are older than all of our pending_inputs
    for (auto it = client_state_->created_bullets_history.begin();
         it != client_state_->created_bullets_history.end();) {
        if (it->input_sequence_id <=
            last_processed_input_id - client_state_->pending_inputs.size()) {
            it = client_state_->created_bullets_history.erase(it);
        } else {
            ++it;
        }
    }

    const Bullet* created_projectile = nullptr;

    BulletParams bullet_params{ style,
                                weapon,
                                { position_x, position_y },
                                { velocity_x, velocity_y },
                                timeout,
                                hit_multiply,
                                team,
                                owner_id };

    if (found_similar_projectile) {
        spdlog::debug(
          "projectile_id vs similar_projectile_id: {} vs {}", projectile_id, similar_projectile_id);
        world_->GetStateManager()->TransformBullet(similar_projectile_id, [&](Bullet& bullet) {
            bullet = bullet_params;
            bullet.active = true;
            bullet.id = similar_projectile_id;
        });
        world_->GetStateManager()->SwapProjectiles(projectile_id, similar_projectile_id);
        for (auto& [input_sequence_id, creation_order, past_projectile] :
             client_state_->created_bullets_history) {

            if (past_projectile.id == projectile_id) {
                past_projectile.id = similar_projectile_id;
            } else if (past_projectile.id == similar_projectile_id) {
                past_projectile.id = projectile_id;
            }
        }
        created_projectile = &world_->GetStateManager()->GetBullet(projectile_id);
    } else {
        created_projectile = world_->GetStateManager()->CreateProjectile(bullet_params);
    }

    if (created_projectile == nullptr) {
        return NetworkEventHandlerResult::Failure;
    }

    if (client_state_->server_reconciliation) {
        for (auto it = client_state_->pending_inputs.begin();
             it != client_state_->pending_inputs.end();
             ++it) {
            if (it->input_sequence_id <= last_processed_input_id) {
                continue;
            }

            world_->UpdateProjectile(created_projectile->id);
        }
    }

    return NetworkEventHandlerResult::Success;
}
} // namespace Soldank
