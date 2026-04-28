module;

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

export module Shared.Core.Animations.States:LegsCrouchAnimationState;

import Shared.Core.Animations;
import :CommonAnimationStateTransitions;
import Shared.Core.Entities.Weapon;
import Shared.Core.Types.WeaponType;

import Shared.Core.Physics.Constants;

export namespace Soldank
{
class LegsCrouchAnimationState final : public Soldank::AnimationState
{
public:
    LegsCrouchAnimationState(const AnimationDataManager& animation_data_manager)
        : AnimationState(animation_data_manager.Get(AnimationType::Crouch))
    {
    }

    ~LegsCrouchAnimationState() override = default;

    std::optional<AnimationState::Transition> HandleInput(HandleInputParams& params) final;

    void Update(UpdateParams& params) final { params.stance = PhysicsConstants::STANCE_CROUCH; }

private:
};
} // namespace Soldank

namespace Soldank
{
std::optional<AnimationState::Transition> LegsCrouchAnimationState::HandleInput(
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

    if (params.control.up && params.on_ground) {
        return AnimationState::Transition{ AnimationType::Jump, std::nullopt };
    }

    auto maybe_crouch_running_animation_state =
      CommonAnimationStateTransitions::TryTransitionToCrouchRunning(params);
    if (maybe_crouch_running_animation_state.has_value()) {
        return *maybe_crouch_running_animation_state;
    }

    return std::nullopt;
}
} // namespace Soldank
