module;
#include <optional>

export module Shared.Core.Animations.States:EmoteAnimationState;
import Shared.Core.Animations;
import Shared.Core.Physics.Constants;

export namespace Soldank
{
// The shared emote controller owns transitions and frame-specific gameplay effects.
class EmoteAnimationState final : public AnimationState
{
public:
    EmoteAnimationState(const AnimationDataManager& manager, AnimationType type)
        : AnimationState(manager.Get(type))
    {
    }
    std::optional<Transition> HandleInput(HandleInputParams&) final { return std::nullopt; }
    void Update(UpdateParams& params) final { params.stance = PhysicsConstants::STANCE_STAND; }
};
} // namespace Soldank
