#include <gtest/gtest.h>

import Shared.Core.Animations;
import Shared.Core.Animations.States;
import Tests.Shared.Core.Animations.AnimationStateTestHelpers;

TEST(LegsJumpAnimationStateTest, HandlesJumpTransitions)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::LegsJumpAnimationState>(
      Soldank::AnimationType::Jump);
    auto params = Soldank::Test::CreateHandleInputParams();
    params.control.up = false;
    params.on_ground = false;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Fall);

    state.SetFrame(state.GetFramesCount());
    params = Soldank::Test::CreateHandleInputParams();
    params.control.up = true;
    params.control.left = true;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::RunBack);
}

TEST(LegsJumpAnimationStateTest, HeldJumpTransitionsOnlyOnLastFrame)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::LegsJumpAnimationState>(
      Soldank::AnimationType::Jump);

    for (unsigned int frame = 1; frame < state.GetFramesCount(); frame += 1) {
        auto params = Soldank::Test::CreateHandleInputParams();
        params.control.up = true;
        params.control.left = true;
        state.SetFrame(frame);

        Soldank::Test::ExpectNoTransition(state.HandleInput(params));
    }

    auto params = Soldank::Test::CreateHandleInputParams();
    params.control.up = true;
    params.control.left = true;
    state.SetFrame(state.GetFramesCount());

    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::RunBack);
}
