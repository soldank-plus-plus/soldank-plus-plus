module;

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

export module Shared.Core.Animations.States:LegsCrouchRunAnimationState;

import Shared.Core.Animations;
import :CommonAnimationStateTransitions;
import Shared.Core.Entities.Weapon;
import Shared.Core.Types.WeaponType;

import Shared.Core.Physics.Constants;

export namespace Soldank
{
class LegsCrouchRunAnimationState final : public Soldank::AnimationState
{
public:
    LegsCrouchRunAnimationState(const AnimationDataManager& animation_data_manager)
        : AnimationState(animation_data_manager.Get(AnimationType::CrouchRun))
    {
    }

    ~LegsCrouchRunAnimationState() override = default;

    std::optional<AnimationState::Transition> HandleInput(HandleInputParams& params) final;

    void Update(UpdateParams& params) final
    {
        params.stance = PhysicsConstants::STANCE_CROUCH;

        if (GetSpeed() > 2) {
            params.velocity.x /= (float)GetSpeed();
            params.velocity.y /= (float)GetSpeed();
        }

        if (params.on_ground) {
            if (params.control.right && params.direction == 1) {
                params.force.x = PhysicsConstants::CROUCHRUNSPEED;
            } else if (params.control.left && params.direction == -1) {
                params.force.x = -PhysicsConstants::CROUCHRUNSPEED;
            }
        }
    }

private:
};
} // namespace Soldank

namespace Soldank
{
std::optional<AnimationState::Transition> LegsCrouchRunAnimationState::HandleInput(
  HandleInputParams& params)
{

    if (params.control.prone) {
        return AnimationState::Transition{ AnimationType::Prone, std::nullopt };
    }

    if (!params.control.down) {
        auto maybe_running_animation_state =
          CommonAnimationStateTransitions::TryTransitionToRunning(params);
        if (maybe_running_animation_state.has_value()) {
            return *maybe_running_animation_state;
        }

        if (params.on_ground) {
            return AnimationState::Transition{ AnimationType::Stand, std::nullopt };
        }

        return AnimationState::Transition{ AnimationType::Fall, std::nullopt };
    }

    if (params.control.jets && params.jets_count > 0) {
        if (params.on_ground) {
            auto maybe_rolling_animation_state =
              CommonAnimationStateTransitions::TryTransitionToRolling(params);
            if (maybe_rolling_animation_state.has_value()) {
                return *maybe_rolling_animation_state;
            }
        }

        return AnimationState::Transition{ AnimationType::Fall, std::nullopt };
    }

    if (!params.control.left && !params.control.right) {
        return AnimationState::Transition{ AnimationType::Crouch, std::nullopt };
    }

    if (params.control.right && params.direction == -1) {
        return AnimationState::Transition{ AnimationType::CrouchRunBack, std::nullopt };
    }

    if (params.control.left && params.direction == 1) {
        return AnimationState::Transition{ AnimationType::CrouchRunBack, std::nullopt };
    }

    return std::nullopt;
}
} // namespace Soldank
