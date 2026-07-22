#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <list>

import Networking.ReconciliationTimeline;

import Shared.Networking.NetworkPackets;

using namespace Soldank;

namespace
{
SoldierInputPacket MakeInput(std::uint32_t input_sequence_id,
                             std::uint32_t client_tick,
                             std::uint32_t apply_server_tick)
{
    SoldierInputPacket input{};
    input.input_sequence_id = input_sequence_id;
    input.client_tick = client_tick;
    input.apply_server_tick = apply_server_tick;
    return input;
}
} // namespace

TEST(ReconciliationTimelineTest, SelectsOnlyInputsAfterTheAuthoritativeServerTick)
{
    const std::list<SoldierInputPacket> prediction_inputs{
        MakeInput(10, 20, 100),
        MakeInput(11, 21, 101),
        MakeInput(12, 22, 102),
    };

    const auto replay_inputs = SelectReplayInputs(prediction_inputs, 100);

    ASSERT_EQ(replay_inputs.size(), 2);
    EXPECT_EQ(replay_inputs[0].input_sequence_id, 11);
    EXPECT_EQ(replay_inputs[1].input_sequence_id, 12);
}

TEST(ReconciliationTimelineTest, OrdersReplayByApplicationTickThenClientTick)
{
    const std::list<SoldierInputPacket> prediction_inputs{
        MakeInput(14, 24, 104),
        MakeInput(13, 23, 103),
        MakeInput(12, 22, 102),
        MakeInput(11, 21, 103),
    };

    const auto replay_inputs = SelectReplayInputs(prediction_inputs, 101);

    ASSERT_EQ(replay_inputs.size(), 4);
    EXPECT_EQ(replay_inputs[0].input_sequence_id, 12);
    EXPECT_EQ(replay_inputs[1].input_sequence_id, 11);
    EXPECT_EQ(replay_inputs[2].input_sequence_id, 13);
    EXPECT_EQ(replay_inputs[3].input_sequence_id, 14);
}

TEST(ReconciliationTimelineTest, SelectionDoesNotDependOnAcknowledgementSequenceOrder)
{
    const std::list<SoldierInputPacket> prediction_inputs{
        MakeInput(100, 20, 99),
        MakeInput(1, 21, 101),
    };

    const auto replay_inputs = SelectReplayInputs(prediction_inputs, 100);

    ASSERT_EQ(replay_inputs.size(), 1);
    EXPECT_EQ(replay_inputs.front().input_sequence_id, 1);
}

TEST(ReconciliationTimelineTest, SelectsAndOrdersReplayInputsAcrossTickWraparound)
{
    constexpr std::uint32_t MAX_TICK = std::numeric_limits<std::uint32_t>::max();
    const std::list<SoldierInputPacket> prediction_inputs{
        MakeInput(10, MAX_TICK - 2, MAX_TICK - 2),
        MakeInput(11, MAX_TICK, MAX_TICK),
        MakeInput(12, 0, 0),
        MakeInput(13, 1, 1),
    };

    const auto replay_inputs = SelectReplayInputs(prediction_inputs, MAX_TICK - 1);

    ASSERT_EQ(replay_inputs.size(), 3);
    EXPECT_EQ(replay_inputs[0].apply_server_tick, MAX_TICK);
    EXPECT_EQ(replay_inputs[1].apply_server_tick, 0U);
    EXPECT_EQ(replay_inputs[2].apply_server_tick, 1U);
}
