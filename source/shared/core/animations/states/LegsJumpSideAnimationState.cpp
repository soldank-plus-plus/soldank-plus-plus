module;

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

export module Shared.Core.Animations.States:LegsJumpSideAnimationState;

import Shared.Core.Animations;
import :CommonAnimationStateTransitions;
import Shared.Core.Entities.Weapon;
import Shared.Core.Types.WeaponType;

import Shared.Core.Physics.Constants;

export namespace Soldank
{
class LegsJumpSideAnimationState final : public Soldank::AnimationState
{
public:
    LegsJumpSideAnimationState(const AnimationDataManager& animation_data_manager)
        : AnimationState(animation_data_manager.Get(AnimationType::JumpSide))
    {
    }

    ~LegsJumpSideAnimationState() override = default;

    std::optional<AnimationState::Transition> HandleInput(HandleInputParams& params) final;

    void Update(UpdateParams& params) final
    {
        params.stance = PhysicsConstants::STANCE_STAND;

        if (!params.control.was_running_left) {
            if ((GetFrame() > 3) && (GetFrame() < 11)) {
                params.force.x = PhysicsConstants::JUMPDIRSPEED;
                params.force.y = -PhysicsConstants::JUMPDIRSPEED / 1.2F;
            }
        }

        if (params.control.was_running_left) {
            if (GetType() == AnimationType::JumpSide) {
                if ((GetFrame() > 3) && (GetFrame() < 11)) {
                    params.force.x = -PhysicsConstants::JUMPDIRSPEED;
                    params.force.y = -PhysicsConstants::JUMPDIRSPEED / 1.2F;
                }
            }
        }
    }

private:
};
} // namespace Soldank

namespace Soldank
{
std::optional<AnimationState::Transition> LegsJumpSideAnimationState::HandleInput(
  HandleInputParams& params)
{

    if (!params.control.left || !params.control.right) {
        params.control.was_running_left = params.control.left;
    }

    if (params.control.prone) {
        return AnimationState::Transition{ AnimationType::Prone, std::nullopt };
    }

    if (params.control.jets) {
        if ((params.control.left && params.direction == 1) ||
            (params.control.right && params.direction == -1)) {
            return AnimationState::Transition{ AnimationType::RollBack, std::nullopt };
        }

        if (params.jets_count > 0) {
            if (params.on_ground) {
                return AnimationState::Transition{ AnimationType::Stand, std::nullopt };
            }
            return AnimationState::Transition{ AnimationType::Fall, std::nullopt };
        }
    }

    if (!params.control.up && !params.control.down && !params.control.left &&
        !params.control.right) {
        if (params.on_ground) {
            return AnimationState::Transition{ AnimationType::Stand, std::nullopt };
        }
        return AnimationState::Transition{ AnimationType::Fall, std::nullopt };
    }

    if (!params.control.up && !params.control.down) {
        auto maybe_running_animation_state =
          CommonAnimationStateTransitions::TryTransitionToRunning(params);
        if (maybe_running_animation_state.has_value()) {
            return *maybe_running_animation_state;
        }
    }

    if (params.control.up && params.on_ground) {
        if (!params.control.left && !params.control.right) {
            return AnimationState::Transition{ AnimationType::Jump, std::nullopt };
        }

        if (params.control.left || params.control.right) {
            // Chain jumping to the side
            if (GetFrame() == GetFramesCount()) {
                return AnimationState::Transition{ AnimationType::JumpSide, std::nullopt };
            }
        }
    }

    if (params.control.down && params.on_ground) {
        if (!params.control.left && !params.control.right) {
            return AnimationState::Transition{ AnimationType::Crouch, std::nullopt };
        }

        auto maybe_crouch_running_animation_state =
          CommonAnimationStateTransitions::TryTransitionToCrouchRunning(params);
        if (maybe_crouch_running_animation_state.has_value()) {
            return *maybe_crouch_running_animation_state;
        }
    }

    return std::nullopt;
}
} // namespace Soldank
