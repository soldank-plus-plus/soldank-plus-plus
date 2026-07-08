module;

#include <cstdint>

export module Shared.Networking.ProtocolConversions;

import Extern.Glm;

import Shared.Core.Entities.Bullet;
import Shared.Core.Simulation.SimulationCommands;
import Shared.Core.Simulation.SimulationEvents;
import Shared.Networking.NetworkPackets;

export namespace Soldank::ProtocolConversions
{
PlayerInputCommand ToPlayerInputCommand(std::uint8_t soldier_id,
                                        const SoldierInputPacket& packet)
{
    return PlayerInputCommand{
        .soldier_id = soldier_id,
        .input_sequence_id = packet.input_sequence_id,
        .tick = packet.game_tick,
        .control = packet.control,
        .mouse_map_position = { packet.mouse_map_position_x, packet.mouse_map_position_y },
    };
}

ProjectileSpawnPacket ToProjectileSpawnPacket(std::uint16_t projectile_id,
                                              const ProjectileSpawnedEvent& event)
{
    return ProjectileSpawnPacket{
        .projectile_id = projectile_id,
        .style = event.bullet_params.style,
        .weapon = event.bullet_params.weapon,
        .position_x = event.bullet_params.position.x,
        .position_y = event.bullet_params.position.y,
        .velocity_x = event.bullet_params.velocity.x,
        .velocity_y = event.bullet_params.velocity.y,
        .timeout = event.bullet_params.timeout,
        .hit_multiply = event.bullet_params.hit_multiply,
        .team = event.bullet_params.team,
        .owner_id = event.bullet_params.owner_id,
    };
}

ProjectileSpawnedEvent ToProjectileSpawnedEvent(const ProjectileSpawnPacket& packet)
{
    return ProjectileSpawnedEvent{
        .bullet_params = BulletParams{
          .style = packet.style,
          .weapon = packet.weapon,
          .position = { packet.position_x, packet.position_y },
          .velocity = { packet.velocity_x, packet.velocity_y },
          .timeout = packet.timeout,
          .hit_multiply = packet.hit_multiply,
          .team = packet.team,
          .owner_id = packet.owner_id,
          .push = 0.0F,
        },
    };
}
} // namespace Soldank::ProtocolConversions
