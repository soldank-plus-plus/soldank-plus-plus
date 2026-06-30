#include <gtest/gtest.h>

import Shared.Core.Animations;
import Shared.Core.Animations.States;
import Tests.Shared.Core.Animations.AnimationStateTestHelpers;

TEST(LegsCrouchRunBackAnimationStateTest, HandlesCrouchRunBackTransitions)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::LegsCrouchRunBackAnimationState>(
      Soldank::AnimationType::CrouchRunBack);
    auto params = Soldank::Test::CreateHandleInputParams();
    params.control.down = true;
    params.control.right = true;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::CrouchRun);
}
