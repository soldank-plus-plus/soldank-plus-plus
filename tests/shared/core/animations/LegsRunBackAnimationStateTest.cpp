#include <gtest/gtest.h>

import Shared.Core.Animations;
import Shared.Core.Animations.States;
import Tests.Shared.Core.Animations.AnimationStateTestHelpers;

TEST(LegsRunBackAnimationStateTest, HandlesRunBackTransitions)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::LegsRunBackAnimationState>(
      Soldank::AnimationType::RunBack);
    auto params = Soldank::Test::CreateHandleInputParams();
    params.on_ground = false;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Fall);

    params = Soldank::Test::CreateHandleInputParams();
    params.control.right = true;
    params.direction = 1;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Run);
}
