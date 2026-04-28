module;

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

export module Shared.Core.Animations.States:LegsProneMoveAnimationState;

import Shared.Core.Animations;
import :CommonAnimationStateTransitions;
import Shared.Core.Entities.Weapon;
import Shared.Core.Types.WeaponType;

import Shared.Core.Physics.Constants;

export namespace Soldank
{
class LegsProneMoveAnimationState final : public Soldank::AnimationState
{
public:
    LegsProneMoveAnimationState(const AnimationDataManager& animation_data_manager)
        : AnimationState(animation_data_manager.Get(AnimationType::ProneMove))
    {
    }

    ~LegsProneMoveAnimationState() override = default;

    std::optional<AnimationState::Transition> HandleInput(HandleInputParams& params) final;

    void Update(UpdateParams& params) final
    {
        params.stance = PhysicsConstants::STANCE_PRONE;

        if (GetSpeed() > 2) {
            params.velocity.x /= (float)GetSpeed();
            params.velocity.y /= (float)GetSpeed();
        }

        if ((GetFrame() < 4) || (GetFrame() > 14)) {
            if (params.on_ground) {
                if (params.control.left) {
                    params.force.x = -PhysicsConstants::PRONESPEED;
                } else {
                    params.force.x = PhysicsConstants::PRONESPEED;
                }
            }
        }
    }

private:
};
} // namespace Soldank

namespace Soldank
{
std::optional<AnimationState::Transition> LegsProneMoveAnimationState::HandleInput(
  HandleInputParams& params)
{

    if (!params.control.left && !params.control.right) {
        return AnimationState::Transition{ AnimationType::Prone, 26 };
    }

    if (params.on_ground) {
        auto maybe_rolling_animation_state =
          CommonAnimationStateTransitions::TryTransitionToRolling(params);
        if (maybe_rolling_animation_state.has_value()) {
            return *maybe_rolling_animation_state;
        }
    }

    if (params.control.prone || params.direction != params.old_direction) {
        return AnimationState::Transition{ AnimationType::GetUp, 9 };
    }

    return std::nullopt;
}
} // namespace Soldank
