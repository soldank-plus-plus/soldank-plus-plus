#include "core/animations/states/LegsRunAnimationState.hpp"

#include "core/animations/states/LegsRollAnimationState.hpp"
#include "core/animations/states/LegsStandAnimationState.hpp"
#include "core/animations/states/LegsRunBackAnimationState.hpp"
#include "core/animations/states/LegsFallAnimationState.hpp"
#include "core/animations/states/LegsJumpSideAnimationState.hpp"
#include "core/animations/states/LegsJumpAnimationState.hpp"
#include "core/animations/states/LegsProneAnimationState.hpp"

#include "core/animations/states/CommonAnimationStateTransitions.hpp"

#include "core/entities/Soldier.hpp"

#include "core/physics/Constants.hpp"
#include <spdlog/spdlog.h>

namespace Soldank
{
LegsRunAnimationState::LegsRunAnimationState(const AnimationDataManager& animation_data_manager)
    : AnimationState(animation_data_manager.Get(AnimationType::Run))
    , animation_data_manager_(animation_data_manager)
{
}

std::optional<std::shared_ptr<AnimationState>> LegsRunAnimationState::HandleInput(Soldier& soldier)
{
    if (soldier.control.prone) {
        return std::make_shared<LegsProneAnimationState>(animation_data_manager_);
    }

    if (soldier.on_ground) {
        auto maybe_rolling_animation_state =
          CommonAnimationStateTransitions::TryTransitionToRolling(soldier, animation_data_manager_);
        if (maybe_rolling_animation_state.has_value()) {
            return *maybe_rolling_animation_state;
        }
    }

    if (!soldier.control.left && !soldier.control.right) {
        if (soldier.on_ground) {
            if (soldier.control.up) {
                return std::make_shared<LegsJumpAnimationState>(animation_data_manager_);
            }
            return std::make_shared<LegsStandAnimationState>(animation_data_manager_);
        }

        if (soldier.control.up) {
            return std::nullopt;
        }

        return std::make_shared<LegsFallAnimationState>(animation_data_manager_);
    }

    if (soldier.control.up && soldier.on_ground) {
        soldier.control.was_running_left = soldier.control.left;
        return std::make_shared<LegsJumpSideAnimationState>(animation_data_manager_);
    }

    if (soldier.control.left && soldier.direction == 1) {
        return std::make_shared<LegsRunBackAnimationState>(animation_data_manager_);
    }

    if (soldier.control.right && soldier.direction == -1) {
        return std::make_shared<LegsRunBackAnimationState>(animation_data_manager_);
    }

    // if using jets, reset animation because first frame looks like "directional" jetting
    if (soldier.control.jets && soldier.jets_count > 0) {
        if (soldier.control.up) {
            return std::make_shared<LegsFallAnimationState>(animation_data_manager_);
        }
        return std::make_shared<LegsRunAnimationState>(animation_data_manager_);
    }

    return std::nullopt;
}

void LegsRunAnimationState::Update(Soldier& soldier, const PhysicsEvents& physics_events)
{
    soldier.stance = PhysicsConstants::STANCE_STAND;
    if (soldier.control.left && !soldier.control.up && soldier.direction == -1) {
        glm::vec2 particle_force = soldier.particle.GetForce();
        if (soldier.on_ground) {
            particle_force.x = -PhysicsConstants::RUNSPEED;
            particle_force.y = -PhysicsConstants::RUNSPEEDUP;
        } else {
            particle_force.x = -PhysicsConstants::FLYSPEED;
        }
        soldier.particle.SetForce(particle_force);
    } else if (soldier.control.right && !soldier.control.up && soldier.direction == 1) {
        if (soldier.on_ground) {
            glm::vec2 particle_force = soldier.particle.GetForce();
            particle_force.x = PhysicsConstants::RUNSPEED;
            particle_force.y = -PhysicsConstants::RUNSPEEDUP;
            soldier.particle.SetForce(particle_force);
        } else {
            glm::vec2 particle_force = soldier.particle.GetForce();
            particle_force.x = PhysicsConstants::FLYSPEED;
            soldier.particle.SetForce(particle_force);
        }
    }
}
} // namespace Soldank
