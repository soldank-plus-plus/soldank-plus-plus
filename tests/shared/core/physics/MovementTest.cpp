#include "core/animations/AnimationData.hpp"
#include "shared_lib_testing/MapBuilder.hpp"
#include "core/map/PMSEnums.hpp"
#include "core/map/PMSStructs.hpp"

#include "core/physics/SoldierPhysics.hpp"

#include "core/animations/AnimationState.hpp"
#include "core/entities/Soldier.hpp"
#include "core/entities/WeaponParametersFactory.hpp"
#include "core/state/StateManager.hpp"
#include "core/data/IFileReader.hpp"

#include "SoldierMovementSimulation.hpp"

#include <gtest/gtest.h>

#include <vector>
#include <algorithm>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <span>

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
        .expected_position_diff_from_origin = { 0.118F, -0.0795F } });
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
        .expected_position_diff_from_origin = { -0.118F, -0.0795F } });
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
        .expected_position_diff_from_origin = { 0.0F, 0.0802F } });
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
        .expected_position_diff_from_origin = { -0.118F, -0.0796F } });
    simulation.AddSoldierExpectedAnimationState(
      32,
      { .part = SoldankTesting::SoldierAnimationPart::Legs,
        .expected_animation_type = Soldank::AnimationType::RollBack,
        .expected_frame = 2,
        .expected_speed = 1,
        .expected_position_diff_from_origin = { -49.6132F, -17.4393F } });
    simulation.AddSoldierExpectedAnimationState(
      32,
      { .part = SoldankTesting::SoldierAnimationPart::Body,
        .expected_animation_type = Soldank::AnimationType::RollBack,
        .expected_frame = 2,
        .expected_speed = 1,
        .expected_position_diff_from_origin = { -49.6132F, -17.4393F } });
    simulation.AddSoldierExpectedAnimationState(
      63,
      { .part = SoldankTesting::SoldierAnimationPart::Body,
        .expected_animation_type = Soldank::AnimationType::Stand,
        .expected_frame = 1,
        .expected_speed = 3,
        .expected_position_diff_from_origin = { -104.5802F, -10.6236F } });
    simulation.AddSoldierExpectedAnimationState(
      64,
      { .part = SoldankTesting::SoldierAnimationPart::Legs,
        .expected_animation_type = Soldank::AnimationType::Fall,
        .expected_frame = 3,
        .expected_speed = 1,
        .expected_position_diff_from_origin = { -106.9678F, -9.5901F } });
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
        .expected_position_diff_from_origin = { -0.1181F, -0.0397F } });
    simulation.AddSoldierExpectedAnimationState(
      12,
      { .part = SoldankTesting::SoldierAnimationPart::Legs,
        .expected_animation_type = Soldank::AnimationType::RollBack,
        .expected_frame = 2,
        .expected_speed = 1,
        .expected_position_diff_from_origin = { -11.4988F, -6.4523F } });
    simulation.AddSoldierExpectedAnimationState(
      12,
      { .part = SoldankTesting::SoldierAnimationPart::Body,
        .expected_animation_type = Soldank::AnimationType::RollBack,
        .expected_frame = 2,
        .expected_speed = 1,
        .expected_position_diff_from_origin = { -11.4988F, -6.4523F } });
    simulation.AddSoldierExpectedAnimationState(
      43,
      { .part = SoldankTesting::SoldierAnimationPart::Body,
        .expected_animation_type = Soldank::AnimationType::Stand,
        .expected_frame = 1,
        .expected_speed = 3,
        .expected_position_diff_from_origin = { -82.6433F, -23.2963F } });
    simulation.AddSoldierExpectedAnimationState(
      44,
      { .part = SoldankTesting::SoldierAnimationPart::Legs,
        .expected_animation_type = Soldank::AnimationType::Fall,
        .expected_frame = 3,
        .expected_speed = 1,
        .expected_position_diff_from_origin = { -85.4888F, -22.8577F } });
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
