#include <gtest/gtest.h>

import Shared.Core.Animations;
import Shared.Core.Animations.States;
import Shared.Core.Physics.Constants;
import Shared.Core.Types.WeaponType;
import Tests.Shared.Core.Animations.AnimationStateTestHelpers;

TEST(BodyAimAnimationStateTest, TransitionsToChangeWhenChangeWeaponIsPressed)
{
    auto animation_data_manager = Soldank::Test::CreateAnimationDataManager(
      Soldank::AnimationType::Aim);
    Soldank::BodyAimAnimationState body_aim_animation_state{ animation_data_manager };
    auto params = Soldank::Test::CreateHandleInputParams();
    params.control.change = true;

    const auto transition = body_aim_animation_state.HandleInput(params);

    Soldank::Test::ExpectTransition(transition, Soldank::AnimationType::Change);
}

TEST(BodyAimAnimationStateTest, TransitionsToThrowWeaponWhenDropIsPressedWithWeaponEquipped)
{
    auto animation_data_manager = Soldank::Test::CreateAnimationDataManager(
      Soldank::AnimationType::Aim);
    Soldank::BodyAimAnimationState body_aim_animation_state{ animation_data_manager };
    auto params = Soldank::Test::CreateHandleInputParams();
    params.control.drop = true;

    const auto transition = body_aim_animation_state.HandleInput(params);

    Soldank::Test::ExpectTransition(transition, Soldank::AnimationType::ThrowWeapon);
}

TEST(BodyAimAnimationStateTest, DoesNotTransitionToThrowWeaponWhenNoWeaponIsEquipped)
{
    auto animation_data_manager = Soldank::Test::CreateAnimationDataManager(
      Soldank::AnimationType::Aim);
    Soldank::BodyAimAnimationState body_aim_animation_state{ animation_data_manager };
    auto params = Soldank::Test::CreateHandleInputParams();
    params.control.drop = true;
    params.weapons[0] = Soldank::Test::CreateWeapon(Soldank::WeaponType::NoWeapon);

    const auto transition = body_aim_animation_state.HandleInput(params);

    ASSERT_FALSE(transition.has_value());
}

TEST(BodyAimAnimationStateTest, TransitionsToThrowWhenGrenadeCanBeThrown)
{
    auto animation_data_manager = Soldank::Test::CreateAnimationDataManager(
      Soldank::AnimationType::Aim);
    Soldank::BodyAimAnimationState body_aim_animation_state{ animation_data_manager };
    auto params = Soldank::Test::CreateHandleInputParams();
    params.control.throw_grenade = true;
    params.grenade_can_throw = true;

    const auto transition = body_aim_animation_state.HandleInput(params);

    Soldank::Test::ExpectTransition(transition, Soldank::AnimationType::Throw);
}

TEST(BodyAimAnimationStateTest, TransitionsToPunchWhenFiringWithoutWeapon)
{
    auto animation_data_manager = Soldank::Test::CreateAnimationDataManager(
      Soldank::AnimationType::Aim);
    Soldank::BodyAimAnimationState body_aim_animation_state{ animation_data_manager };
    auto params = Soldank::Test::CreateHandleInputParams();
    params.control.fire = true;
    params.weapons[0] = Soldank::Test::CreateWeapon(Soldank::WeaponType::NoWeapon);

    const auto transition = body_aim_animation_state.HandleInput(params);

    Soldank::Test::ExpectTransition(transition, Soldank::AnimationType::Punch);
}

TEST(BodyAimAnimationStateTest, TransitionsToPunchWhenFiringKnife)
{
    auto animation_data_manager = Soldank::Test::CreateAnimationDataManager(
      Soldank::AnimationType::Aim);
    Soldank::BodyAimAnimationState body_aim_animation_state{ animation_data_manager };
    auto params = Soldank::Test::CreateHandleInputParams();
    params.control.fire = true;
    params.weapons[0] = Soldank::Test::CreateWeapon(Soldank::WeaponType::Knife);

    const auto transition = body_aim_animation_state.HandleInput(params);

    Soldank::Test::ExpectTransition(transition, Soldank::AnimationType::Punch);
}

TEST(BodyAimAnimationStateTest, TransitionsToStandWhenSoldierStanceIsStand)
{
    auto animation_data_manager = Soldank::Test::CreateAnimationDataManager(
      Soldank::AnimationType::Aim);
    Soldank::BodyAimAnimationState body_aim_animation_state{ animation_data_manager };
    auto params = Soldank::Test::CreateHandleInputParams();
    params.stance = Soldank::PhysicsConstants::STANCE_STAND;

    const auto transition = body_aim_animation_state.HandleInput(params);

    Soldank::Test::ExpectTransition(transition, Soldank::AnimationType::Stand);
}

TEST(BodyAimAnimationStateTest, TransitionsToProneWhenSoldierStanceIsProne)
{
    auto animation_data_manager = Soldank::Test::CreateAnimationDataManager(
      Soldank::AnimationType::Aim);
    Soldank::BodyAimAnimationState body_aim_animation_state{ animation_data_manager };
    auto params = Soldank::Test::CreateHandleInputParams();
    params.stance = Soldank::PhysicsConstants::STANCE_PRONE;

    const auto transition = body_aim_animation_state.HandleInput(params);

    Soldank::Test::ExpectTransition(transition, Soldank::AnimationType::Prone);
}

TEST(BodyAimAnimationStateTest, DoesNotTransitionWhenNoTransitionInputIsProvided)
{
    auto animation_data_manager = Soldank::Test::CreateAnimationDataManager(
      Soldank::AnimationType::Aim);
    Soldank::BodyAimAnimationState body_aim_animation_state{ animation_data_manager };
    auto params = Soldank::Test::CreateHandleInputParams();
    params.grenade_can_throw = false;

    const auto transition = body_aim_animation_state.HandleInput(params);

    ASSERT_FALSE(transition.has_value());
    EXPECT_TRUE(params.grenade_can_throw);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
