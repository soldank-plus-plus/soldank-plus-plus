#include <gtest/gtest.h>

import Shared.Core.Animations;
import Shared.Core.Animations.States;
import Shared.Core.Physics.Constants;
import Shared.Core.Types.WeaponType;
import Tests.Shared.Core.Animations.AnimationStateTestHelpers;

TEST(BodyProneAnimationStateTest, HandlesCommonActionTransitions)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::BodyProneAnimationState>(
      Soldank::AnimationType::Prone);
    auto params = Soldank::Test::CreateHandleInputParams();
    params.control.change = true;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Change);

    params = Soldank::Test::CreateHandleInputParams();
    params.control.drop = true;
    Soldank::Test::ExpectTransition(
      state.HandleInput(params), Soldank::AnimationType::ThrowWeapon);

    params = Soldank::Test::CreateHandleInputParams();
    params.control.throw_grenade = true;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Throw);

    params = Soldank::Test::CreateHandleInputParams();
    params.control.fire = true;
    params.weapons[0] = Soldank::Test::CreateWeapon(Soldank::WeaponType::NoWeapon);
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Punch);
}

TEST(BodyProneAnimationStateTest, HandlesProneSpecificTransitions)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::BodyProneAnimationState>(
      Soldank::AnimationType::Prone);
    auto params = Soldank::Test::CreateHandleInputParams();
    params.stance = Soldank::PhysicsConstants::STANCE_STAND;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::GetUp, 9);

    params = Soldank::Test::CreateHandleInputParams();
    params.legs_animation_type = Soldank::AnimationType::ProneMove;
    params.control.left = true;
    Soldank::Test::ExpectTransition(
      state.HandleInput(params), Soldank::AnimationType::ProneMove);

    state.SetFrame(23);
    params = Soldank::Test::CreateHandleInputParams();
    params.control.down = true;
    params.control.left = true;
    params.legs_animation_type = Soldank::AnimationType::Roll;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Roll);
}

TEST(BodyProneAnimationStateTest, AllowsRollingOnlyFromFrameTwentyThree)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::BodyProneAnimationState>(
      Soldank::AnimationType::Prone);

    for (unsigned int frame = 1; frame < 23; frame += 1) {
        auto params = Soldank::Test::CreateHandleInputParams();
        params.control.down = true;
        params.control.left = true;
        params.legs_animation_type = Soldank::AnimationType::Roll;
        state.SetFrame(frame);

        Soldank::Test::ExpectNoTransition(state.HandleInput(params));
    }

    auto params = Soldank::Test::CreateHandleInputParams();
    params.control.down = true;
    params.control.left = true;
    params.legs_animation_type = Soldank::AnimationType::Roll;
    state.SetFrame(23);

    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Roll);
}
