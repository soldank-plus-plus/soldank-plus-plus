module;

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include <memory>

export module Shared.Core.Animations.States:BodyStandAnimationState;

import Shared.Core.Animations;
import :CommonAnimationStateTransitions;
import Shared.Core.Entities.Weapon;
import Shared.Core.Types.WeaponType;

import Shared.Core.Physics.Constants;

export namespace Soldank
{
class BodyStandAnimationState final : public Soldank::AnimationState
{
public:
    BodyStandAnimationState(const AnimationDataManager& animation_data_manager)
        : AnimationState(animation_data_manager.Get(AnimationType::Stand))
    {
    }
    ~BodyStandAnimationState() override = default;

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
std::optional<AnimationState::Transition> BodyStandAnimationState::HandleInput(
  HandleInputParams& params)
{

    if (params.legs_animation_type == AnimationType::Roll) {
        return AnimationState::Transition{ AnimationType::Roll, std::nullopt };
    }

    if (params.legs_animation_type == AnimationType::RollBack) {
        return AnimationState::Transition{ AnimationType::RollBack, std::nullopt };
    }

    if (params.control.change) {
        return AnimationState::Transition{ AnimationType::Change, std::nullopt };
    }

    if (params.control.drop &&
        params.weapons[params.active_weapon].GetWeaponParameters().kind != WeaponType::NoWeapon) {
        return AnimationState::Transition{ AnimationType::ThrowWeapon, std::nullopt };
    }

    auto maybe_throw_grenade_animation_state =
      CommonAnimationStateTransitions::TryTransitionToThrowingGrenade(params);
    if (maybe_throw_grenade_animation_state.has_value()) {
        return *maybe_throw_grenade_animation_state;
    }

    if (params.control.fire &&
        (params.weapons[params.active_weapon].GetWeaponParameters().kind == WeaponType::NoWeapon ||
         params.weapons[params.active_weapon].GetWeaponParameters().kind == WeaponType::Knife)) {
        return AnimationState::Transition{ AnimationType::Punch, std::nullopt };
    }

    if (params.stance == PhysicsConstants::STANCE_CROUCH) {
        return AnimationState::Transition{ AnimationType::Aim, std::nullopt };
    }

    if (params.stance == PhysicsConstants::STANCE_PRONE) {
        return AnimationState::Transition{ AnimationType::Prone, std::nullopt };
    }

    return std::nullopt;
}
} // namespace Soldank
