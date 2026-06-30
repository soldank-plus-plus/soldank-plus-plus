#include <gtest/gtest.h>

import Shared.Core.Animations;
import Shared.Core.Animations.States;
import Shared.Core.Physics.Constants;
import Tests.Shared.Core.Animations.AnimationStateTestHelpers;

TEST(BodyPunchAnimationStateTest, MirrorsLegAnimationInterruptions)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::BodyPunchAnimationState>(
      Soldank::AnimationType::Punch);
    auto params = Soldank::Test::CreateHandleInputParams();
    params.legs_animation_type = Soldank::AnimationType::RollBack;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::RollBack);

    params = Soldank::Test::CreateHandleInputParams();
    params.legs_animation_type = Soldank::AnimationType::GetUp;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::GetUp, 9);
}

TEST(BodyPunchAnimationStateTest, HandlesChangeAndFinalStanceTransitions)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::BodyPunchAnimationState>(
      Soldank::AnimationType::Punch);
    auto params = Soldank::Test::CreateHandleInputParams();
    params.control.change = true;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Change);

    state.SetFrame(state.GetFramesCount());
    params = Soldank::Test::CreateHandleInputParams();
    params.stance = Soldank::PhysicsConstants::STANCE_STAND;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Stand);
}

TEST(BodyPunchAnimationStateTest, TransitionsToStanceAnimationOnlyOnLastFrame)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::BodyPunchAnimationState>(
      Soldank::AnimationType::Punch);

    for (unsigned int frame = 1; frame < state.GetFramesCount(); frame += 1) {
        auto params = Soldank::Test::CreateHandleInputParams();
        state.SetFrame(frame);
        Soldank::Test::ExpectNoTransition(state.HandleInput(params));
    }

    auto params = Soldank::Test::CreateHandleInputParams();
    state.SetFrame(state.GetFramesCount());
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Aim);
}
