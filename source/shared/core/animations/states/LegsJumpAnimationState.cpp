module;

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

export module Shared.Core.Animations.States:LegsJumpAnimationState;

import Shared.Core.Animations;
import :CommonAnimationStateTransitions;
import Shared.Core.Entities.Weapon;
import Shared.Core.Types.WeaponType;

import Shared.Core.Physics.Constants;

export namespace Soldank
{
class LegsJumpAnimationState final : public Soldank::AnimationState
{
public:
    LegsJumpAnimationState(const AnimationDataManager& animation_data_manager)
        : AnimationState(animation_data_manager.Get(AnimationType::Jump))
    {
    }

    ~LegsJumpAnimationState() override = default;

    std::optional<AnimationState::Transition> HandleInput(HandleInputParams& params) final;

    void Update(UpdateParams& params) final
    {
        params.stance = PhysicsConstants::STANCE_STAND;

        if ((GetFrame() > 8) && (GetFrame() < 15)) {
            params.force.y = -PhysicsConstants::JUMPSPEED;
        }
    }

private:
};
} // namespace Soldank

namespace Soldank
{
std::optional<AnimationState::Transition> LegsJumpAnimationState::HandleInput(
  HandleInputParams& params)
{

    if (params.control.prone) {
        return AnimationState::Transition{ AnimationType::Prone, std::nullopt };
    }

    if (!params.control.up) {
        if (params.on_ground) {
            return AnimationState::Transition{ AnimationType::Stand, std::nullopt };
        }

        return AnimationState::Transition{ AnimationType::Fall, std::nullopt };
    }

    if (GetFrame() == GetFramesCount()) {
        // If holding A or D, don't change animation. TODO: figure out why soldat does it and
        // improve explanation
        if (!params.control.left && !params.control.right) {
            if (params.on_ground) {
                return AnimationState::Transition{ AnimationType::Stand, std::nullopt };
            }

            return AnimationState::Transition{ AnimationType::Fall, std::nullopt };
        }

        if (params.on_ground) {
            auto maybe_running_animation_state =
              CommonAnimationStateTransitions::TryTransitionToRunning(params);
            if (maybe_running_animation_state.has_value()) {
                return *maybe_running_animation_state;
            }
        }
    }
    return std::nullopt;
}
} // namespace Soldank
