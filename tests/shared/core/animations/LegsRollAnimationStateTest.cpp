#include <gtest/gtest.h>

import Shared.Core.Animations;
import Shared.Core.Animations.States;
import Tests.Shared.Core.Animations.AnimationStateTestHelpers;

TEST(LegsRollAnimationStateTest, HandlesEnterAndFinalTransitions)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::LegsRollAnimationState>(
      Soldank::AnimationType::Roll);
    auto enter_params = Soldank::Test::CreateEnterParams();
    state.Enter(enter_params);
    EXPECT_NE(enter_params.force.x, 0.0F);

    auto params = Soldank::Test::CreateHandleInputParams();
    params.control.prone = true;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Prone);

    state.SetFrame(state.GetFramesCount());
    params = Soldank::Test::CreateHandleInputParams();
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Stand);
}

TEST(LegsRollAnimationStateTest, ResolvesMovementOnlyOnLastFrame)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::LegsRollAnimationState>(
      Soldank::AnimationType::Roll);

    for (unsigned int frame = 1; frame < state.GetFramesCount(); frame += 1) {
        auto params = Soldank::Test::CreateHandleInputParams();
        state.SetFrame(frame);

        Soldank::Test::ExpectNoTransition(state.HandleInput(params));
    }

    auto params = Soldank::Test::CreateHandleInputParams();
    state.SetFrame(state.GetFramesCount());

    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Stand);
}
