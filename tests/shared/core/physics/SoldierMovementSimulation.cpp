module;

#include "core/math/Glm.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <memory>
#include <unordered_map>
#include <vector>

export module Tests.Shared.Core.Physics.SoldierMovementSimulation;

import Shared.Core.Physics.SoldierPhysics;
import Shared.Core.State.StateManager;
import Shared.Core.Physics.PhysicsEvents;
import Shared.Core.Physics.Particles;
import Shared.Core.State.Control;
import Shared.Core.Animations;
import Shared.Core.Data.IFileReader;
import Shared.Core.Entities.Bullet;
import Shared.Core.Entities.Soldier;
import Shared.Core.Entities.Weapon;
import Shared.Core.Entities.WeaponParametersFactory;
import Shared.Core.Map.PMSEnums;
import Shared.Core.Types.WeaponType;

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

    void AddControlToChangeAt(unsigned int tick, const ControlToChange& control_to_change);

    void TurnSoldierLeft();
    void TurnSoldierRight();

    std::unique_ptr<Soldank::StateManager> state_manager_;
    Soldank::AnimationDataManager animation_data_manager_;

    std::unordered_map<unsigned int, SoldierExpectedAnimationStates> animations_to_check_at_tick_;
    std::unordered_map<unsigned int, std::vector<ControlToChange>> controls_to_change_at_tick_;
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
          { 0.0F, 0.0F }, { 100.0F, 0.0F }, { 50.0F, 50.0F }, Soldank::PMSPolygonType::Normal)
        ->Build();
    animation_data_manager_.LoadAllAnimationDatas(file_reader);
    state_manager_ = std::make_unique<Soldank::StateManager>(
      animation_data_manager_,
      Soldank::ParticleSystem::Load(Soldank::ParticleSystemType::Soldier, 4.5F, file_reader));
    state_manager_->OverrideMap(*map);
    std::vector<Soldank::Weapon> weapons{
        { Soldank::WeaponParametersFactory::GetParameters(
          Soldank::WeaponType::DesertEagles, false, file_reader) },
        { Soldank::WeaponParametersFactory::GetParameters(
          Soldank::WeaponType::Knife, false, file_reader) },
        { Soldank::WeaponParametersFactory::GetParameters(
          Soldank::WeaponType::FragGrenade, false, file_reader) }
    };
    state_manager_->CreateSoldier(0);
    state_manager_->SetSoldierPosition(0, { 0.0F, -29.0F });
}

void SoldierMovementSimulation::HoldRight()
{
    state_manager_->ChangeSoldierControlActionState(0, Soldank::ControlActionType::MoveRight, true);
}

void SoldierMovementSimulation::HoldLeft()
{
    state_manager_->ChangeSoldierControlActionState(0, Soldank::ControlActionType::MoveLeft, true);
}

void SoldierMovementSimulation::HoldJump()
{
    state_manager_->ChangeSoldierControlActionState(0, Soldank::ControlActionType::Jump, true);
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

void SoldierMovementSimulation::RunUntilSoldierOnGround(unsigned int ticks_limit)
{
    static float gravity = 0.06F;
    ticks_limit = 5;
    unsigned int ticks = 0;

    const auto& current_soldier = state_manager_->GetSoldier(0);
    while (!current_soldier.on_ground) {
        std::vector<Soldank::BulletParams> bullet_emitter;
        Soldank::PhysicsEvents physics_events;
        state_manager_->TransformSoldier(0, [&](auto& soldier) {
            Soldank::SoldierPhysics::Update(*state_manager_,
                                            soldier,
                                            physics_events,
                                            animation_data_manager_,
                                            bullet_emitter,
                                            gravity);
        });
        ++ticks;
        if (ticks == ticks_limit) {
            break;
        }
    }
}

void SoldierMovementSimulation::RunFor(unsigned int ticks_to_run)
{
    // TODO: Move this somewhere else
    static float gravity = 0.06F;

    const auto& current_soldier = state_manager_->GetSoldier(0);
    glm::vec2 soldier_position_origin = current_soldier.particle.position;
    for (unsigned int current_tick = 0; current_tick < ticks_to_run; current_tick++) {
        if (controls_to_change_at_tick_.contains(current_tick)) {
            const auto& controls_to_change = controls_to_change_at_tick_.at(current_tick);
            for (const auto& control_to_change : controls_to_change) {
                state_manager_->ChangeSoldierControlActionState(
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
        state_manager_->TransformSoldier(0, [&](auto& soldier) {
            Soldank::SoldierPhysics::Update(*state_manager_,
                                            soldier,
                                            physics_events,
                                            animation_data_manager_,
                                            bullet_emitter,
                                            gravity);
        });

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
    state_manager_->ChangeSoldierMouseMapPosition(0, { -6400.0F, 0.0F });
}

void SoldierMovementSimulation::TurnSoldierRight()
{
    state_manager_->ChangeSoldierMouseMapPosition(0, { 6400.0F, 0.0F });
}
} // namespace SoldankTesting
