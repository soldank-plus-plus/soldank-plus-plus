#include "core/animations/states/LegsFallAnimationState.hpp"

#include "core/animations/states/LegsStandAnimationState.hpp"
#include "core/animations/states/LegsRunBackAnimationState.hpp"
#include "core/animations/states/LegsRunAnimationState.hpp"
#include "core/animations/states/LegsProneAnimationState.hpp"

#include "core/physics/Constants.hpp"
#include "core/entities/Soldier.hpp"

namespace Soldank
{
LegsFallAnimationState::LegsFallAnimationState(const AnimationDataManager& animation_data_manager)
    : AnimationState(animation_data_manager.Get(AnimationType::Fall))
    , animation_data_manager_(animation_data_manager)
{
}

std::optional<std::shared_ptr<AnimationState>> LegsFallAnimationState::HandleInput(Soldier& soldier)
{
    // TODO: Transition from fall to jump when control.up and on_ground

    if (soldier.control.prone) {
        return std::make_shared<LegsProneAnimationState>(animation_data_manager_);
    }

    if (soldier.control.right) {
        if (soldier.direction == -1) {
            soldier.legs_animation_state_machine = std::make_shared<LegsRunBackAnimationState>(
              animation_data_manager_, soldier.control.left, soldier.control.right);
            return std::nullopt;
        }

        soldier.legs_animation_state_machine = std::make_shared<LegsRunAnimationState>(
          animation_data_manager_, soldier.control.left, soldier.control.right);
        return std::nullopt;
    }

    if (soldier.control.left) {
        if (soldier.direction == 1) {
            soldier.legs_animation_state_machine = std::make_shared<LegsRunBackAnimationState>(
              animation_data_manager_, soldier.control.left, soldier.control.right);
            return std::nullopt;
        }

        soldier.legs_animation_state_machine = std::make_shared<LegsRunAnimationState>(
          animation_data_manager_, soldier.control.left, soldier.control.right);
        return std::nullopt;
    }

    if (soldier.on_ground) {
        soldier.legs_animation_state_machine =
          std::make_shared<LegsStandAnimationState>(animation_data_manager_);
        return std::nullopt;
    }
    return std::nullopt;
}

void LegsFallAnimationState::Update(Soldier& soldier)
{
    soldier.stance = 1;
}
} // namespace Soldank