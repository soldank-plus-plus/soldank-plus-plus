#include <gtest/gtest.h>

import Shared.Core.Animations;
import Shared.Core.Animations.States;
import Tests.Shared.Core.Animations.AnimationStateTestHelpers;

TEST(LegsJumpSideAnimationStateTest, HandlesJetAndLandingTransitions)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::LegsJumpSideAnimationState>(
      Soldank::AnimationType::JumpSide);
    auto params = Soldank::Test::CreateHandleInputParams();
    params.control.jets = true;
    params.control.left = true;
    params.direction = 1;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::RollBack);

    params = Soldank::Test::CreateHandleInputParams();
    params.on_ground = false;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::Fall);
}

TEST(LegsJumpSideAnimationStateTest, HandlesChainedGroundTransitions)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::LegsJumpSideAnimationState>(
      Soldank::AnimationType::JumpSide);
    state.SetFrame(state.GetFramesCount());
    auto params = Soldank::Test::CreateHandleInputParams();
    params.control.up = true;
    params.control.right = true;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::JumpSide);

    params = Soldank::Test::CreateHandleInputParams();
    params.control.down = true;
    params.control.right = true;
    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::CrouchRun);
}

TEST(LegsJumpSideAnimationStateTest, ChainJumpSideRequiresLastFrame)
{
    auto state = Soldank::Test::CreateAnimationState<Soldank::LegsJumpSideAnimationState>(
      Soldank::AnimationType::JumpSide);

    for (unsigned int frame = 1; frame < state.GetFramesCount(); frame += 1) {
        auto params = Soldank::Test::CreateHandleInputParams();
        params.control.up = true;
        params.control.right = true;
        state.SetFrame(frame);

        Soldank::Test::ExpectNoTransition(state.HandleInput(params));
    }

    auto params = Soldank::Test::CreateHandleInputParams();
    params.control.up = true;
    params.control.right = true;
    state.SetFrame(state.GetFramesCount());

    Soldank::Test::ExpectTransition(state.HandleInput(params), Soldank::AnimationType::JumpSide);
}
