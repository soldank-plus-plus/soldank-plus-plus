#include "core/animations/states/LegsCrouchAnimationState.hpp"

#include "core/animations/states/LegsStandAnimationState.hpp"
#include "core/animations/states/LegsFallAnimationState.hpp"
#include "core/animations/states/LegsRunBackAnimationState.hpp"
#include "core/animations/states/LegsRunAnimationState.hpp"
#include "core/animations/states/LegsJumpAnimationState.hpp"

#include "core/animations/states/CommonAnimationStateTransitions.hpp"

#include "core/entities/Soldier.hpp"
#include "core/physics/Constants.hpp"

namespace Soldank
{
LegsCrouchAnimationState::LegsCrouchAnimationState(
  const AnimationDataManager& animation_data_manager)
    : AnimationState(animation_data_manager.Get(AnimationType::Crouch))
    , animation_data_manager_(animation_data_manager)
{
}

std::optional<std::shared_ptr<AnimationState>> LegsCrouchAnimationState::HandleInput(
  Soldier& soldier)
{
    if (!soldier.control.down) {
        auto maybe_running_animation_state =
          CommonAnimationStateTransitions::TryTransitionToRunning(soldier, animation_data_manager_);
        if (maybe_running_animation_state.has_value()) {
            return *maybe_running_animation_state;
        }

        if (soldier.on_ground) {
            return std::make_shared<LegsStandAnimationState>(animation_data_manager_);
        }

        return std::make_shared<LegsFallAnimationState>(animation_data_manager_);
    }

    if (soldier.control.up && soldier.on_ground) {
        return std::make_shared<LegsJumpAnimationState>(animation_data_manager_);
    }

    return std::nullopt;
}

void LegsCrouchAnimationState::Update(Soldier& soldier)
{
    soldier.stance = PhysicsConstants::STANCE_CROUCH;
}
} // namespace Soldank
