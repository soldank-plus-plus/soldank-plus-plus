module;

#include "core/math/Glm.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <map>
#include <memory>
#include <vector>

export module Tests.Shared.Core.Physics.SoldierMovementSimulation;

import Shared.Core.Physics.SoldierPhysics;
import Shared.Core.State.StateManager;
import Shared.Core.Physics.PhysicsEvents;
import Shared.Core.Physics.Particles;
import Shared.Core.Simulation.PlayerInputApplication;
import Shared.Core.Simulation.SimulationCommands;
import Shared.Core.State.Control;
import Shared.Core.Animations;
import Shared.Core.Data.IFileReader;
import Shared.Core.Entities.Bullet;
import Shared.Core.Entities.Soldier;
import Shared.Core.Map.PMSEnums;

import Testing.Framework.Shared.MapBuilder;

export namespace SoldankTesting
{
enum class SoldierAnimationPart
{
    Legs = 0,
    Body
};

struct SoldierExpectedAnimationState
{
    SoldierAnimationPart part;
    Soldank::AnimationType expected_animation_type;
    unsigned int expected_frame;
    int expected_speed;
    glm::vec2 expected_position_diff_from_origin;
};

struct ControlToChange
{
    Soldank::ControlActionType control_type;
    bool new_state;
};

class SoldierMovementSimulation
{
public:
    SoldierMovementSimulation(const Soldank::IFileReader& file_reader);

    void HoldRight();
    void HoldLeft();
    void HoldJump();

    void HoldRightAt(unsigned int tick);
    void HoldLeftAt(unsigned int tick);
    void HoldJumpAt(unsigned int tick);
    void HoldJetsAt(unsigned int tick);
    void ReleaseRightAt(unsigned int tick);
    void ReleaseLeftAt(unsigned int tick);
    void ReleaseJumpAt(unsigned int tick);
    void ReleaseJetsAt(unsigned int tick);

    void LookLeft();
    void LookRight();

    void ApplyPlayerInput(const Soldank::PlayerInputCommand& command);
    const Soldank::Control& GetSoldierControl() const;

    void AddSoldierExpectedAnimationState(
      unsigned int tick,
      const SoldierExpectedAnimationState& soldier_expected_animation_state);

    void RunUntilSoldierOnGround(unsigned int ticks_limit = 5000);

    void RunFor(unsigned int ticks_to_run);

private:
    using SoldierExpectedAnimationStates = std::vector<SoldierExpectedAnimationState>;

    static void CheckSoldierAnimationStates(
      const Soldank::Soldier& soldier,
      const SoldierExpectedAnimationStates& expected_animation_states,
      const glm::vec2& soldier_position_origin);

    static void CheckSoldierAnimationState(
      const Soldank::Soldier& soldier,
      const SoldierExpectedAnimationState& expected_animation_state,
      const glm::vec2& soldier_position_origin);

    void RunPhysicsStep();
    void AddControlToChangeAt(unsigned int tick, const ControlToChange& control_to_change);

    void TurnSoldierLeft();
    void TurnSoldierRight();

    static constexpr std::uint8_t SOLDIER_ID = 0;
    static constexpr float GRAVITY = 0.06F;

    std::unique_ptr<Soldank::StateManager> state_manager_;
    Soldank::AnimationDataManager animation_data_manager_;

    std::map<unsigned int, SoldierExpectedAnimationStates> animations_to_check_at_tick_;
    std::map<unsigned int, std::vector<ControlToChange>> controls_to_change_at_tick_;
    bool is_soldier_looking_left_ = false;
};
} // namespace SoldankTesting

export namespace SoldankTesting
{
SoldierMovementSimulation::SoldierMovementSimulation(const Soldank::IFileReader& file_reader)
{
    auto map =
      SoldankTesting::MapBuilder::Empty()
        ->AddPolygon(
          { -300.0F, 0.0F }, { 300.0F, 0.0F }, { 300.0F, 60.0F }, Soldank::PMSPolygonType::Normal)
        ->AddPolygon(
          { -300.0F, 0.0F }, { 300.0F, 60.0F }, { -300.0F, 60.0F }, Soldank::PMSPolygonType::Normal)
        ->Build();
    animation_data_manager_.LoadAllAnimationDatas(file_reader);
    state_manager_ = std::make_unique<Soldank::StateManager>(
      animation_data_manager_,
      Soldank::ParticleSystem::Load(Soldank::ParticleSystemType::Soldier, 4.5F, file_reader));
    state_manager_->OverrideMap(*map);
    state_manager_->CreateSoldier(0);
    state_manager_->SpawnSoldier(0, glm::vec2{ 0.0F, -34.0F });
}

void SoldierMovementSimulation::HoldRight()
{
    state_manager_->ChangeSoldierControlActionState(
      SOLDIER_ID, Soldank::ControlActionType::MoveRight, true);
}

void SoldierMovementSimulation::HoldLeft()
{
    state_manager_->ChangeSoldierControlActionState(
      SOLDIER_ID, Soldank::ControlActionType::MoveLeft, true);
}

void SoldierMovementSimulation::HoldJump()
{
    state_manager_->ChangeSoldierControlActionState(
      SOLDIER_ID, Soldank::ControlActionType::Jump, true);
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

void SoldierMovementSimulation::ApplyPlayerInput(const Soldank::PlayerInputCommand& command)
{
    Soldank::ApplyPlayerInputCommand(*state_manager_, command);
}

const Soldank::Control& SoldierMovementSimulation::GetSoldierControl() const
{
    return state_manager_->GetSoldier(SOLDIER_ID).control;
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
    animations_to_check_at_tick_[tick].push_back(soldier_expected_animation_state);
}

void SoldierMovementSimulation::RunUntilSoldierOnGround(unsigned int ticks_limit)
{
    static constexpr unsigned int REQUIRED_CONSECUTIVE_GROUNDED_TICKS = 5;
    unsigned int consecutive_grounded_ticks = 0;

    for (unsigned int tick = 0; tick < ticks_limit; ++tick) {
        if (state_manager_->GetSoldier(SOLDIER_ID).on_ground) {
            ++consecutive_grounded_ticks;
            if (consecutive_grounded_ticks >= REQUIRED_CONSECUTIVE_GROUNDED_TICKS) {
                return;
            }
        } else {
            consecutive_grounded_ticks = 0;
        }

        RunPhysicsStep();
    }

    ASSERT_TRUE(state_manager_->GetSoldier(SOLDIER_ID).on_ground)
      << "Soldier did not reach the ground within " << ticks_limit << " ticks";
}

void SoldierMovementSimulation::RunFor(unsigned int ticks_to_run)
{
    glm::vec2 soldier_position_origin = state_manager_->GetSoldier(SOLDIER_ID).particle.position;
    for (unsigned int current_tick = 0; current_tick < ticks_to_run; current_tick++) {
        if (auto controls = controls_to_change_at_tick_.find(current_tick);
            controls != controls_to_change_at_tick_.end()) {
            for (const auto& control_to_change : controls->second) {
                state_manager_->ChangeSoldierControlActionState(
                  SOLDIER_ID, control_to_change.control_type, control_to_change.new_state);
            }
        }
        if (is_soldier_looking_left_) {
            TurnSoldierLeft();
        } else {
            TurnSoldierRight();
        }

        RunPhysicsStep();

        if (auto expected_states = animations_to_check_at_tick_.find(current_tick);
            expected_states != animations_to_check_at_tick_.end()) {
            CheckSoldierAnimationStates(state_manager_->GetSoldier(SOLDIER_ID),
                                        expected_states->second,
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

    EXPECT_NEAR(soldier_position_diff_from_origin_x, expected_position_diff_from_origin_x, 0.01F);
    EXPECT_NEAR(soldier_position_diff_from_origin_y, expected_position_diff_from_origin_y, 0.01F);
}

void SoldierMovementSimulation::AddControlToChangeAt(unsigned int tick,
                                                     const ControlToChange& control_to_change)
{
    controls_to_change_at_tick_[tick].push_back(control_to_change);
}

void SoldierMovementSimulation::RunPhysicsStep()
{
    std::vector<Soldank::BulletParams> bullet_emitter;
    Soldank::PhysicsEvents physics_events;
    state_manager_->TransformSoldier(SOLDIER_ID, [&](auto& soldier) {
        Soldank::SoldierPhysics::Update(*state_manager_,
                                        soldier,
                                        physics_events,
                                        animation_data_manager_,
                                        bullet_emitter,
                                        GRAVITY);
    });
}

void SoldierMovementSimulation::TurnSoldierLeft()
{
    state_manager_->TransformSoldier(SOLDIER_ID, [](auto& soldier) {
        soldier.direction = -1;
        soldier.old_direction = -1;
    });
    state_manager_->ChangeSoldierMouseMapPosition(SOLDIER_ID, { -6400.0F, 0.0F });
}

void SoldierMovementSimulation::TurnSoldierRight()
{
    state_manager_->TransformSoldier(SOLDIER_ID, [](auto& soldier) {
        soldier.direction = 1;
        soldier.old_direction = 1;
    });
    state_manager_->ChangeSoldierMouseMapPosition(SOLDIER_ID, { 6400.0F, 0.0F });
}
} // namespace SoldankTesting
