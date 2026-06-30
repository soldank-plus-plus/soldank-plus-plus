#include <gtest/gtest.h>

import Shared.Core.Animations;
import Shared.Core.Animations.States;
import Shared.Core.Physics.Constants;
import Tests.Shared.Core.Animations.AnimationStateTestHelpers;

TEST(BodyRollBackAnimationStateTest, HandlesProneAndFinalStanceTransitions)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::BodyRollBackAnimationState>(
      Soldank::AnimationType::RollBack);
    auto params = Soldank::Test::CreateHandleInputParams();
    params.control.prone = true;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Prone);

    state.SetFrame(state.GetFramesCount());
    params = Soldank::Test::CreateHandleInputParams();
    params.stance = Soldank::PhysicsConstants::STANCE_STAND;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Stand);
}

TEST(BodyRollBackAnimationStateTest, TransitionsToStanceAnimationOnlyOnLastFrame)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::BodyRollBackAnimationState>(
      Soldank::AnimationType::RollBack);

    for (unsigned int frame = 1; frame < state.GetFramesCount(); frame += 1) {
        auto params = Soldank::Test::CreateHandleInputParams();
        state.SetFrame(frame);
        Soldank::Test::ExpectNoTransition(state.HandleInput(params));
    }

    auto params = Soldank::Test::CreateHandleInputParams();
    state.SetFrame(state.GetFramesCount());
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Aim);
}
