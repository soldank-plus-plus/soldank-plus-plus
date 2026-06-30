#include <gtest/gtest.h>

import Shared.Core.Animations;
import Shared.Core.Animations.States;
import Shared.Core.Physics.Constants;
import Shared.Core.Types.WeaponType;
import Tests.Shared.Core.Animations.AnimationStateTestHelpers;

TEST(BodyThrowWeaponAnimationStateTest, HandlesEnterAndExitSideEffects)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::BodyThrowWeaponAnimationState>(
      Soldank::AnimationType::ThrowWeapon);
    auto enter_params = Soldank::Test::CreateEnterParams();
    enter_params.weapons[0] = Soldank::Test::CreateWeapon(Soldank::WeaponType::Knife);
    state.Enter(enter_params);
    EXPECT_EQ(state.GetSpeed(), 2);

    auto handle_params = Soldank::Test::CreateHandleInputParams();
    handle_params.control.change = true;
    Soldank::Test::ExpectTransition(
      state.HandleInput(handle_params), Soldank::AnimationType::Change);

    auto exit_params = Soldank::Test::CreateExitParams();
    state.Exit(exit_params);
    EXPECT_FALSE(exit_params.should_throw_active_weapon);
}

TEST(BodyThrowWeaponAnimationStateTest, HandlesKnifeAndFinalTransitions)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::BodyThrowWeaponAnimationState>(
      Soldank::AnimationType::ThrowWeapon);
    auto params = Soldank::Test::CreateHandleInputParams();
    params.weapons[0] = Soldank::Test::CreateWeapon(Soldank::WeaponType::Knife);
    params.control.fire = true;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Punch);

    state.SetFrame(state.GetFramesCount());
    params = Soldank::Test::CreateHandleInputParams();
    params.stance = Soldank::PhysicsConstants::STANCE_STAND;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Stand);
}

TEST(BodyThrowWeaponAnimationStateTest, KnifeDropFinishesThrowOnFrameSixteen)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::BodyThrowWeaponAnimationState>(
      Soldank::AnimationType::ThrowWeapon);

    for (unsigned int frame = 1; frame < state.GetFramesCount(); frame += 1) {
        auto params = Soldank::Test::CreateHandleInputParams();
        params.weapons[0] = Soldank::Test::CreateWeapon(Soldank::WeaponType::Knife);
        params.control.drop = true;
        state.SetFrame(frame);

        if (frame == 16) {
            Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Aim);
        } else {
            Soldank::Test::ExpectNoTransition(state.HandleInput(params));
        }
    }

    auto params = Soldank::Test::CreateHandleInputParams();
    params.weapons[0] = Soldank::Test::CreateWeapon(Soldank::WeaponType::Knife);
    params.control.drop = true;
    state.SetFrame(state.GetFramesCount());
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Aim);
}
