module;

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include <spdlog/spdlog.h>

export module Shared.Core.Animations.States:LegsRunAnimationState;

import Shared.Core.Animations;
import :CommonAnimationStateTransitions;
import Shared.Core.Entities.Weapon;
import Shared.Core.Types.WeaponType;

import Shared.Core.Physics.Constants;

export namespace Soldank
{
class LegsRunAnimationState final : public Soldank::AnimationState
{
public:
    LegsRunAnimationState(const AnimationDataManager& animation_data_manager)
        : AnimationState(animation_data_manager.Get(AnimationType::Run))
    {
    }

    ~LegsRunAnimationState() override = default;

    std::optional<AnimationState::Transition> HandleInput(HandleInputParams& params) final;
    void Update(UpdateParams& params) final
    {
        params.stance = PhysicsConstants::STANCE_STAND;
        if (params.control.left && !params.control.up && params.direction == -1) {
            if (params.on_ground) {
                params.force.x = -PhysicsConstants::RUNSPEED;
                params.force.y = -PhysicsConstants::RUNSPEEDUP;
            } else {
                params.force.x = -PhysicsConstants::FLYSPEED;
            }
        } else if (params.control.right && !params.control.up && params.direction == 1) {
            if (params.on_ground) {
                params.force.x = PhysicsConstants::RUNSPEED;
                params.force.y = -PhysicsConstants::RUNSPEEDUP;
            } else {
                params.force.x = PhysicsConstants::FLYSPEED;
            }
        }
    }

private:
};
} // namespace Soldank

namespace Soldank
{
std::optional<AnimationState::Transition> LegsRunAnimationState::HandleInput(
  HandleInputParams& params)
{

    if (params.control.prone) {
        return AnimationState::Transition{ AnimationType::Prone, std::nullopt };
    }

    if (params.on_ground) {
        auto maybe_rolling_animation_state =
          CommonAnimationStateTransitions::TryTransitionToRolling(params);
        if (maybe_rolling_animation_state.has_value()) {
            return *maybe_rolling_animation_state;
        }
    }

    if (!params.control.left && !params.control.right) {
        if (params.on_ground) {
            if (params.control.up) {
                return AnimationState::Transition{ AnimationType::Jump, std::nullopt };
            }
            return AnimationState::Transition{ AnimationType::Stand, std::nullopt };
        }

        if (params.control.up) {
            return std::nullopt;
        }

        return AnimationState::Transition{ AnimationType::Fall, std::nullopt };
    }

    if (params.control.up && params.on_ground) {
        params.control.was_running_left = params.control.left;
        return AnimationState::Transition{ AnimationType::JumpSide, std::nullopt };
    }

    if (params.control.left && params.direction == 1) {
        return AnimationState::Transition{ AnimationType::RunBack, std::nullopt };
    }

    if (params.control.right && params.direction == -1) {
        return AnimationState::Transition{ AnimationType::RunBack, std::nullopt };
    }

    // if using jets, reset animation because first frame looks like "directional" jetting
    if (params.control.jets && params.jets_count > 0) {
        if (params.control.up) {
            return AnimationState::Transition{ AnimationType::Fall, std::nullopt };
        }
        return AnimationState::Transition{ AnimationType::Run, std::nullopt };
    }

    return std::nullopt;
}
} // namespace Soldank
