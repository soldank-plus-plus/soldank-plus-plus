module;

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

export module Shared.Core.Animations.States:BodyWeaponActionAnimationState;

import Shared.Core.Animations;
import Shared.Core.Entities.Weapon;
import Shared.Core.Types.WeaponType;
import Shared.Core.Physics.Constants;
import :CommonAnimationStateTransitions;

export namespace Soldank
{
class BodyWeaponActionAnimationState final : public AnimationState
{
public:
    BodyWeaponActionAnimationState(const AnimationDataManager& manager, AnimationType type)
        : AnimationState(manager.Get(type))
    {
    }
    std::optional<Transition> HandleInput(HandleInputParams& params) final
    {
        if (params.legs_animation_type == AnimationType::Roll ||
            params.legs_animation_type == AnimationType::RollBack) {
            return Transition{ params.legs_animation_type, std::nullopt };
        }
        if (params.legs_animation_type == AnimationType::GetUp) {
            return Transition{ AnimationType::GetUp, 9 };
        }
        if (params.control.change) {
            return Transition{ AnimationType::Change, std::nullopt };
        }
        if (const auto grenade =
              CommonAnimationStateTransitions::TryTransitionToThrowingGrenade(params)) {
            return grenade;
        }
        if (params.control.drop && !params.weapons.at(params.active_weapon)
                                      .IsAny({ WeaponType::NoWeapon,
                                               WeaponType::Bow,
                                               WeaponType::FlameBow,
                                               WeaponType::Flamer })) {
            return Transition{ AnimationType::ThrowWeapon, std::nullopt };
        }
        if (GetFrame() < GetFramesCount()) {
            return std::nullopt;
        }
        if (params.stance == PhysicsConstants::STANCE_PRONE) {
            return Transition{ AnimationType::Prone, 26 };
        }
        return Transition{ params.stance == PhysicsConstants::STANCE_CROUCH ? AnimationType::Aim
                                                                            : AnimationType::Stand,
                           std::nullopt };
    }

private:
    bool IsSoldierShootingPossible(const std::vector<Weapon>&, std::uint8_t) const final
    {
        return GetType() != AnimationType::Melee || (GetFrame() == 12 && GetCount() == 0);
    }
};
} // namespace Soldank
