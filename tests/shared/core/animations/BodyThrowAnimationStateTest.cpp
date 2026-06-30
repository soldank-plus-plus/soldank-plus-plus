#include <gtest/gtest.h>

import Shared.Core.Animations;
import Shared.Core.Animations.States;
import Shared.Core.Physics.Constants;
import Tests.Shared.Core.Animations.AnimationStateTestHelpers;

TEST(BodyThrowAnimationStateTest, ClearsGrenadeAvailabilityOnEnter)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::BodyThrowAnimationState>(
      Soldank::AnimationType::Throw);
    auto params = Soldank::Test::CreateEnterParams();
    params.grenade_can_throw = true;

    state.Enter(params);

    EXPECT_FALSE(params.grenade_can_throw);
}

TEST(BodyThrowAnimationStateTest, HandlesInterruptionsAndReleaseTransitions)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::BodyThrowAnimationState>(
      Soldank::AnimationType::Throw);
    auto params = Soldank::Test::CreateHandleInputParams();
    params.legs_animation_type = Soldank::AnimationType::GetUp;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::GetUp, 9);

    params = Soldank::Test::CreateHandleInputParams();
    params.control.throw_grenade = false;
    params.stance = Soldank::PhysicsConstants::STANCE_PRONE;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Prone, 26);
}

TEST(BodyThrowAnimationStateTest, HoldingGrenadeTransitionsToStanceAnimationOnlyOnLastFrame)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::BodyThrowAnimationState>(
      Soldank::AnimationType::Throw);

    for (unsigned int frame = 1; frame < state.GetFramesCount(); frame += 1) {
        auto params = Soldank::Test::CreateHandleInputParams();
        params.control.throw_grenade = true;
        state.SetFrame(frame);
        Soldank::Test::ExpectNoTransition(state.HandleInput(params));
    }

    auto params = Soldank::Test::CreateHandleInputParams();
    params.control.throw_grenade = true;
    state.SetFrame(state.GetFramesCount());
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Aim);
}
