#include "SoldierMovementSimulation.hpp"
#include "core/physics/PhysicsEvents.hpp"
#include "core/state/Control.hpp"
#include <gtest/gtest.h>

#include <cmath>

namespace SoldankTesting
{
SoldierMovementSimulation::SoldierMovementSimulation(const Soldank::IFileReader& file_reader)
{
    Soldank::State& state = state_manager_.GetState();
    auto map =
      SoldankTesting::MapBuilder::Empty()
        ->AddPolygon(
          { 0.0F, 0.0F }, { 100.0F, 0.0F }, { 50.0F, 50.0F }, Soldank::PMSPolygonType::Normal)
        ->Build();
    state.map = *map;
    animation_data_manager_.LoadAllAnimationDatas(file_reader);
    std::vector<Soldank::Weapon> weapons{
        { Soldank::WeaponParametersFactory::GetParameters(
          Soldank::WeaponType::DesertEagles, false, file_reader) },
        { Soldank::WeaponParametersFactory::GetParameters(
          Soldank::WeaponType::Knife, false, file_reader) },
        { Soldank::WeaponParametersFactory::GetParameters(
          Soldank::WeaponType::FragGrenade, false, file_reader) }
    };
    state.soldiers.emplace_back(
      0,
      animation_data_manager_,
      Soldank::ParticleSystem::Load(Soldank::ParticleSystemType::Soldier, 4.5F, file_reader),
      weapons);
    state.soldiers.back().particle.position = glm::vec2{ 0.0F, -29.0F };
    state.soldiers.back().particle.old_position = glm::vec2{ 0.0F, -29.0F };
}

void SoldierMovementSimulation::HoldRight()
{
    state_manager_.ChangeSoldierControlActionState(0, Soldank::ControlActionType::MoveRight, true);
}

void SoldierMovementSimulation::HoldLeft()
{
    state_manager_.ChangeSoldierControlActionState(0, Soldank::ControlActionType::MoveLeft, true);
}

void SoldierMovementSimulation::HoldJump()
{
    state_manager_.ChangeSoldierControlActionState(0, Soldank::ControlActionType::Jump, true);
}

void SoldierMovementSimulation::HoldRightAt(unsigned int tick)
{
    AddControlToChangeAt(
      tick, { .control_type = Soldank::ControlActionType::MoveRight, .new_state = true });
}

void SoldierMovementSimulation::HoldLeftAt(unsigned int tick)
{
    AddControlToChangeAt(
      tick, { .control_type = Soldank::ControlActionType::MoveLeft, .new_state = true });
}

void SoldierMovementSimulation::HoldJumpAt(unsigned int tick)
{
    AddControlToChangeAt(tick,
                         { .control_type = Soldank::ControlActionType::Jump, .new_state = true });
}

void SoldierMovementSimulation::LookLeft()
{
    is_soldier_looking_left_ = true;
    TurnSoldierLeft();
}

void SoldierMovementSimulation::LookRight()
{
    is_soldier_looking_left_ = false;
    TurnSoldierRight();
}

void SoldierMovementSimulation::HoldJetsAt(unsigned int tick)
{
    AddControlToChangeAt(
      tick, { .control_type = Soldank::ControlActionType::UseJets, .new_state = true });
}

void SoldierMovementSimulation::ReleaseRightAt(unsigned int tick)
{
    AddControlToChangeAt(
      tick, { .control_type = Soldank::ControlActionType::MoveRight, .new_state = false });
}

void SoldierMovementSimulation::ReleaseLeftAt(unsigned int tick)
{
    AddControlToChangeAt(
      tick, { .control_type = Soldank::ControlActionType::MoveLeft, .new_state = false });
}

void SoldierMovementSimulation::ReleaseJumpAt(unsigned int tick)
{
    AddControlToChangeAt(tick,
                         { .control_type = Soldank::ControlActionType::Jump, .new_state = false });
}

void SoldierMovementSimulation::ReleaseJetsAt(unsigned int tick)
{
    AddControlToChangeAt(
      tick, { .control_type = Soldank::ControlActionType::UseJets, .new_state = false });
}

void SoldierMovementSimulation::AddSoldierExpectedAnimationState(
  unsigned int tick,
  const SoldierExpectedAnimationState& soldier_expected_animation_state)
{
    if (!animations_to_check_at_tick_.contains(tick)) {
        animations_to_check_at_tick_[tick] = {};
    }

    animations_to_check_at_tick_.at(tick).push_back(soldier_expected_animation_state);
}

void SoldierMovementSimulation::RunUntilSoldierOnGround()
{
    static float gravity = 0.06F;

    Soldank::State& state = state_manager_.GetState();
    auto& current_soldier = *state.soldiers.begin();
    while (!current_soldier.on_ground) {
        std::vector<Soldank::BulletParams> bullet_emitter;
        Soldank::PhysicsEvents physics_events;
        Soldank::SoldierPhysics::Update(
          state, current_soldier, physics_events, animation_data_manager_, bullet_emitter, gravity);
    }
}

void SoldierMovementSimulation::RunFor(unsigned int ticks_to_run)
{
    // TODO: Move this somewhere else
    static float gravity = 0.06F;

    Soldank::State& state = state_manager_.GetState();
    auto& current_soldier = *state.soldiers.begin();
    glm::vec2 soldier_position_origin = current_soldier.particle.position;
    for (unsigned int current_tick = 0; current_tick < ticks_to_run; current_tick++) {
        if (controls_to_change_at_tick_.contains(current_tick)) {
            const auto& controls_to_change = controls_to_change_at_tick_.at(current_tick);
            for (const auto& control_to_change : controls_to_change) {
                state_manager_.ChangeSoldierControlActionState(
                  current_soldier.id, control_to_change.control_type, control_to_change.new_state);
            }
        }
        if (is_soldier_looking_left_) {
            TurnSoldierLeft();
        } else {
            TurnSoldierRight();
        }

        std::vector<Soldank::BulletParams> bullet_emitter;
        Soldank::PhysicsEvents physics_events;
        Soldank::SoldierPhysics::Update(
          state, current_soldier, physics_events, animation_data_manager_, bullet_emitter, gravity);

        if (animations_to_check_at_tick_.contains(current_tick)) {
            CheckSoldierAnimationStates(current_soldier,
                                        animations_to_check_at_tick_.at(current_tick),
                                        soldier_position_origin);
        }
    }
}

void SoldierMovementSimulation::CheckSoldierAnimationStates(
  const Soldank::Soldier& soldier,
  const SoldierExpectedAnimationStates& expected_animation_states,
  const glm::vec2& soldier_position_origin)
{
    for (const auto& expected_animation_state : expected_animation_states) {
        CheckSoldierAnimationState(soldier, expected_animation_state, soldier_position_origin);
    }
}

void SoldierMovementSimulation::CheckSoldierAnimationState(
  const Soldank::Soldier& soldier,
  const SoldierExpectedAnimationState& expected_animation_state,
  const glm::vec2& soldier_position_origin)
{
    switch (expected_animation_state.part) {
        case SoldierAnimationPart::Legs: {
            EXPECT_EQ(soldier.legs_animation->GetType(),
                      expected_animation_state.expected_animation_type);
            EXPECT_EQ(soldier.legs_animation->GetFrame(), expected_animation_state.expected_frame);
            EXPECT_EQ(soldier.legs_animation->GetSpeed(), expected_animation_state.expected_speed);
            break;
        }
        case SoldierAnimationPart::Body: {
            EXPECT_EQ(soldier.body_animation->GetType(),
                      expected_animation_state.expected_animation_type);
            EXPECT_EQ(soldier.body_animation->GetFrame(), expected_animation_state.expected_frame);
            EXPECT_EQ(soldier.body_animation->GetSpeed(), expected_animation_state.expected_speed);
            break;
        }
    }

    float soldier_position_diff_from_origin_x =
      soldier.particle.position.x - soldier_position_origin.x;
    float soldier_position_diff_from_origin_y =
      soldier.particle.position.y - soldier_position_origin.y;
    float expected_position_diff_from_origin_x =
      expected_animation_state.expected_position_diff_from_origin.x;
    float expected_position_diff_from_origin_y =
      expected_animation_state.expected_position_diff_from_origin.y;

    // I think there are some issues with inaccuracy of float so we are rounding so much...
    // TODO: Look if we can fix it somehow and make it more accurate...
    EXPECT_LE(std::abs(soldier_position_diff_from_origin_x - expected_position_diff_from_origin_x),
              1.5);
    EXPECT_LE(std::abs(soldier_position_diff_from_origin_y - expected_position_diff_from_origin_y),
              1.5);
}

void SoldierMovementSimulation::AddControlToChangeAt(unsigned int tick,
                                                     const ControlToChange& control_to_change)
{
    if (!controls_to_change_at_tick_.contains(tick)) {
        controls_to_change_at_tick_[tick] = {};
    }

    controls_to_change_at_tick_.at(tick).push_back(control_to_change);
}

void SoldierMovementSimulation::TurnSoldierLeft()
{
    state_manager_.ChangeSoldierMouseMapPosition(0, { -6400.0F, 0.0F });
}

void SoldierMovementSimulation::TurnSoldierRight()
{
    state_manager_.ChangeSoldierMouseMapPosition(0, { 6400.0F, 0.0F });
}
} // namespace SoldankTesting
