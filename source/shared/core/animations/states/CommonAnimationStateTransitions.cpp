module;

#include <optional>
#include <memory>

export module Shared.Core.Animations.States:CommonAnimationStateTransitions;

import Shared.Core.Animations;

export namespace Soldank::CommonAnimationStateTransitions
{
std::optional<AnimationState::Transition> TryTransitionToRunning(
  const AnimationState::HandleInputParams& params);

std::optional<AnimationState::Transition> TryTransitionToCrouchRunning(
  const AnimationState::HandleInputParams& params);

std::optional<AnimationState::Transition> TryTransitionToRolling(
  const AnimationState::HandleInputParams& params);

std::optional<AnimationState::Transition> TryTransitionToThrowingGrenade(
  AnimationState::HandleInputParams& params);
} // namespace Soldank::CommonAnimationStateTransitions

namespace Soldank::CommonAnimationStateTransitions
{
std::optional<AnimationState::Transition> TryTransitionToRunning(
  const AnimationState::HandleInputParams& params)
{
    if (params.control.left) {
        if (params.direction == 1) {
            return AnimationState::Transition{ AnimationType::RunBack, std::nullopt };
        }

        return AnimationState::Transition{ AnimationType::Run, std::nullopt };
    }

    if (params.control.right) {
        if (params.direction == -1) {
            return AnimationState::Transition{ AnimationType::RunBack, std::nullopt };
        }

        return AnimationState::Transition{ AnimationType::Run, std::nullopt };
    }

    return std::nullopt;
}

std::optional<AnimationState::Transition> TryTransitionToCrouchRunning(
  const AnimationState::HandleInputParams& params)
{
    if (params.control.right) {
        if (params.direction == 1) {
            return AnimationState::Transition{ AnimationType::CrouchRun, std::nullopt };
        }

        return AnimationState::Transition{ AnimationType::CrouchRunBack, std::nullopt };
    }

    if (params.control.left) {
        if (params.direction == -1) {
            return AnimationState::Transition{ AnimationType::CrouchRun, std::nullopt };
        }

        return AnimationState::Transition{ AnimationType::CrouchRunBack, std::nullopt };
    }

    return std::nullopt;
}

std::optional<AnimationState::Transition> TryTransitionToRolling(
  const AnimationState::HandleInputParams& params)
{
    if (params.control.down && params.control.left && params.direction == -1) {
        return AnimationState::Transition{ AnimationType::Roll, std::nullopt };
    }

    if (params.control.down && params.control.right && params.direction == 1) {
        return AnimationState::Transition{ AnimationType::Roll, std::nullopt };
    }

    if (params.control.down && params.control.left && params.direction == 1) {
        return AnimationState::Transition{ AnimationType::RollBack, std::nullopt };
    }

    if (params.control.down && params.control.right && params.direction == -1) {
        return AnimationState::Transition{ AnimationType::RollBack, std::nullopt };
    }

    return std::nullopt;
}

std::optional<AnimationState::Transition> TryTransitionToThrowingGrenade(
  AnimationState::HandleInputParams& params)
{
    if (!params.control.throw_grenade) {
        params.grenade_can_throw = true;
    }

    if (params.grenade_can_throw && params.control.throw_grenade) {
        return AnimationState::Transition{ AnimationType::Throw, std::nullopt };
    }

    return std::nullopt;
}
} // namespace Soldank::CommonAnimationStateTransitions
