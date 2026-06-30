#include <gtest/gtest.h>

import Shared.Core.Animations;
import Shared.Core.Animations.States;
import Tests.Shared.Core.Animations.AnimationStateTestHelpers;

TEST(LegsGetUpAnimationStateTest, HandlesCancelFinalAndLateJumpTransitions)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::LegsGetUpAnimationState>(
      Soldank::AnimationType::GetUp);
    auto params = Soldank::Test::CreateHandleInputParams();
    params.control.throw_grenade = true;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Stand);

    state.SetFrame(21);
    params = Soldank::Test::CreateHandleInputParams();
    params.control.up = true;
    params.control.left = true;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::JumpSide, 1);
}

TEST(LegsGetUpAnimationStateTest, JumpSideIsAvailableFromFrameTwentyOneUntilBeforeFinalFrame)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::LegsGetUpAnimationState>(
      Soldank::AnimationType::GetUp);

    for (unsigned int frame = 1; frame <= 20; frame += 1) {
        auto params = Soldank::Test::CreateHandleInputParams();
        params.control.up = true;
        params.control.left = true;
        state.SetFrame(frame);

        Soldank::Test::ExpectNoTransition(state.HandleInput(params));
    }

    for (unsigned int frame = 21; frame < state.GetFramesCount(); frame += 1) {
        auto params = Soldank::Test::CreateHandleInputParams();
        params.control.up = true;
        params.control.left = true;
        state.SetFrame(frame);

        Soldank::Test::ExpectTransition(
          state.HandleInput(params), Soldank::AnimationType::JumpSide, frame - 20);
    }

    auto params = Soldank::Test::CreateHandleInputParams();
    params.control.up = true;
    params.control.left = true;
    state.SetFrame(state.GetFramesCount());

    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Stand);
}

TEST(LegsGetUpAnimationStateTest, GroundedRunningInputDoesNotTransitionBeforeFinalFrame)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::LegsGetUpAnimationState>(
      Soldank::AnimationType::GetUp);

    for (unsigned int frame = 1; frame < state.GetFramesCount(); frame += 1) {
        auto params = Soldank::Test::CreateHandleInputParams();
        params.control.left = true;
        state.SetFrame(frame);

        Soldank::Test::ExpectNoTransition(state.HandleInput(params));
    }

    auto params = Soldank::Test::CreateHandleInputParams();
    params.control.left = true;
    state.SetFrame(state.GetFramesCount());

    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Stand);
}
