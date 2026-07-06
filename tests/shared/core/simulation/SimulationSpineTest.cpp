#include "core/math/Glm.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <variant>

import Shared.Core.Entities.Bullet;
import Shared.Core.Simulation.SimulationCommandQueue;
import Shared.Core.Simulation.SimulationCommands;
import Shared.Core.Simulation.SimulationEvents;
import Shared.Core.Types.BulletType;
import Shared.Core.Types.TeamType;
import Shared.Core.Types.WeaponType;
import Shared.Networking.NetworkPackets;
import Shared.Networking.ProtocolConversions;

using namespace Soldank;

TEST(SimulationSpineTest, DrainsQueuedSimulationCommandsInOrder)
{
    SimulationCommandQueue command_queue;
    command_queue.Enqueue(SpawnSoldierCommand{ .soldier_id = 4 });
    command_queue.Enqueue(KillSoldierCommand{ .soldier_id = 7 });

    auto commands = command_queue.Drain();

    ASSERT_EQ(commands.size(), 2);
    ASSERT_TRUE(std::holds_alternative<SpawnSoldierCommand>(commands.at(0)));
    ASSERT_TRUE(std::holds_alternative<KillSoldierCommand>(commands.at(1)));
    ASSERT_TRUE(command_queue.IsEmpty());
}

TEST(SimulationSpineTest, ConvertsSoldierInputPacketToPlayerInputCommand)
{
    SoldierInputPacket packet{
        .input_sequence_id = 11,
        .game_tick = 22,
        .position_x = 1.0F,
        .position_y = 2.0F,
        .mouse_map_position_x = 3.0F,
        .mouse_map_position_y = 4.0F,
        .control = {},
    };
    packet.control.left = true;
    packet.control.fire = true;

    auto command = ProtocolConversions::ToPlayerInputCommand(9, packet);

    ASSERT_EQ(command.soldier_id, 9);
    ASSERT_EQ(command.input_sequence_id, 11U);
    ASSERT_EQ(command.tick, 22U);
    ASSERT_TRUE(command.control.left);
    ASSERT_TRUE(command.control.fire);
    ASSERT_EQ(command.mouse_map_position.x, 3.0F);
    ASSERT_EQ(command.mouse_map_position.y, 4.0F);
}

TEST(SimulationSpineTest, ConvertsProjectileSpawnEventToPacket)
{
    ProjectileSpawnedEvent event{
        .bullet_params = BulletParams{
          .style = BulletType::FragGrenade,
          .weapon = WeaponType::FragGrenade,
          .position = { 10.0F, 20.0F },
          .velocity = { 30.0F, 40.0F },
          .timeout = 50,
          .hit_multiply = 1.5F,
          .team = TeamType::Alpha,
          .owner_id = 6,
          .push = 0.0F,
        },
    };

    auto packet = ProtocolConversions::ToProjectileSpawnPacket(3, event);

    ASSERT_EQ(packet.projectile_id, 3);
    ASSERT_EQ(packet.style, BulletType::FragGrenade);
    ASSERT_EQ(packet.weapon, WeaponType::FragGrenade);
    ASSERT_EQ(packet.position_x, 10.0F);
    ASSERT_EQ(packet.position_y, 20.0F);
    ASSERT_EQ(packet.velocity_x, 30.0F);
    ASSERT_EQ(packet.velocity_y, 40.0F);
    ASSERT_EQ(packet.timeout, 50);
    ASSERT_EQ(packet.hit_multiply, 1.5F);
    ASSERT_EQ(packet.team, TeamType::Alpha);
    ASSERT_EQ(packet.owner_id, 6);
}
