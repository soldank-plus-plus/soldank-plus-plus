#include <gtest/gtest.h>

import Shared.Core.Animations;
import Shared.Core.Animations.States;
import Shared.Core.Physics.Constants;
import Tests.Shared.Core.Animations.AnimationStateTestHelpers;

TEST(BodyChangeAnimationStateTest, HandlesRollTransitionsAndFinalStanceTransitions)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::BodyChangeAnimationState>(
      Soldank::AnimationType::Change);
    auto params = Soldank::Test::CreateHandleInputParams();
    params.legs_animation_type = Soldank::AnimationType::Roll;

    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Roll);

    state.SetFrame(state.GetFramesCount());
    params = Soldank::Test::CreateHandleInputParams();
    params.stance = Soldank::PhysicsConstants::STANCE_PRONE;

    Soldank::Test::ExpectTransition(
      state.HandleInput(params), Soldank::AnimationType::Prone, 26);
}

TEST(BodyChangeAnimationStateTest, SkipsFrameTwoBeforeEvaluatingFinalTransition)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::BodyChangeAnimationState>(
      Soldank::AnimationType::Change);
    auto params = Soldank::Test::CreateHandleInputParams();
    state.SetFrame(2);

    Soldank::Test::ExpectNoTransition(state.HandleInput(params));
    EXPECT_EQ(state.GetFrame(), 3U);
}

TEST(BodyChangeAnimationStateTest, SwitchesWeaponOnlyOnFrameTwentyFive)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::BodyChangeAnimationState>(
      Soldank::AnimationType::Change);

    for (unsigned int frame = 1; frame <= state.GetFramesCount(); frame += 1) {
        auto update_params = Soldank::Test::CreateUpdateParams();
        state.SetFrame(frame);
        state.Update(update_params);

        EXPECT_EQ(update_params.should_switch_weapon, frame == 25) << "frame " << frame;
    }
}

TEST(BodyChangeAnimationStateTest, TransitionsToStanceAnimationOnlyOnLastFrame)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::BodyChangeAnimationState>(
      Soldank::AnimationType::Change);

    for (unsigned int frame = 3; frame < state.GetFramesCount(); frame += 1) {
        auto params = Soldank::Test::CreateHandleInputParams();
        state.SetFrame(frame);
        Soldank::Test::ExpectNoTransition(state.HandleInput(params));
    }

    auto params = Soldank::Test::CreateHandleInputParams();
    state.SetFrame(state.GetFramesCount());
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Aim);
}
