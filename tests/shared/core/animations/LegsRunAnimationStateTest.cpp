#include <gtest/gtest.h>

import Shared.Core.Animations;
import Shared.Core.Animations.States;
import Tests.Shared.Core.Animations.AnimationStateTestHelpers;

TEST(LegsRunAnimationStateTest, HandlesRunTransitions)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::LegsRunAnimationState>(
      Soldank::AnimationType::Run);
    auto params = Soldank::Test::CreateHandleInputParams();
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Stand);

    params = Soldank::Test::CreateHandleInputParams();
    params.control.up = true;
    params.control.left = true;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::JumpSide);
    EXPECT_TRUE(params.control.was_running_left);

    params = Soldank::Test::CreateHandleInputParams();
    params.control.left = true;
    params.direction = 1;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::RunBack);
}
