#include <gtest/gtest.h>

import Shared.Core.Animations;
import Shared.Core.Animations.States;
import Tests.Shared.Core.Animations.AnimationStateTestHelpers;

TEST(LegsFallAnimationStateTest, HandlesFallTransitions)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::LegsFallAnimationState>(
      Soldank::AnimationType::Fall);
    auto params = Soldank::Test::CreateHandleInputParams();
    params.control.prone = true;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Prone);

    params = Soldank::Test::CreateHandleInputParams();
    params.on_ground = false;
    params.control.up = true;
    Soldank::Test::ExpectNoTransition(state.HandleInput(params));

    params = Soldank::Test::CreateHandleInputParams();
    params.control.down = true;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::CrouchRun);
}
