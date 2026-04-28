module;

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

export module Shared.Core.Animations.States:BodyPunchAnimationState;

import Shared.Core.Animations;
import :CommonAnimationStateTransitions;
import Shared.Core.Entities.Weapon;
import Shared.Core.Types.WeaponType;

import Shared.Core.Physics.Constants;

export namespace Soldank
{
class BodyPunchAnimationState final : public Soldank::AnimationState
{
public:
    BodyPunchAnimationState(const AnimationDataManager& animation_data_manager)
        : AnimationState(animation_data_manager.Get(AnimationType::Punch))
    {
    }
    ~BodyPunchAnimationState() override = default;

    std::optional<AnimationState::Transition> HandleInput(HandleInputParams& params) final;

    void Update(UpdateParams& params) final {}

private:
    bool IsSoldierFlagThrowingPossible() const final { return true; }
};
} // namespace Soldank

namespace Soldank
{
std::optional<AnimationState::Transition> BodyPunchAnimationState::HandleInput(
  HandleInputParams& params)
{

    if (params.legs_animation_type == AnimationType::Roll) {
        return AnimationState::Transition{ AnimationType::Roll, std::nullopt };
    }

    if (params.legs_animation_type == AnimationType::RollBack) {
        return AnimationState::Transition{ AnimationType::RollBack, std::nullopt };
    }

    if (params.legs_animation_type == AnimationType::ProneMove) {
        return AnimationState::Transition{ AnimationType::ProneMove, std::nullopt };
    }

    // Prone cancelling
    if (params.legs_animation_type == AnimationType::GetUp) {
        return AnimationState::Transition{ AnimationType::GetUp, 9 };
    }

    if (params.control.change) {
        return AnimationState::Transition{ AnimationType::Change, std::nullopt };
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
