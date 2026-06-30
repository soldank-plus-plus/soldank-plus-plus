#include <gtest/gtest.h>

import Shared.Core.Animations;
import Shared.Core.Animations.States;
import Shared.Core.Physics.Constants;
import Shared.Core.Types.WeaponType;
import Tests.Shared.Core.Animations.AnimationStateTestHelpers;

namespace
{
Soldank::BodyGetUpAnimationState CreateBodyGetUpAnimationState(
  const Soldank::AnimationDataManager& animation_data_manager)
{
    return Soldank::BodyGetUpAnimationState{ animation_data_manager };
}
} // namespace

TEST(BodyGetUpAnimationStateTest, TransitionsToChangeWhenChangeWeaponIsPressed)
{
    auto animation_data_manager = Soldank::Test::CreateAnimationDataManager(
      Soldank::AnimationType::GetUp);
    auto body_get_up_animation_state = CreateBodyGetUpAnimationState(animation_data_manager);
    auto params = Soldank::Test::CreateHandleInputParams();
    params.control.change = true;

    const auto transition = body_get_up_animation_state.HandleInput(params);

    Soldank::Test::ExpectTransition(transition, Soldank::AnimationType::Change);
}

TEST(BodyGetUpAnimationStateTest, TransitionsToThrowWeaponWhenDropIsPressedWithWeaponEquipped)
{
    auto animation_data_manager = Soldank::Test::CreateAnimationDataManager(
      Soldank::AnimationType::GetUp);
    auto body_get_up_animation_state = CreateBodyGetUpAnimationState(animation_data_manager);
    auto params = Soldank::Test::CreateHandleInputParams();
    params.control.drop = true;

    const auto transition = body_get_up_animation_state.HandleInput(params);

    Soldank::Test::ExpectTransition(transition, Soldank::AnimationType::ThrowWeapon);
}

TEST(BodyGetUpAnimationStateTest, DoesNotTransitionToThrowWeaponWhenNoWeaponIsEquipped)
{
    auto animation_data_manager = Soldank::Test::CreateAnimationDataManager(
      Soldank::AnimationType::GetUp);
    auto body_get_up_animation_state = CreateBodyGetUpAnimationState(animation_data_manager);
    auto params = Soldank::Test::CreateHandleInputParams();
    params.control.drop = true;
    params.weapons[0] = Soldank::Test::CreateWeapon(Soldank::WeaponType::NoWeapon);

    const auto transition = body_get_up_animation_state.HandleInput(params);

    ASSERT_FALSE(transition.has_value());
}

TEST(BodyGetUpAnimationStateTest, TransitionsToThrowWhenGrenadeIsPressedEvenIfGrenadeWasBlocked)
{
    auto animation_data_manager = Soldank::Test::CreateAnimationDataManager(
      Soldank::AnimationType::GetUp);
    auto body_get_up_animation_state = CreateBodyGetUpAnimationState(animation_data_manager);
    auto params = Soldank::Test::CreateHandleInputParams();
    params.control.throw_grenade = true;
    params.grenade_can_throw = false;

    const auto transition = body_get_up_animation_state.HandleInput(params);

    Soldank::Test::ExpectTransition(transition, Soldank::AnimationType::Throw);
    EXPECT_TRUE(params.grenade_can_throw);
}

TEST(BodyGetUpAnimationStateTest, TransitionsToPunchWhenFiringWithoutWeapon)
{
    auto animation_data_manager = Soldank::Test::CreateAnimationDataManager(
      Soldank::AnimationType::GetUp);
    auto body_get_up_animation_state = CreateBodyGetUpAnimationState(animation_data_manager);
    auto params = Soldank::Test::CreateHandleInputParams();
    params.control.fire = true;
    params.weapons[0] = Soldank::Test::CreateWeapon(Soldank::WeaponType::NoWeapon);

    const auto transition = body_get_up_animation_state.HandleInput(params);

    Soldank::Test::ExpectTransition(transition, Soldank::AnimationType::Punch);
}

TEST(BodyGetUpAnimationStateTest, TransitionsToPunchWhenFiringKnife)
{
    auto animation_data_manager = Soldank::Test::CreateAnimationDataManager(
      Soldank::AnimationType::GetUp);
    auto body_get_up_animation_state = CreateBodyGetUpAnimationState(animation_data_manager);
    auto params = Soldank::Test::CreateHandleInputParams();
    params.control.fire = true;
    params.weapons[0] = Soldank::Test::CreateWeapon(Soldank::WeaponType::Knife);

    const auto transition = body_get_up_animation_state.HandleInput(params);

    Soldank::Test::ExpectTransition(transition, Soldank::AnimationType::Punch);
}

TEST(BodyGetUpAnimationStateTest, DoesNotTransitionToStanceAnimationBeforeLastFrame)
{
    auto animation_data_manager = Soldank::Test::CreateAnimationDataManager(
      Soldank::AnimationType::GetUp);
    auto body_get_up_animation_state = CreateBodyGetUpAnimationState(animation_data_manager);

    for (unsigned int frame = 1; frame < body_get_up_animation_state.GetFramesCount(); frame += 1) {
        auto params = Soldank::Test::CreateHandleInputParams();
        params.stance = Soldank::PhysicsConstants::STANCE_STAND;
        body_get_up_animation_state.SetFrame(frame);

        const auto transition = body_get_up_animation_state.HandleInput(params);

        ASSERT_FALSE(transition.has_value()) << "frame " << frame;
    }
}

TEST(BodyGetUpAnimationStateTest, TransitionsToStandOnLastFrameWhenSoldierStanceIsStand)
{
    auto animation_data_manager = Soldank::Test::CreateAnimationDataManager(
      Soldank::AnimationType::GetUp);
    auto body_get_up_animation_state = CreateBodyGetUpAnimationState(animation_data_manager);
    body_get_up_animation_state.SetFrame(body_get_up_animation_state.GetFramesCount());
    auto params = Soldank::Test::CreateHandleInputParams();
    params.stance = Soldank::PhysicsConstants::STANCE_STAND;

    const auto transition = body_get_up_animation_state.HandleInput(params);

    Soldank::Test::ExpectTransition(transition, Soldank::AnimationType::Stand);
}

TEST(BodyGetUpAnimationStateTest, TransitionsToAimOnLastFrameWhenSoldierStanceIsCrouch)
{
    auto animation_data_manager = Soldank::Test::CreateAnimationDataManager(
      Soldank::AnimationType::GetUp);
    auto body_get_up_animation_state = CreateBodyGetUpAnimationState(animation_data_manager);
    body_get_up_animation_state.SetFrame(body_get_up_animation_state.GetFramesCount());
    auto params = Soldank::Test::CreateHandleInputParams();
    params.stance = Soldank::PhysicsConstants::STANCE_CROUCH;

    const auto transition = body_get_up_animation_state.HandleInput(params);

    Soldank::Test::ExpectTransition(transition, Soldank::AnimationType::Aim);
}

TEST(BodyGetUpAnimationStateTest, TransitionsToProneOnLastFrameWhenSoldierStanceIsProne)
{
    auto animation_data_manager = Soldank::Test::CreateAnimationDataManager(
      Soldank::AnimationType::GetUp);
    auto body_get_up_animation_state = CreateBodyGetUpAnimationState(animation_data_manager);
    body_get_up_animation_state.SetFrame(body_get_up_animation_state.GetFramesCount());
    auto params = Soldank::Test::CreateHandleInputParams();
    params.stance = Soldank::PhysicsConstants::STANCE_PRONE;

    const auto transition = body_get_up_animation_state.HandleInput(params);

    Soldank::Test::ExpectTransition(transition, Soldank::AnimationType::Prone);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
