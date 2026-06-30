#include <gtest/gtest.h>

import Shared.Core.Animations;
import Shared.Core.Animations.States;
import Tests.Shared.Core.Animations.AnimationStateTestHelpers;

TEST(LegsProneMoveAnimationStateTest, HandlesProneMoveTransitions)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::LegsProneMoveAnimationState>(
      Soldank::AnimationType::ProneMove);
    auto params = Soldank::Test::CreateHandleInputParams();
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Prone, 26);

    params = Soldank::Test::CreateHandleInputParams();
    params.control.left = true;
    params.control.prone = true;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::GetUp, 9);
}
