module;

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

export module Shared.Core.Animations.States:BodyChangeAnimationState;

import Shared.Core.Animations;
import :CommonAnimationStateTransitions;
import Shared.Core.Entities.Weapon;
import Shared.Core.Types.WeaponType;

import Shared.Core.Physics.Constants;

export namespace Soldank
{
class BodyChangeAnimationState final : public Soldank::AnimationState
{
public:
    BodyChangeAnimationState(const AnimationDataManager& animation_data_manager)
        : AnimationState(animation_data_manager.Get(AnimationType::Change))
    {
    }
    ~BodyChangeAnimationState() final = default;

    std::optional<AnimationState::Transition> HandleInput(HandleInputParams& params) final;

    void Update(UpdateParams& params) final
    {
        if (GetFrame() == 25) {
            params.should_switch_weapon = true;
        }
    }

private:
    bool IsSoldierShootingPossible(const std::vector<Weapon>& weapons,
                                   std::uint8_t active_weapon) const final
    {
        return weapons[active_weapon].GetWeaponParameters().kind == WeaponType::Chainsaw;
    }

    bool IsSoldierFlagThrowingPossible() const final { return true; }
};
} // namespace Soldank

namespace Soldank
{
std::optional<AnimationState::Transition> BodyChangeAnimationState::HandleInput(
  HandleInputParams& params)
{

    if (GetFrame() == 2) {
        // TODO: play sound

        // Apparently soldat skips this frame
        SetNextFrame();
    }

    if (params.legs_animation_type == AnimationType::Roll) {
        return AnimationState::Transition{ AnimationType::Roll, std::nullopt };
    }

    if (params.legs_animation_type == AnimationType::RollBack) {
        return AnimationState::Transition{ AnimationType::RollBack, std::nullopt };
    }

    if (GetFrame() == GetFramesCount()) {
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
