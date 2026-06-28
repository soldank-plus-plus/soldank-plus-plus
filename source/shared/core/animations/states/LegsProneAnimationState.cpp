module;

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

export module Shared.Core.Animations.States:LegsProneAnimationState;

import Shared.Core.Animations;
import :CommonAnimationStateTransitions;
import Shared.Core.Entities.Weapon;
import Shared.Core.Types.WeaponType;

import Shared.Core.Physics.Constants;

export namespace Soldank
{
class LegsProneAnimationState final : public Soldank::AnimationState
{
public:
    LegsProneAnimationState(const AnimationDataManager& animation_data_manager)
        : AnimationState(animation_data_manager.Get(AnimationType::Prone))
    {
    }

    ~LegsProneAnimationState() override = default;

    std::optional<AnimationState::Transition> HandleInput(HandleInputParams& params) final;

    void Update(UpdateParams& params) final { params.stance = PhysicsConstants::STANCE_PRONE; }

private:
};
} // namespace Soldank

namespace Soldank
{
std::optional<AnimationState::Transition> LegsProneAnimationState::HandleInput(
  HandleInputParams& params)
{

    // Keep the prone direction in sync until turning around is allowed to cancel prone.
    if (GetFrame() <= 23) {
        params.old_direction = params.direction;
    }

    if (GetFrame() > 23) {
        if (params.control.prone || params.direction != params.old_direction) {
            return AnimationState::Transition{ AnimationType::GetUp, 9 };
        }

        if (params.on_ground) {
            auto maybe_rolling_animation_state =
              CommonAnimationStateTransitions::TryTransitionToRolling(params);
            if (maybe_rolling_animation_state.has_value()) {
                return *maybe_rolling_animation_state;
            }
        }
    }

    if (GetFrame() > 25 && params.on_ground) {
        if (params.control.left || params.control.right) {
            return AnimationState::Transition{ AnimationType::ProneMove, std::nullopt };
        }
    }

    return std::nullopt;
}
} // namespace Soldank
