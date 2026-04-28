module;

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

export module Shared.Core.Animations.States:BodyAimAnimationState;

import Shared.Core.Animations;
import :CommonAnimationStateTransitions;
import Shared.Core.Entities.Weapon;
import Shared.Core.Types.WeaponType;

import Shared.Core.State.Control;
import Shared.Core.Physics.Constants;

export namespace Soldank
{
class BodyAimAnimationState final : public Soldank::AnimationState
{
public:
    BodyAimAnimationState(const AnimationDataManager& animation_data_manager)
        : AnimationState(animation_data_manager.Get(AnimationType::Aim))
    {
    }

    ~BodyAimAnimationState() override = default;

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
std::optional<AnimationState::Transition> BodyAimAnimationState::HandleInput(
  HandleInputParams& params)
{

    if (params.control.change) {
        return AnimationState::Transition{ .animation_type = AnimationType::Change,
                                           .initial_frame = std::nullopt };
    }

    if (params.control.drop &&
        params.weapons[0].GetWeaponParameters().kind != WeaponType::NoWeapon) {
        return AnimationState::Transition{ .animation_type = AnimationType::ThrowWeapon,
                                           .initial_frame = std::nullopt };
    }

    auto maybe_throw_grenade_animation_state =
      CommonAnimationStateTransitions::TryTransitionToThrowingGrenade(params);
    if (maybe_throw_grenade_animation_state.has_value()) {
        return *maybe_throw_grenade_animation_state;
    }

    if (params.control.fire &&
        (params.weapons[params.active_weapon].GetWeaponParameters().kind == WeaponType::NoWeapon ||
         params.weapons[params.active_weapon].GetWeaponParameters().kind == WeaponType::Knife)) {
        return AnimationState::Transition{ .animation_type = AnimationType::Punch,
                                           .initial_frame = std::nullopt };
    }

    if (params.stance == PhysicsConstants::STANCE_STAND) {
        return AnimationState::Transition{ .animation_type = AnimationType::Stand,
                                           .initial_frame = std::nullopt };
    }

    if (params.stance == PhysicsConstants::STANCE_PRONE) {
        return AnimationState::Transition{ .animation_type = AnimationType::Prone,
                                           .initial_frame = std::nullopt };
    }

    return std::nullopt;
}
} // namespace Soldank
