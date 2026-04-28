module;

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

export module Shared.Core.Animations.States:BodyRollBackAnimationState;

import Shared.Core.Animations;
import :CommonAnimationStateTransitions;
import Shared.Core.Entities.Weapon;
import Shared.Core.Types.WeaponType;

import Shared.Core.Physics.Constants;

export namespace Soldank
{
class BodyRollBackAnimationState final : public Soldank::AnimationState
{
public:
    BodyRollBackAnimationState(const AnimationDataManager& animation_data_manager)
        : AnimationState(animation_data_manager.Get(AnimationType::RollBack))
    {
    }
    ~BodyRollBackAnimationState() override = default;

    std::optional<AnimationState::Transition> HandleInput(HandleInputParams& params) final;

private:
    bool IsSoldierShootingPossible(const std::vector<Weapon>& weapons,
                                   std::uint8_t active_weapon) const final
    {
        return weapons[active_weapon].GetWeaponParameters().kind == WeaponType::Chainsaw;
    }
};
} // namespace Soldank

namespace Soldank
{
std::optional<AnimationState::Transition> BodyRollBackAnimationState::HandleInput(
  HandleInputParams& params)
{

    if (params.control.prone) {
        return AnimationState::Transition{ AnimationType::Prone, std::nullopt };
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
