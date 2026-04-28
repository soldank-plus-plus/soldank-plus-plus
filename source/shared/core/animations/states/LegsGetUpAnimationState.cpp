module;

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

export module Shared.Core.Animations.States:LegsGetUpAnimationState;

import Shared.Core.Animations;
import :CommonAnimationStateTransitions;
import Shared.Core.Entities.Weapon;
import Shared.Core.Types.WeaponType;

import Shared.Core.Physics.Constants;

export namespace Soldank
{
class LegsGetUpAnimationState final : public Soldank::AnimationState
{
public:
    LegsGetUpAnimationState(const AnimationDataManager& animation_data_manager)
        : AnimationState(animation_data_manager.Get(AnimationType::GetUp))
    {
    }

    ~LegsGetUpAnimationState() override = default;

    std::optional<AnimationState::Transition> HandleInput(HandleInputParams& params) final;

    void Update(UpdateParams& params) final { params.stance = PhysicsConstants::STANCE_STAND; }

private:
    /*
    This method is used for the so called "prone cancelling".
    It's a trick in Soldank++ to skip GetUp animation.
    The main benefit is not to break momentum when running around
    */
    static bool ShouldCancelAnimation(const HandleInputParams& params)
    {
        if (params.control.throw_grenade) {
            return true;
        }

        if (params.control.fire &&
            (params.weapons[params.active_weapon].GetWeaponParameters().kind ==
               WeaponType::NoWeapon ||
             params.weapons[params.active_weapon].GetWeaponParameters().kind ==
               WeaponType::Knife)) {
            return true;
        }

        return false;
    }
};
} // namespace Soldank

namespace Soldank
{
std::optional<AnimationState::Transition> LegsGetUpAnimationState::HandleInput(
  HandleInputParams& params)
{

    if (ShouldCancelAnimation(params) || GetFrame() == GetFramesCount()) {
        if (params.on_ground) {
            return AnimationState::Transition{ AnimationType::Stand, std::nullopt };
        }

        return AnimationState::Transition{ AnimationType::Fall, std::nullopt };
    }

    // Immediately switch from unprone to jump/sidejump, because the end of the
    // unprone animation can be seen as the "wind up" for the jump
    if (ShouldCancelAnimation(params) || (GetFrame() > 20 && params.on_ground)) {
        if (params.control.up) {
            if (params.control.left || params.control.right) {
                // Set sidejump frame 1 to 4 depending on which unprone frame we're in
                auto id = GetFrame() - 20;
                return AnimationState::Transition{ AnimationType::JumpSide, id };
            }

            // Set jump frame 6 to 9 depending on which unprone frame we're in
            auto id = GetFrame() - (23 - (9 - 1));
            return AnimationState::Transition{ AnimationType::Jump, id };
        }
    } else if (ShouldCancelAnimation(params) || GetFrame() > 23) {
        auto maybe_running_animation_state =
          CommonAnimationStateTransitions::TryTransitionToRunning(params);
        if (maybe_running_animation_state.has_value()) {
            return *maybe_running_animation_state;
        }

        if (!params.on_ground && params.control.up) {
            return AnimationState::Transition{ AnimationType::Run, std::nullopt };
        }
    }
    return std::nullopt;
}
} // namespace Soldank
