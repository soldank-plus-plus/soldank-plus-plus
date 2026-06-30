#include <gtest/gtest.h>

import Shared.Core.Animations;
import Shared.Core.Animations.States;
import Tests.Shared.Core.Animations.AnimationStateTestHelpers;

TEST(LegsCrouchAnimationStateTest, HandlesCrouchTransitions)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::LegsCrouchAnimationState>(
      Soldank::AnimationType::Crouch);
    auto params = Soldank::Test::CreateHandleInputParams();
    params.control.down = false;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Stand);

    params = Soldank::Test::CreateHandleInputParams();
    params.control.down = true;
    params.control.up = true;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Jump);

    params = Soldank::Test::CreateHandleInputParams();
    params.control.down = true;
    params.control.right = true;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::CrouchRun);
}
