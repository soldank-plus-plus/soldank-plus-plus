module;

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include <memory>

export module Shared.Core.Animations.States:BodyGetUpAnimationState;

import Shared.Core.Animations;
import :CommonAnimationStateTransitions;
import Shared.Core.Entities.Weapon;
import Shared.Core.Types.WeaponType;

import Shared.Core.Physics.Constants;

export namespace Soldank
{
class BodyGetUpAnimationState final : public Soldank::AnimationState
{
public:
    BodyGetUpAnimationState(const AnimationDataManager& animation_data_manager)
        : AnimationState(animation_data_manager.Get(AnimationType::GetUp))
    {
    }
    ~BodyGetUpAnimationState() override = default;

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
std::optional<AnimationState::Transition> BodyGetUpAnimationState::HandleInput(
  HandleInputParams& params)
{

    if (params.control.change) {
        return AnimationState::Transition{ AnimationType::Change, std::nullopt };
    }

    if (params.control.drop &&
        params.weapons[0].GetWeaponParameters().kind != WeaponType::NoWeapon) {
        return AnimationState::Transition{ AnimationType::ThrowWeapon, std::nullopt };
    }

    // Prone cancelling
    if (params.control.throw_grenade) {
        params.grenade_can_throw = true;
        auto maybe_throw_grenade_animation_state =
          CommonAnimationStateTransitions::TryTransitionToThrowingGrenade(params);
        if (maybe_throw_grenade_animation_state.has_value()) {
            return *maybe_throw_grenade_animation_state;
        }
    }

    if (params.control.fire &&
        (params.weapons[params.active_weapon].GetWeaponParameters().kind == WeaponType::NoWeapon ||
         params.weapons[params.active_weapon].GetWeaponParameters().kind == WeaponType::Knife)) {
        return AnimationState::Transition{ AnimationType::Punch, std::nullopt };
    }

    if (GetFrame() == GetFramesCount()) {
        if (params.stance == PhysicsConstants::STANCE_STAND) {
            return AnimationState::Transition{ AnimationType::Stand, std::nullopt };
        }

        if (params.stance == PhysicsConstants::STANCE_CROUCH) {
            return AnimationState::Transition{ AnimationType::Aim, std::nullopt };
        }

        if (params.stance == PhysicsConstants::STANCE_PRONE) {
            return AnimationState::Transition{ AnimationType::Prone, std::nullopt };
        }
    }

    return std::nullopt;
}
} // namespace Soldank
