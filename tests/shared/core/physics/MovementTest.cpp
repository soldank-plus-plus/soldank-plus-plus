#include <gtest/gtest.h>

#include <vector>
#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <span>
#include <sstream>
#include <string>

#include "core/utility/Expected.hpp"

import Tests.Shared.Core.Physics.SoldierMovementSimulation;
import Testing.Framework.Shared.MapBuilder;

import Shared.Core.Physics.SoldierPhysics;
import Shared.Core.Simulation.SimulationCommands;
import Shared.Core.State.StateManager;
import Shared.Core.State.Control;
import Shared.Core.Animations;
import Shared.Core.Map.PMSEnums;
import Shared.Core.Map.PMSStructs;
import Shared.Core.Entities.Soldier;
import Shared.Core.Entities.WeaponParametersFactory;
import Shared.Core.Data.IFileReader;

std::filesystem::path test_file_path;

class FileReaderForTestsWorkingDirectory : public Soldank::IFileReader
{
public:
    std::expected<std::string, Soldank::FileReaderError> Read(
      const std::string& file_path,
      std::ios_base::openmode mode = std::ios_base::in) const override
    {
        std::filesystem::path absolute_file_path = test_file_path;
        absolute_file_path.append(file_path);
        std::ifstream file_to_read(absolute_file_path, mode);
        if (!file_to_read.is_open()) {
            return std::unexpected(Soldank::FileReaderError::FileNotFound);
        }

        std::stringstream buffer;
        buffer << file_to_read.rdbuf();

        if (buffer.bad()) {
            return std::unexpected(Soldank::FileReaderError::BufferError);
        }

        return buffer.str();
    }
};

TEST(MovementTest, AppliesCompletePlayerInputControlState)
{
    SoldankTesting::SoldierMovementSimulation simulation{ FileReaderForTestsWorkingDirectory() };
    const Soldank::Control input_control{
        .left = true,
        .right = false,
        .up = true,
        .down = false,
        .fire = true,
        .jets = false,
        .change = true,
        .throw_grenade = false,
        .drop = true,
        .reload = true,
        .prone = false,
        .flag_throw = true,
        .mouse_aim_x = -100,
        .mouse_aim_y = -200,
        .mouse_dist = 321,
        .was_running_left = true,
        .was_jumping = false,
        .was_throwing_weapon = true,
        .was_changing_weapon = false,
        .was_throwing_grenade = true,
        .was_reloading_weapon = true,
    };
    const Soldank::PlayerInputCommand command{
        .soldier_id = 0,
        .input_sequence_id = 1,
        .client_tick = 2,
        .apply_server_tick = 3,
        .control = input_control,
        .mouse_map_position = { 123.0F, 456.0F },
    };

    simulation.ApplyPlayerInput(command);

    const Soldank::Control& applied_control = simulation.GetSoldierControl();
    EXPECT_EQ(applied_control.left, input_control.left);
    EXPECT_EQ(applied_control.right, input_control.right);
    EXPECT_EQ(applied_control.up, input_control.up);
    EXPECT_EQ(applied_control.down, input_control.down);
    EXPECT_EQ(applied_control.fire, input_control.fire);
    EXPECT_EQ(applied_control.jets, input_control.jets);
    EXPECT_EQ(applied_control.change, input_control.change);
    EXPECT_EQ(applied_control.throw_grenade, input_control.throw_grenade);
    EXPECT_EQ(applied_control.drop, input_control.drop);
    EXPECT_EQ(applied_control.reload, input_control.reload);
    EXPECT_EQ(applied_control.prone, input_control.prone);
    EXPECT_EQ(applied_control.flag_throw, input_control.flag_throw);
    EXPECT_EQ(applied_control.mouse_aim_x, 123);
    EXPECT_EQ(applied_control.mouse_aim_y, 456);
    EXPECT_EQ(applied_control.mouse_dist, input_control.mouse_dist);
    EXPECT_EQ(applied_control.was_running_left, input_control.was_running_left);
    EXPECT_EQ(applied_control.was_jumping, input_control.was_jumping);
    EXPECT_EQ(applied_control.was_throwing_weapon, input_control.was_throwing_weapon);
    EXPECT_EQ(applied_control.was_changing_weapon, input_control.was_changing_weapon);
    EXPECT_EQ(applied_control.was_throwing_grenade, input_control.was_throwing_grenade);
    EXPECT_EQ(applied_control.was_reloading_weapon, input_control.was_reloading_weapon);
}

TEST(MovementTest, TestRunBackRight)
{
    SoldankTesting::SoldierMovementSimulation simulation{ FileReaderForTestsWorkingDirectory() };
    simulation.RunUntilSoldierOnGround();
    simulation.LookLeft();
    simulation.RunFor(10);
    simulation.HoldRight();
    simulation.AddSoldierExpectedAnimationState(
      1,
      { .part = SoldankTesting::SoldierAnimationPart::Legs,
        .expected_animation_type = Soldank::AnimationType::RunBack,
        .expected_frame = 3,
        .expected_speed = 1,
        .expected_position_diff_from_origin = { 0.118F, -0.0444F } });
    simulation.RunFor(10);
}

TEST(MovementTest, TestRunBackLeft)
{
    SoldankTesting::SoldierMovementSimulation simulation{ FileReaderForTestsWorkingDirectory() };
    simulation.RunUntilSoldierOnGround();
    simulation.LookRight();
    simulation.RunFor(10);
    simulation.HoldLeft();
    simulation.AddSoldierExpectedAnimationState(
      1,
      { .part = SoldankTesting::SoldierAnimationPart::Legs,
        .expected_animation_type = Soldank::AnimationType::RunBack,
        .expected_frame = 3,
        .expected_speed = 1,
        .expected_position_diff_from_origin = { -0.118F, -0.0444F } });
    simulation.RunFor(10);
}

TEST(MovementTest, TestFallAndJumpSideLeft)
{
    SoldankTesting::SoldierMovementSimulation simulation{ FileReaderForTestsWorkingDirectory() };
    simulation.RunUntilSoldierOnGround();
    simulation.LookLeft();
    simulation.RunFor(10);
    simulation.HoldLeft();
    simulation.HoldJump();
    simulation.AddSoldierExpectedAnimationState(
      0,
      { .part = SoldankTesting::SoldierAnimationPart::Legs,
        .expected_animation_type = Soldank::AnimationType::JumpSide,
        .expected_frame = 2,
        .expected_speed = 1,
        .expected_position_diff_from_origin = { 0.0F, 0.0F } });
    simulation.AddSoldierExpectedAnimationState(
      3,
      { .part = SoldankTesting::SoldierAnimationPart::Legs,
        .expected_animation_type = Soldank::AnimationType::JumpSide,
        .expected_frame = 5,
        .expected_speed = 1,
        .expected_position_diff_from_origin = { 0.0F, 0.0018F } });
    simulation.RunFor(10);
}

TEST(MovementTest, TestJumpSideLeft)
{
    SoldankTesting::SoldierMovementSimulation simulation{ FileReaderForTestsWorkingDirectory() };
    simulation.RunUntilSoldierOnGround();
    simulation.RunFor(10);
    simulation.HoldLeft();
    simulation.HoldJump();
    simulation.AddSoldierExpectedAnimationState(
      0,
      { .part = SoldankTesting::SoldierAnimationPart::Legs,
        .expected_animation_type = Soldank::AnimationType::JumpSide,
        .expected_frame = 2,
        .expected_speed = 1,
        .expected_position_diff_from_origin = { 0.0F, 0.0F } });
    simulation.AddSoldierExpectedAnimationState(
      4,
      { .part = SoldankTesting::SoldierAnimationPart::Legs,
        .expected_animation_type = Soldank::AnimationType::JumpSide,
        .expected_frame = 6,
        .expected_speed = 1,
        .expected_position_diff_from_origin = { -0.3F, -0.1882 } });
    simulation.RunFor(10);
}

TEST(MovementTest, TestLateBackflipLeftLookingRight)
{
    SoldankTesting::SoldierMovementSimulation simulation{ FileReaderForTestsWorkingDirectory() };
    simulation.RunUntilSoldierOnGround();
    simulation.LookRight();
    simulation.RunFor(10);
    simulation.HoldLeft();
    simulation.HoldJumpAt(1);
    simulation.HoldJetsAt(32);
    simulation.ReleaseLeftAt(35);
    simulation.ReleaseJetsAt(35);
    simulation.ReleaseJumpAt(35);
    simulation.AddSoldierExpectedAnimationState(
      0,
      { .part = SoldankTesting::SoldierAnimationPart::Legs,
        .expected_animation_type = Soldank::AnimationType::RunBack,
        .expected_frame = 2,
        .expected_speed = 1,
        .expected_position_diff_from_origin = { 0.0F, 0.0F } });
    simulation.AddSoldierExpectedAnimationState(
      1,
      { .part = SoldankTesting::SoldierAnimationPart::Legs,
        .expected_animation_type = Soldank::AnimationType::JumpSide,
        .expected_frame = 2,
        .expected_speed = 1,
        .expected_position_diff_from_origin = { -0.118F, -0.0444F } });
    simulation.AddSoldierExpectedAnimationState(
      32,
      { .part = SoldankTesting::SoldierAnimationPart::Legs,
        .expected_animation_type = Soldank::AnimationType::RollBack,
        .expected_frame = 2,
        .expected_speed = 1,
        .expected_position_diff_from_origin = { -49.6917F, -16.4150F } });
    simulation.AddSoldierExpectedAnimationState(
      32,
      { .part = SoldankTesting::SoldierAnimationPart::Body,
        .expected_animation_type = Soldank::AnimationType::RollBack,
        .expected_frame = 2,
        .expected_speed = 1,
        .expected_position_diff_from_origin = { -49.6917F, -16.4150F } });
    simulation.AddSoldierExpectedAnimationState(
      63,
      { .part = SoldankTesting::SoldierAnimationPart::Body,
        .expected_animation_type = Soldank::AnimationType::Stand,
        .expected_frame = 1,
        .expected_speed = 3,
        .expected_position_diff_from_origin = { -104.7013F, -9.7904F } });
    simulation.AddSoldierExpectedAnimationState(
      64,
      { .part = SoldankTesting::SoldierAnimationPart::Legs,
        .expected_animation_type = Soldank::AnimationType::Fall,
        .expected_frame = 3,
        .expected_speed = 1,
        .expected_position_diff_from_origin = { -107.0300F, -8.7348F } });
    simulation.RunFor(70);
}

TEST(MovementTest, TestCannonballLeftLookingRight)
{
    SoldankTesting::SoldierMovementSimulation simulation{ FileReaderForTestsWorkingDirectory() };
    simulation.RunUntilSoldierOnGround();
    simulation.LookRight();
    simulation.RunFor(6);
    simulation.HoldLeft();
    simulation.HoldJumpAt(1);
    simulation.ReleaseJumpAt(14);
    simulation.HoldJetsAt(12);
    simulation.ReleaseLeftAt(14);
    simulation.ReleaseJetsAt(19);

    simulation.AddSoldierExpectedAnimationState(
      0,
      { .part = SoldankTesting::SoldierAnimationPart::Legs,
        .expected_animation_type = Soldank::AnimationType::RunBack,
        .expected_frame = 2,
        .expected_speed = 1,
        .expected_position_diff_from_origin = { 0.0F, 0.0F } });
    simulation.AddSoldierExpectedAnimationState(
      1,
      { .part = SoldankTesting::SoldierAnimationPart::Legs,
        .expected_animation_type = Soldank::AnimationType::JumpSide,
        .expected_frame = 2,
        .expected_speed = 1,
        .expected_position_diff_from_origin = { -0.1180F, -0.0444F } });
    simulation.AddSoldierExpectedAnimationState(
      12,
      { .part = SoldankTesting::SoldierAnimationPart::Legs,
        .expected_animation_type = Soldank::AnimationType::RollBack,
        .expected_frame = 2,
        .expected_speed = 1,
        .expected_position_diff_from_origin = { -11.5269F, -6.4391F } });
    simulation.AddSoldierExpectedAnimationState(
      12,
      { .part = SoldankTesting::SoldierAnimationPart::Body,
        .expected_animation_type = Soldank::AnimationType::RollBack,
        .expected_frame = 2,
        .expected_speed = 1,
        .expected_position_diff_from_origin = { -11.5269F, -6.4391F } });
    simulation.AddSoldierExpectedAnimationState(
      43,
      { .part = SoldankTesting::SoldierAnimationPart::Body,
        .expected_animation_type = Soldank::AnimationType::Stand,
        .expected_frame = 1,
        .expected_speed = 3,
        .expected_position_diff_from_origin = { -82.7292F, -23.2314F } });
    simulation.AddSoldierExpectedAnimationState(
      44,
      { .part = SoldankTesting::SoldierAnimationPart::Legs,
        .expected_animation_type = Soldank::AnimationType::Fall,
        .expected_frame = 3,
        .expected_speed = 1,
        .expected_position_diff_from_origin = { -85.5163F, -22.7913F } });
    simulation.RunFor(70);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    assert(argc >= 1);
    auto arguments = std::span(argv, argc);
    test_file_path = arguments[0];
    test_file_path = test_file_path.remove_filename();
    return RUN_ALL_TESTS();
}
