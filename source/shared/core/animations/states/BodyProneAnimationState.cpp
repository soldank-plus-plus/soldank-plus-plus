module;

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

export module Shared.Core.Animations.States:BodyProneAnimationState;

import Shared.Core.Animations;
import :CommonAnimationStateTransitions;
import Shared.Core.Entities.Weapon;
import Shared.Core.Types.WeaponType;

import Shared.Core.Physics.Constants;

export namespace Soldank
{
class BodyProneAnimationState final : public Soldank::AnimationState
{
public:
    BodyProneAnimationState(const AnimationDataManager& animation_data_manager)
        : AnimationState(animation_data_manager.Get(AnimationType::Prone))
    {
    }
    ~BodyProneAnimationState() override = default;

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
std::optional<AnimationState::Transition> BodyProneAnimationState::HandleInput(
  HandleInputParams& params)
{

    if (GetFrame() >= 23 && params.on_ground) {
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

    if (params.stance == PhysicsConstants::STANCE_STAND) {
        return AnimationState::Transition{ AnimationType::GetUp, 9 };
    }

    if (params.legs_animation_type == AnimationType::ProneMove && params.on_ground) {
        if (params.control.left || params.control.right) {
            return AnimationState::Transition{ AnimationType::ProneMove, std::nullopt };
        }
    }

    if (params.control.fire &&
        (params.weapons[params.active_weapon].GetWeaponParameters().kind == WeaponType::NoWeapon ||
         params.weapons[params.active_weapon].GetWeaponParameters().kind == WeaponType::Knife)) {
        return AnimationState::Transition{ AnimationType::Punch, std::nullopt };
    }

    return std::nullopt;
}
} // namespace Soldank
