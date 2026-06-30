#include <gtest/gtest.h>

import Shared.Core.Animations;
import Shared.Core.Animations.States;
import Tests.Shared.Core.Animations.AnimationStateTestHelpers;

TEST(LegsRollBackAnimationStateTest, HandlesEnterAndFinalTransitions)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::LegsRollBackAnimationState>(
      Soldank::AnimationType::RollBack);
    auto enter_params = Soldank::Test::CreateEnterParams();
    state.Enter(enter_params);
    EXPECT_NE(enter_params.force.x, 0.0F);

    state.SetFrame(state.GetFramesCount());
    auto params = Soldank::Test::CreateHandleInputParams();
    params.control.down = true;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Crouch);
}

TEST(LegsRollBackAnimationStateTest, ResolvesMovementOnlyOnLastFrame)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::LegsRollBackAnimationState>(
      Soldank::AnimationType::RollBack);

    for (unsigned int frame = 1; frame < state.GetFramesCount(); frame += 1) {
        auto params = Soldank::Test::CreateHandleInputParams();
        params.control.down = true;
        state.SetFrame(frame);

        Soldank::Test::ExpectNoTransition(state.HandleInput(params));
    }

    auto params = Soldank::Test::CreateHandleInputParams();
    params.control.down = true;
    state.SetFrame(state.GetFramesCount());

    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Crouch);
}
