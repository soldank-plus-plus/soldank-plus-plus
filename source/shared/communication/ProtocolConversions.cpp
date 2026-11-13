module;

#include <cstdint>
#include <array>
#include <optional>

export module Shared.Networking.ProtocolConversions;

import Extern.Glm;

import Shared.Core.Entities.Bullet;
import Shared.Core.Simulation.SimulationCommands;
import Shared.Core.Simulation.SimulationEvents;
import Shared.Networking.NetworkPackets;

export namespace Soldank::ProtocolConversions
{
PlayerInputCommand ToPlayerInputCommand(std::uint8_t soldier_id, const SoldierInputPacket& packet)
{
    auto command = PlayerInputCommand{
        .soldier_id = soldier_id,
        .input_sequence_id = packet.input_sequence_id,
        .client_tick = packet.client_tick,
        .apply_server_tick = packet.apply_server_tick,
        .control = packet.control,
        .mouse_map_position = { packet.mouse_map_position_x, packet.mouse_map_position_y },
        .weapon_choices = std::nullopt,
        .appearance = std::nullopt,
    };
    if (packet.has_weapon_choice) {
        command.weapon_choices = std::array{ packet.primary_choice, packet.secondary_choice };
    }
    if (packet.has_appearance && packet.appearance.IsValid()) {
        command.appearance = packet.appearance;
    }
    return command;
}

ProjectileSpawnPacket ToProjectileSpawnPacket(std::uint32_t projectile_id,
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
        .push = event.bullet_params.push,
        .owner_ping_ticks = event.bullet_params.owner_ping_ticks,
        .seed = event.bullet_params.seed,
        .spawn_tick = event.bullet_params.spawn_tick,
        .derived = event.bullet_params.derived,
        .realistic = event.bullet_params.realistic,
    };
}

ProjectileSpawnedEvent ToProjectileSpawnedEvent(const ProjectileSpawnPacket& packet)
{
    return ProjectileSpawnedEvent{
        .bullet_params =
          BulletParams{
            .style = packet.style,
            .weapon = packet.weapon,
            .position = { packet.position_x, packet.position_y },
            .velocity = { packet.velocity_x, packet.velocity_y },
            .timeout = packet.timeout,
            .hit_multiply = packet.hit_multiply,
            .team = packet.team,
            .owner_id = packet.owner_id,
            .push = packet.push,
            .owner_ping_ticks = packet.owner_ping_ticks,
            .id = packet.projectile_id,
            .seed = packet.seed,
            .spawn_tick = packet.spawn_tick,
            .derived = packet.derived,
            .realistic = packet.realistic,
          },
    };
}
} // namespace Soldank::ProtocolConversions
