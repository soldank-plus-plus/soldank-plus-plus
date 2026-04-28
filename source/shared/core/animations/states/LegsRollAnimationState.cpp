module;

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

export module Shared.Core.Animations.States:LegsRollAnimationState;

import Shared.Core.Animations;
import :CommonAnimationStateTransitions;
import Shared.Core.Entities.Weapon;
import Shared.Core.Types.WeaponType;

import Shared.Core.Physics.Constants;

export namespace Soldank
{
class LegsRollAnimationState final : public Soldank::AnimationState
{
public:
    LegsRollAnimationState(const AnimationDataManager& animation_data_manager)
        : AnimationState(animation_data_manager.Get(AnimationType::Roll))
    {
    }

    ~LegsRollAnimationState() override = default;

    void Enter(EnterParams& params) final
    {
        if (params.on_ground) {
            params.force.x = (float)params.direction * 2.0F * PhysicsConstants::CROUCHRUNSPEED;
        }
    }

    std::optional<AnimationState::Transition> HandleInput(HandleInputParams& params) final;

    void Update(UpdateParams& params) final
    {
        params.stance = PhysicsConstants::STANCE_STAND;

        if (GetSpeed() > 1) {
            params.velocity.x /= (float)GetSpeed();
            params.velocity.y /= (float)GetSpeed();
        }

        if (params.on_ground) {
            params.force.x = (float)params.direction * PhysicsConstants::ROLLSPEED;
        } else {
            params.force.x = (float)params.direction * 2.0F * PhysicsConstants::FLYSPEED;
        }
    }

private:
};
} // namespace Soldank

namespace Soldank
{
std::optional<AnimationState::Transition> LegsRollAnimationState::HandleInput(
  HandleInputParams& params)
{

    if (params.control.prone) {
        return AnimationState::Transition{ AnimationType::Prone, std::nullopt };
    }

    if (GetFrame() == GetFramesCount()) {
        auto maybe_crouch_run_animation_state =
          CommonAnimationStateTransitions::TryTransitionToCrouchRunning(params);
        if (maybe_crouch_run_animation_state.has_value()) {
            return *maybe_crouch_run_animation_state;
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
                return AnimationState::Transition{ AnimationType::Crouch, std::nullopt };
            }

            return AnimationState::Transition{ AnimationType::Stand, std::nullopt };
        }

        return AnimationState::Transition{ AnimationType::Fall, std::nullopt };
    }

    return std::nullopt;
}
} // namespace Soldank
