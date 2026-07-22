#include <gtest/gtest.h>

#include <cstdint>
#include <limits>

import Runtime.ServerCommandQueues;
import Sessions.PlayerSessionManager;

import Shared.Core.Simulation.SimulationCommands;

using namespace Soldank;

namespace
{
PlayerInputCommand MakePlayerInput(std::uint32_t input_sequence_id)
{
    return PlayerInputCommand{
        .soldier_id = 1,
        .input_sequence_id = input_sequence_id,
        .client_tick = 0,
        .apply_server_tick = 10,
        .control = {},
        .mouse_map_position = {},
    };
}
} // namespace

TEST(ServerCommandQueuesTests, NewestInputReceivedBeforeOneServerTickIsSelected)
{
    ServerCommandQueues command_queues;
    command_queues.StorePendingPlayerInput(MakePlayerInput(10));
    command_queues.StorePendingPlayerInput(MakePlayerInput(11));
    command_queues.StorePendingPlayerInput(MakePlayerInput(12));

#ifndef NDEBUG
    EXPECT_EQ(command_queues.GetInputDebugStats().received_input_count, 3);
#endif

    const auto selected_inputs = command_queues.SelectPlayerInputsForSimulation(10);

    ASSERT_EQ(selected_inputs.size(), 1);
    EXPECT_EQ(selected_inputs[0].input_sequence_id, 12);
}

TEST(ServerCommandQueuesTests, SelectsOneNewestInputPerSoldier)
{
    ServerCommandQueues command_queues;
    command_queues.StorePendingPlayerInput(MakePlayerInput(10));
    auto second_soldier_input = MakePlayerInput(20);
    second_soldier_input.soldier_id = 2;
    command_queues.StorePendingPlayerInput(second_soldier_input);

    const auto selected_inputs = command_queues.SelectPlayerInputsForSimulation(10);

    ASSERT_EQ(selected_inputs.size(), 2);
    EXPECT_EQ(selected_inputs[0].input_sequence_id, 10);
    EXPECT_EQ(selected_inputs[1].input_sequence_id, 20);
    EXPECT_TRUE(command_queues.SelectPlayerInputsForSimulation(11).empty());
}

TEST(ServerCommandQueuesTests, SelectsNewestInputOnlyWhenItsServerTickIsDue)
{
    ServerCommandQueues command_queues;
    auto first_input = MakePlayerInput(10);
    first_input.apply_server_tick = 12;
    command_queues.StorePendingPlayerInput(first_input);
    auto newest_input = MakePlayerInput(11);
    newest_input.apply_server_tick = 12;
    command_queues.StorePendingPlayerInput(newest_input);

    EXPECT_TRUE(command_queues.SelectPlayerInputsForSimulation(11).empty());

    const auto selected_inputs = command_queues.SelectPlayerInputsForSimulation(12);
    ASSERT_EQ(selected_inputs.size(), 1);
    EXPECT_EQ(selected_inputs[0].input_sequence_id, 11);
}

TEST(ServerCommandQueuesTests, AppliesInputsThatMissedTheirScheduledServerTick)
{
    ServerCommandQueues command_queues;
    auto input = MakePlayerInput(10);
    input.apply_server_tick = 9;
    command_queues.StorePendingPlayerInput(input);

    const auto selected_inputs = command_queues.SelectPlayerInputsForSimulation(10);
    ASSERT_EQ(selected_inputs.size(), 1);
    EXPECT_EQ(selected_inputs[0].input_sequence_id, 10);
#ifndef NDEBUG
    EXPECT_EQ(command_queues.GetInputDebugStats().late_applied_input_count, 1);
#endif
}

TEST(ServerCommandQueuesTests, SelectsNewestInputWhenSeveralInputsAreLate)
{
    ServerCommandQueues command_queues;
    auto first_input = MakePlayerInput(10);
    first_input.apply_server_tick = 8;
    command_queues.StorePendingPlayerInput(first_input);
    auto newest_input = MakePlayerInput(11);
    newest_input.apply_server_tick = 9;
    command_queues.StorePendingPlayerInput(newest_input);

    const auto selected_inputs = command_queues.SelectPlayerInputsForSimulation(10);

    ASSERT_EQ(selected_inputs.size(), 1);
    EXPECT_EQ(selected_inputs[0].input_sequence_id, 11);
#ifndef NDEBUG
    EXPECT_EQ(command_queues.GetInputDebugStats().late_applied_input_count, 2);
    EXPECT_EQ(command_queues.GetInputDebugStats().superseded_input_count, 1);
#endif
}

TEST(PlayerSessionManagerTests, ReceiptDoesNotAdvanceAppliedInputId)
{
    PlayerSessionManager player_session_manager;

    EXPECT_TRUE(player_session_manager.ShouldAcceptInput(1, 10));
    player_session_manager.MarkInputReceived(1, 10);

    EXPECT_EQ(player_session_manager.GetLastReceivedInputId(1), 10);
    EXPECT_EQ(player_session_manager.GetLastAppliedInputId(1), 0);
    EXPECT_FALSE(player_session_manager.ShouldAcceptInput(1, 10));
    EXPECT_FALSE(player_session_manager.ShouldAcceptInput(1, 9));

    player_session_manager.MarkInputApplied(1, 10);
    EXPECT_EQ(player_session_manager.GetLastAppliedInputId(1), 10);
}

TEST(PlayerSessionManagerTests, AppliedInputIdCanRemainUnchangedThenJumpAfterSuperseding)
{
    ServerCommandQueues command_queues;
    PlayerSessionManager player_session_manager;
    command_queues.StorePendingPlayerInput(MakePlayerInput(10));
    command_queues.StorePendingPlayerInput(MakePlayerInput(11));
    command_queues.StorePendingPlayerInput(MakePlayerInput(12));

    player_session_manager.MarkInputReceived(1, 12);
    EXPECT_EQ(player_session_manager.GetLastAppliedInputId(1), 0U);

    const auto selected_inputs = command_queues.SelectPlayerInputsForSimulation(10);
    ASSERT_EQ(selected_inputs.size(), 1);
    player_session_manager.MarkInputApplied(1, selected_inputs.front().input_sequence_id);

    EXPECT_EQ(player_session_manager.GetLastAppliedInputId(1), 12U);
}

TEST(ServerCommandQueuesTests, SelectsDueInputsAcrossTickWraparound)
{
    constexpr std::uint32_t MAX_TICK = std::numeric_limits<std::uint32_t>::max();
    ServerCommandQueues command_queues;
    auto before_wrap = MakePlayerInput(10);
    before_wrap.apply_server_tick = MAX_TICK;
    command_queues.StorePendingPlayerInput(before_wrap);
    auto after_wrap = MakePlayerInput(11);
    after_wrap.apply_server_tick = 1;
    command_queues.StorePendingPlayerInput(after_wrap);

    const auto selected_at_wrap = command_queues.SelectPlayerInputsForSimulation(0);
    ASSERT_EQ(selected_at_wrap.size(), 1);
    EXPECT_EQ(selected_at_wrap.front().input_sequence_id, 10U);

    const auto selected_after_wrap = command_queues.SelectPlayerInputsForSimulation(1);
    ASSERT_EQ(selected_after_wrap.size(), 1);
    EXPECT_EQ(selected_after_wrap.front().input_sequence_id, 11U);
}

TEST(PlayerSessionManagerTests, AcceptsSequenceIdsAcrossWraparound)
{
    constexpr std::uint32_t MAX_SEQUENCE = std::numeric_limits<std::uint32_t>::max();
    PlayerSessionManager player_session_manager;
    player_session_manager.MarkInputReceived(1, MAX_SEQUENCE - 1);

    EXPECT_TRUE(player_session_manager.ShouldAcceptInput(1, MAX_SEQUENCE));
    player_session_manager.MarkInputReceived(1, MAX_SEQUENCE);
    EXPECT_TRUE(player_session_manager.ShouldAcceptInput(1, 0));
    player_session_manager.MarkInputReceived(1, 0);
    EXPECT_TRUE(player_session_manager.ShouldAcceptInput(1, 1));
    EXPECT_FALSE(player_session_manager.ShouldAcceptInput(1, MAX_SEQUENCE));
}
