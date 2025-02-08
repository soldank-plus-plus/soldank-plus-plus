#include "core/animations/states/LegsJumpSideAnimationState.hpp"

#include "core/animations/states/LegsCrouchAnimationState.hpp"
#include "core/animations/states/LegsRollBackAnimationState.hpp"
#include "core/animations/states/LegsStandAnimationState.hpp"
#include "core/animations/states/LegsFallAnimationState.hpp"
#include "core/animations/states/LegsRunBackAnimationState.hpp"
#include "core/animations/states/LegsRunAnimationState.hpp"
#include "core/animations/states/LegsJumpAnimationState.hpp"
#include "core/animations/states/LegsProneAnimationState.hpp"

#include "core/animations/states/CommonAnimationStateTransitions.hpp"

#include "core/entities/Soldier.hpp"
#include "core/physics/Constants.hpp"

namespace Soldank
{
LegsJumpSideAnimationState::LegsJumpSideAnimationState(
  const AnimationDataManager& animation_data_manager)
    : AnimationState(animation_data_manager.Get(AnimationType::JumpSide))
    , animation_data_manager_(animation_data_manager)
{
}

std::optional<std::shared_ptr<AnimationState>> LegsJumpSideAnimationState::HandleInput(
  Soldier& soldier)
{
    if (!soldier.control.left || !soldier.control.right) {
        soldier.control.was_running_left = soldier.control.left;
    }

    bool jumping_direction_left = soldier.control.was_running_left;

    if (soldier.control.prone) {
        return std::make_shared<LegsProneAnimationState>(animation_data_manager_);
    }

    if (soldier.control.jets) {
        if ((soldier.control.left && soldier.direction == 1) ||
            (soldier.control.right && soldier.direction == -1)) {
            return std::make_shared<LegsRollBackAnimationState>(animation_data_manager_);
        }

        if (soldier.jets_count > 0) {
            if (soldier.on_ground) {
                return std::make_shared<LegsStandAnimationState>(animation_data_manager_);
            }
            return std::make_shared<LegsFallAnimationState>(animation_data_manager_);
        }
    }

    if (!soldier.control.up && !soldier.control.down && !soldier.control.left &&
        !soldier.control.right) {
        if (soldier.on_ground) {
            return std::make_shared<LegsStandAnimationState>(animation_data_manager_);
        }
        return std::make_shared<LegsFallAnimationState>(animation_data_manager_);
    }

    if (!soldier.control.up && !soldier.control.down) {
        auto maybe_running_animation_state =
          CommonAnimationStateTransitions::TryTransitionToRunning(soldier, animation_data_manager_);
        if (maybe_running_animation_state.has_value()) {
            return *maybe_running_animation_state;
        }
    }

    if (soldier.control.up && soldier.on_ground) {
        if (!soldier.control.left && !soldier.control.right) {
            return std::make_shared<LegsJumpAnimationState>(animation_data_manager_);
        }

        if (soldier.control.left || soldier.control.right) {
            // Chain jumping to the side
            if (GetFrame() == GetFramesCount()) {
                return std::make_shared<LegsJumpSideAnimationState>(animation_data_manager_);
            }
        }
    }

    if (soldier.control.down && soldier.on_ground) {
        if (!soldier.control.left && !soldier.control.right) {
            return std::make_shared<LegsCrouchAnimationState>(animation_data_manager_);
        }

        auto maybe_crouch_running_animation_state =
          CommonAnimationStateTransitions::TryTransitionToCrouchRunning(soldier,
                                                                        animation_data_manager_);
        if (maybe_crouch_running_animation_state.has_value()) {
            return *maybe_crouch_running_animation_state;
        }
    }

    if (!jumping_direction_left) {
        if ((GetFrame() > 3) && (GetFrame() < 11)) {
            glm::vec2 particle_force = soldier.particle.GetForce();
            particle_force.x = PhysicsConstants::JUMPDIRSPEED;
            particle_force.y = -PhysicsConstants::JUMPDIRSPEED / 1.2F;
            soldier.particle.SetForce(particle_force);
        }
    }

    if (jumping_direction_left) {
        if (GetType() == AnimationType::JumpSide) {
            if ((GetFrame() > 3) && (GetFrame() < 11)) {
                glm::vec2 particle_force = soldier.particle.GetForce();
                particle_force.x = -PhysicsConstants::JUMPDIRSPEED;
                particle_force.y = -PhysicsConstants::JUMPDIRSPEED / 1.2F;
                soldier.particle.SetForce(particle_force);
            }
        }
    }

    return std::nullopt;
}

void LegsJumpSideAnimationState::Update(Soldier& soldier, const PhysicsEvents& /*physics_events*/)
{
    soldier.stance = PhysicsConstants::STANCE_STAND;
}
} // namespace Soldank
