module;

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include <memory>

export module Shared.Core.Animations.States:BodyThrowWeaponAnimationState;

import Shared.Core.Animations;
import :CommonAnimationStateTransitions;
import Shared.Core.Entities.Weapon;
import Shared.Core.Types.WeaponType;

import Shared.Core.Physics.Constants;

export namespace Soldank
{
class BodyThrowWeaponAnimationState final : public Soldank::AnimationState
{
public:
    BodyThrowWeaponAnimationState(const AnimationDataManager& animation_data_manager)
        : AnimationState(animation_data_manager.Get(AnimationType::ThrowWeapon))
        , should_throw_weapon_(true)
    {
    }
    ~BodyThrowWeaponAnimationState() override = default;

    void Enter(EnterParams& params) final
    {
        if (params.weapons[params.active_weapon].GetWeaponParameters().kind == WeaponType::Knife) {
            SetSpeed(2);
        }
    }

    std::optional<AnimationState::Transition> HandleInput(HandleInputParams& params) final;

    void Exit(ExitParams& params) final
    {
        params.should_throw_active_weapon = should_throw_weapon_;
    }

private:
    bool IsSoldierShootingPossible(const std::vector<Weapon>&, std::uint8_t) const final
    {
        return true;
    }
    bool IsSoldierFlagThrowingPossible() const final { return true; }

    bool should_throw_weapon_;
};
} // namespace Soldank

namespace Soldank
{
std::optional<AnimationState::Transition> BodyThrowWeaponAnimationState::HandleInput(
  HandleInputParams& params)
{

    should_throw_weapon_ = true;

    if (params.legs_animation_type == AnimationType::Roll) {
        should_throw_weapon_ = false;
        return AnimationState::Transition{ AnimationType::Roll, std::nullopt };
    }

    if (params.legs_animation_type == AnimationType::RollBack) {
        should_throw_weapon_ = false;
        return AnimationState::Transition{ AnimationType::RollBack, std::nullopt };
    }

    if (params.control.throw_grenade) {
        should_throw_weapon_ = false;
        return AnimationState::Transition{ AnimationType::Throw, std::nullopt };
    }

    if (params.control.change) {
        should_throw_weapon_ = false;
        return AnimationState::Transition{ AnimationType::Change, std::nullopt };
    }

    if (params.weapons[params.active_weapon].GetWeaponParameters().kind == WeaponType::Knife) {
        if (params.control.fire && params.control.drop) {
            should_throw_weapon_ = false;
            return AnimationState::Transition{ AnimationType::ThrowWeapon, std::nullopt };
        }

        if (params.control.fire) {
            should_throw_weapon_ = false;
            return AnimationState::Transition{ AnimationType::Punch, std::nullopt };
        }
    }

    if ((params.weapons[params.active_weapon].GetWeaponParameters().kind == WeaponType::Knife &&
         (!params.control.drop || GetFrame() == 16)) ||
        GetFrame() == GetFramesCount()) {

        if (params.stance == PhysicsConstants::STANCE_CROUCH) {
            return AnimationState::Transition{ AnimationType::Aim, std::nullopt };
        }

        if (params.stance == PhysicsConstants::STANCE_PRONE) {
            return AnimationState::Transition{ AnimationType::Prone, 26 };
        }

        if (params.stance == PhysicsConstants::STANCE_STAND) {
            return AnimationState::Transition{ AnimationType::Stand, std::nullopt };
        }
    }

    return std::nullopt;
}
} // namespace Soldank
