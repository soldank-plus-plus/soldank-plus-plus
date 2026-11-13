module;

#include <memory>
#include <optional>
#include <vector>
#include <cstdint>

export module Shared.Core.Animations.States:BodyReloadAnimationState;

import Shared.Core.Animations;
import Shared.Core.Entities.Weapon;
import Shared.Core.Types.WeaponType;
import Shared.Core.Physics.Constants;

export namespace Soldank
{
class BodyReloadAnimationState final : public AnimationState
{
public:
    BodyReloadAnimationState(const AnimationDataManager& manager, AnimationType type)
        : AnimationState(manager.Get(type))
    {
    }

    std::optional<Transition> HandleInput(HandleInputParams& params) final
    {
        if (params.legs_animation_type == AnimationType::Roll ||
            params.legs_animation_type == AnimationType::RollBack) {
            return Transition{ params.legs_animation_type, std::nullopt };
        }
        if (params.control.change) {
            return Transition{ AnimationType::Change, std::nullopt };
        }
        if (params.control.drop && !params.weapons.at(params.active_weapon)
                                      .IsAny({ WeaponType::NoWeapon,
                                               WeaponType::Bow,
                                               WeaponType::FlameBow,
                                               WeaponType::Flamer })) {
            return Transition{ AnimationType::ThrowWeapon, std::nullopt };
        }
        if (params.grenade_can_throw && params.control.throw_grenade) {
            return Transition{ AnimationType::Throw, std::nullopt };
        }
        if (params.weapons.at(params.active_weapon).IsReloading()) {
            if (GetType() == AnimationType::Reload && GetFrame() == GetFramesCount()) {
                SetFrame(1);
                SetCount(0);
            }
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
    bool IsSoldierShootingPossible(const std::vector<Weapon>& weapons,
                                   std::uint8_t active_weapon) const final
    {
        return weapons[active_weapon].GetWeaponParameters().kind == WeaponType::Spas12;
    }
};
} // namespace Soldank
