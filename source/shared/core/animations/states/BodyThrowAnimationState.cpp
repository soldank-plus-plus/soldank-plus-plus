module;

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include <memory>

export module Shared.Core.Animations.States:BodyThrowAnimationState;

import Shared.Core.Animations;
import :CommonAnimationStateTransitions;
import Shared.Core.Entities.Weapon;
import Shared.Core.Types.WeaponType;

import Shared.Core.Physics.Constants;

export namespace Soldank
{
class BodyThrowAnimationState final : public Soldank::AnimationState
{
public:
    BodyThrowAnimationState(const AnimationDataManager& animation_data_manager)
        : AnimationState(animation_data_manager.Get(AnimationType::Throw))
    {
    }
    ~BodyThrowAnimationState() override = default;

    void Enter(EnterParams& params) final { params.grenade_can_throw = false; }

    std::optional<AnimationState::Transition> HandleInput(HandleInputParams& params) final;

    void Update(UpdateParams& params) final {}

private:
    bool IsSoldierShootingPossible(const std::vector<Weapon>&, std::uint8_t) const final
    {
        return true;
    }
    bool IsSoldierFlagThrowingPossible() const final { return true; }

}; // namespace Soldank
} // namespace Soldank

namespace Soldank
{
std::optional<AnimationState::Transition> BodyThrowAnimationState::HandleInput(
  HandleInputParams& params)
{

    if (params.legs_animation_type == AnimationType::Roll) {
        return AnimationState::Transition{ AnimationType::Roll, std::nullopt };
    }

    if (params.legs_animation_type == AnimationType::RollBack) {
        return AnimationState::Transition{ AnimationType::RollBack, std::nullopt };
    }

    // Prone cancelling
    if (params.legs_animation_type == AnimationType::GetUp) {
        return AnimationState::Transition{ AnimationType::GetUp, 9 };
    }

    if (params.control.change) {
        return AnimationState::Transition{ AnimationType::Change, std::nullopt };
    }

    if (GetFrame() == GetFramesCount() || !params.control.throw_grenade) {
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
