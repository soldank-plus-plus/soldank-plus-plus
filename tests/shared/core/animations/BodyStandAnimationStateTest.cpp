#include <gtest/gtest.h>

import Shared.Core.Animations;
import Shared.Core.Animations.States;
import Shared.Core.Physics.Constants;
import Shared.Core.Types.WeaponType;
import Tests.Shared.Core.Animations.AnimationStateTestHelpers;

TEST(BodyStandAnimationStateTest, HandlesActionTransitions)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::BodyStandAnimationState>(
      Soldank::AnimationType::Stand);
    auto params = Soldank::Test::CreateHandleInputParams();
    params.legs_animation_type = Soldank::AnimationType::Roll;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Roll);

    params = Soldank::Test::CreateHandleInputParams();
    params.control.drop = true;
    Soldank::Test::ExpectTransition(
      state.HandleInput(params), Soldank::AnimationType::ThrowWeapon);

    params = Soldank::Test::CreateHandleInputParams();
    params.control.fire = true;
    params.weapons[0] = Soldank::Test::CreateWeapon(Soldank::WeaponType::Knife);
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Punch);
}

TEST(BodyStandAnimationStateTest, HandlesStanceTransitions)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::BodyStandAnimationState>(
      Soldank::AnimationType::Stand);
    auto params = Soldank::Test::CreateHandleInputParams();
    params.stance = Soldank::PhysicsConstants::STANCE_CROUCH;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Aim);

    params.stance = Soldank::PhysicsConstants::STANCE_PRONE;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Prone);
}
