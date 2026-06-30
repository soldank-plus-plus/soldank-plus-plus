#include <gtest/gtest.h>

import Shared.Core.Animations;
import Shared.Core.Animations.States;
import Tests.Shared.Core.Animations.AnimationStateTestHelpers;

TEST(LegsProneAnimationStateTest, HandlesProneTransitionsAndDirectionSync)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::LegsProneAnimationState>(
      Soldank::AnimationType::Prone);
    auto params = Soldank::Test::CreateHandleInputParams();
    params.old_direction = -1;
    Soldank::Test::ExpectNoTransition(state.HandleInput(params));
    EXPECT_EQ(params.old_direction, params.direction);

    state.SetFrame(24);
    params = Soldank::Test::CreateHandleInputParams();
    params.control.prone = true;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::GetUp, 9);

    state.SetFrame(26);
    params = Soldank::Test::CreateHandleInputParams();
    params.control.left = true;
    Soldank::Test::ExpectTransition(
      state.HandleInput(params), Soldank::AnimationType::ProneMove);
}

TEST(LegsProneAnimationStateTest, DirectionChangeDoesNotCancelProneUntilFrameTwentyFour)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::LegsProneAnimationState>(
      Soldank::AnimationType::Prone);

    for (unsigned int frame = 1; frame <= 23; frame += 1) {
        auto params = Soldank::Test::CreateHandleInputParams();
        params.direction = 1;
        params.old_direction = -1;
        state.SetFrame(frame);

        Soldank::Test::ExpectNoTransition(state.HandleInput(params));
        EXPECT_EQ(params.old_direction, params.direction) << "frame " << frame;
    }

    auto params = Soldank::Test::CreateHandleInputParams();
    params.direction = 1;
    params.old_direction = -1;
    state.SetFrame(24);

    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::GetUp, 9);
}

TEST(LegsProneAnimationStateTest, ProneMoveStartsOnlyAfterFrameTwentyFive)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::LegsProneAnimationState>(
      Soldank::AnimationType::Prone);

    for (unsigned int frame = 1; frame <= 25; frame += 1) {
        auto params = Soldank::Test::CreateHandleInputParams();
        params.control.left = true;
        state.SetFrame(frame);

        Soldank::Test::ExpectNoTransition(state.HandleInput(params));
    }

    auto params = Soldank::Test::CreateHandleInputParams();
    params.control.left = true;
    state.SetFrame(26);

    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::ProneMove);
}
