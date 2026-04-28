module;

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

export module Shared.Core.Animations.States:LegsRollBackAnimationState;

import Shared.Core.Animations;
import :CommonAnimationStateTransitions;
import Shared.Core.Entities.Weapon;
import Shared.Core.Types.WeaponType;

import Shared.Core.Physics.Constants;

export namespace Soldank
{
class LegsRollBackAnimationState final : public Soldank::AnimationState
{
public:
    LegsRollBackAnimationState(const AnimationDataManager& animation_data_manager)
        : AnimationState(animation_data_manager.Get(AnimationType::RollBack))
    {
    }

    ~LegsRollBackAnimationState() override = default;

    void Enter(EnterParams& params) final
    {
        if (params.on_ground) {
            params.force.x = -(float)params.direction * 2.0F * PhysicsConstants::CROUCHRUNSPEED;
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
            params.force.x = -(float)params.direction * PhysicsConstants::ROLLSPEED;
        } else {
            params.force.x = -(float)params.direction * 2.0F * PhysicsConstants::FLYSPEED;
        }

        if ((GetFrame() > 1) && (GetFrame() < 8)) {
            // so apparently in soldat there's no difference between a RollBack and a backflip
            // if you press W during a Rollback in those frames then it pushes you up and therefore
            // it turns into a backflip. That's how also cannonballs works, if we release W fast
            // enough, it will turn into a RollBack
            if (params.control.up) {
                params.force.y -= PhysicsConstants::JUMPDIRSPEED * 1.5F;
                params.force.x *= 0.5;
                params.velocity.x *= 0.8;
            }
        }
    }

private:
};
} // namespace Soldank

namespace Soldank
{
std::optional<AnimationState::Transition> LegsRollBackAnimationState::HandleInput(
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

        if (params.control.down) {
            return AnimationState::Transition{ AnimationType::Crouch, std::nullopt };
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
