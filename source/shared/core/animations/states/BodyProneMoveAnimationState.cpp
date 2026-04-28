module;

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

export module Shared.Core.Animations.States:BodyProneMoveAnimationState;

import Shared.Core.Animations;
import :CommonAnimationStateTransitions;
import Shared.Core.Entities.Weapon;
import Shared.Core.Types.WeaponType;

import Shared.Core.Physics.Constants;

export namespace Soldank
{
class BodyProneMoveAnimationState final : public Soldank::AnimationState
{
public:
    BodyProneMoveAnimationState(const AnimationDataManager& animation_data_manager)
        : AnimationState(animation_data_manager.Get(AnimationType::ProneMove))
    {
    }
    ~BodyProneMoveAnimationState() override = default;

    std::optional<AnimationState::Transition> HandleInput(HandleInputParams& params) final;

private:
    bool IsSoldierShootingPossible(const std::vector<Weapon>&, std::uint8_t) const final
    {
        return true;
    }
    bool IsSoldierFlagThrowingPossible() const final { return true; }
};
} // namespace Soldank

namespace Soldank
{
std::optional<AnimationState::Transition> BodyProneMoveAnimationState::HandleInput(
  HandleInputParams& params)
{

    if (params.control.prone || params.direction != params.old_direction) {
        return AnimationState::Transition{ AnimationType::GetUp, 9 };
    }

    if (params.on_ground) {
        if (params.control.down && (params.control.left || params.control.right)) {
            if (params.legs_animation_type == AnimationType::Roll) {
                return AnimationState::Transition{ AnimationType::Roll, std::nullopt };
            }

            if (params.legs_animation_type == AnimationType::RollBack) {
                return AnimationState::Transition{ AnimationType::RollBack, std::nullopt };
            }
        }
    }

    if (params.control.change) {
        return AnimationState::Transition{ AnimationType::Change, std::nullopt };
    }

    if (params.control.drop &&
        params.weapons[0].GetWeaponParameters().kind != WeaponType::NoWeapon) {
        return AnimationState::Transition{ AnimationType::ThrowWeapon, std::nullopt };
    }

    auto maybe_throw_grenade_animation_state =
      CommonAnimationStateTransitions::TryTransitionToThrowingGrenade(params);
    if (maybe_throw_grenade_animation_state.has_value()) {
        return *maybe_throw_grenade_animation_state;
    }

    if (!params.control.left && !params.control.right) {
        return AnimationState::Transition{ AnimationType::Prone, 26 };
    }

    if (GetFrame() == GetFramesCount()) {
        return AnimationState::Transition{ AnimationType::Prone, 26 };
    }

    return std::nullopt;
}
} // namespace Soldank
