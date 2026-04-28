module;

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

export module Shared.Core.Animations.States:LegsStandAnimationState;

import Shared.Core.Animations;
import :CommonAnimationStateTransitions;
import Shared.Core.Entities.Weapon;
import Shared.Core.Types.WeaponType;

import Shared.Core.Physics.Constants;

export namespace Soldank
{
class LegsStandAnimationState final : public Soldank::AnimationState
{
public:
    LegsStandAnimationState(const AnimationDataManager& animation_data_manager)
        : AnimationState(animation_data_manager.Get(AnimationType::Stand))
    {
    }

    ~LegsStandAnimationState() override = default;

    std::optional<AnimationState::Transition> HandleInput(HandleInputParams& params) final;

    void Update(UpdateParams& params) final { params.stance = PhysicsConstants::STANCE_STAND; }

private:
};
} // namespace Soldank

namespace Soldank
{
std::optional<AnimationState::Transition> LegsStandAnimationState::HandleInput(
  HandleInputParams& params)
{

    if (params.control.prone) {
        return AnimationState::Transition{ AnimationType::Prone, std::nullopt };
    }

    if (params.control.up && params.on_ground) {
        if (params.control.left || params.control.right) {
            return AnimationState::Transition{ AnimationType::JumpSide, std::nullopt };
        }
        return AnimationState::Transition{ AnimationType::Jump, std::nullopt };
    }

    auto maybe_running_animation_state =
      CommonAnimationStateTransitions::TryTransitionToRunning(params);
    if (maybe_running_animation_state.has_value()) {
        return *maybe_running_animation_state;
    }

    if (!params.on_ground) {
        return AnimationState::Transition{ AnimationType::Fall, std::nullopt };
    }

    if (params.control.down) {
        return AnimationState::Transition{ AnimationType::Crouch, std::nullopt };
    }

    return std::nullopt;
}
} // namespace Soldank
