#include <gtest/gtest.h>

import Shared.Core.Animations;
import Shared.Core.Animations.States;
import Tests.Shared.Core.Animations.AnimationStateTestHelpers;

TEST(LegsStandAnimationStateTest, HandlesCoreTransitions)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::LegsStandAnimationState>(
      Soldank::AnimationType::Stand);
    auto params = Soldank::Test::CreateHandleInputParams();
    params.control.prone = true;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Prone);

    params = Soldank::Test::CreateHandleInputParams();
    params.control.up = true;
    params.control.right = true;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::JumpSide);

    params = Soldank::Test::CreateHandleInputParams();
    params.control.left = true;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::RunBack);
}
