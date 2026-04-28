module;

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

export module Shared.Core.Animations.States:LegsFallAnimationState;

import Shared.Core.Animations;
import :CommonAnimationStateTransitions;
import Shared.Core.Entities.Weapon;
import Shared.Core.Types.WeaponType;

import Shared.Core.Physics.Constants;

export namespace Soldank
{
class LegsFallAnimationState final : public Soldank::AnimationState
{
public:
    LegsFallAnimationState(const AnimationDataManager& animation_data_manager)
        : AnimationState(animation_data_manager.Get(AnimationType::Fall))
    {
    }

    ~LegsFallAnimationState() override = default;

    std::optional<AnimationState::Transition> HandleInput(HandleInputParams& params) final;

    void Update(UpdateParams& params) final { params.stance = PhysicsConstants::STANCE_STAND; }

private:
};
} // namespace Soldank

namespace Soldank
{
std::optional<AnimationState::Transition> LegsFallAnimationState::HandleInput(
  HandleInputParams& params)
{

    if (params.control.prone) {
        return AnimationState::Transition{ AnimationType::Prone, std::nullopt };
    }

    if (params.control.up && !params.on_ground) {
        return std::nullopt;
    }

    auto maybe_running_animation_state =
      CommonAnimationStateTransitions::TryTransitionToRunning(params);
    if (maybe_running_animation_state.has_value()) {
        return *maybe_running_animation_state;
    }

    if (params.on_ground) {
        if (params.control.up) {
            return AnimationState::Transition{ AnimationType::Jump, std::nullopt };
        }

        if (params.control.down) {
            return AnimationState::Transition{ AnimationType::CrouchRun, std::nullopt };
        }

        return AnimationState::Transition{ AnimationType::Stand, std::nullopt };
    }

    return std::nullopt;
}
} // namespace Soldank
