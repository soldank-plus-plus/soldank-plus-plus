#include <gtest/gtest.h>

import Shared.Core.Animations;
import Shared.Core.Animations.States;
import Tests.Shared.Core.Animations.AnimationStateTestHelpers;

TEST(LegsCrouchRunAnimationStateTest, HandlesCrouchRunTransitions)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::LegsCrouchRunAnimationState>(
      Soldank::AnimationType::CrouchRun);
    auto params = Soldank::Test::CreateHandleInputParams();
    params.control.down = true;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Crouch);

    params = Soldank::Test::CreateHandleInputParams();
    params.control.down = true;
    params.control.left = true;
    Soldank::Test::ExpectTransition(
      state.HandleInput(params), Soldank::AnimationType::CrouchRunBack);
}
