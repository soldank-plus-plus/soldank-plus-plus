#include <gtest/gtest.h>

import Shared.Core.Animations;
import Shared.Core.Animations.States;
import Tests.Shared.Core.Animations.AnimationStateTestHelpers;

TEST(BodyProneMoveAnimationStateTest, HandlesCancellationAndMovementTransitions)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::BodyProneMoveAnimationState>(
      Soldank::AnimationType::ProneMove);
    auto params = Soldank::Test::CreateHandleInputParams();
    params.control.prone = true;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::GetUp, 9);

    params = Soldank::Test::CreateHandleInputParams();
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Prone, 26);

    state.SetFrame(state.GetFramesCount());
    params = Soldank::Test::CreateHandleInputParams();
    params.control.left = true;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Prone, 26);
}

TEST(BodyProneMoveAnimationStateTest, ReturnsToProneOnlyOnLastFrameWhileMoving)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::BodyProneMoveAnimationState>(
      Soldank::AnimationType::ProneMove);

    for (unsigned int frame = 1; frame < state.GetFramesCount(); frame += 1) {
        auto params = Soldank::Test::CreateHandleInputParams();
        params.control.left = true;
        state.SetFrame(frame);

        Soldank::Test::ExpectNoTransition(state.HandleInput(params));
    }

    auto params = Soldank::Test::CreateHandleInputParams();
    params.control.left = true;
    state.SetFrame(state.GetFramesCount());

    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Prone, 26);
}

TEST(BodyProneMoveAnimationStateTest, HandlesActionAndRollTransitions)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::BodyProneMoveAnimationState>(
      Soldank::AnimationType::ProneMove);
    auto params = Soldank::Test::CreateHandleInputParams();
    params.control.down = true;
    params.control.left = true;
    params.legs_animation_type = Soldank::AnimationType::RollBack;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::RollBack);

    params = Soldank::Test::CreateHandleInputParams();
    params.control.change = true;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Change);
}
