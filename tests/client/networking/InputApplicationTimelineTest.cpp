#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <optional>

import Networking.InputApplicationTimeline;

using namespace Soldank;

namespace
{
constexpr std::uint32_t INPUT_DELAY_TICKS = 5;
}

TEST(InputApplicationTimelineTest, WaitsForAnAuthoritativeOffsetBeforeAnchoring)
{
    InputApplicationTimeline timeline;

    EXPECT_EQ(timeline.Schedule(100, std::nullopt, INPUT_DELAY_TICKS), std::nullopt);
    EXPECT_EQ(timeline.Schedule(101, 50, INPUT_DELAY_TICKS), 156);
}

TEST(InputApplicationTimelineTest, CalculatesLatencyAwareDelayFromRoundTripTime)
{
    EXPECT_EQ(CalculateInputDelayTicks(0), 5);
    EXPECT_EQ(CalculateInputDelayTicks(50), 5);
    EXPECT_EQ(CalculateInputDelayTicks(70), 7);
    EXPECT_EQ(CalculateInputDelayTicks(275), 19);
}

TEST(InputApplicationTimelineTest, AdvancesOncePerClientTickWhenOffsetSamplesFluctuate)
{
    InputApplicationTimeline timeline;

    EXPECT_EQ(timeline.Schedule(100, 50, INPUT_DELAY_TICKS), 155);
    EXPECT_EQ(timeline.Schedule(101, 49, INPUT_DELAY_TICKS), 156);
    EXPECT_EQ(timeline.Schedule(102, 51, INPUT_DELAY_TICKS), 157);
    EXPECT_EQ(timeline.Schedule(103, 48, INPUT_DELAY_TICKS), 158);
}

TEST(InputApplicationTimelineTest, ResetAllowsANewAuthoritativeAnchor)
{
    InputApplicationTimeline timeline;
    ASSERT_EQ(timeline.Schedule(100, 50, INPUT_DELAY_TICKS), 155);

    timeline.Reset();

    EXPECT_EQ(timeline.Schedule(500, 80, INPUT_DELAY_TICKS), 585);
}

TEST(InputApplicationTimelineTest, ResynchronizesForwardWhenHigherDelayExceedsCurrentLead)
{
    InputApplicationTimeline timeline;
    ASSERT_EQ(timeline.Schedule(100, 50, INPUT_DELAY_TICKS), 155);
    ASSERT_FALSE(timeline.WasLastScheduleResynchronized());

    EXPECT_EQ(timeline.Schedule(101, 42, 19), 162);
    EXPECT_TRUE(timeline.WasLastScheduleResynchronized());
    EXPECT_EQ(timeline.GetActiveInputDelayTicks(), 19);

    EXPECT_EQ(timeline.Schedule(102, 43, 19), 163);
    EXPECT_FALSE(timeline.WasLastScheduleResynchronized());
}

TEST(InputApplicationTimelineTest, HigherTargetDoesNotResynchronizeWhenCurrentLeadIsSufficient)
{
    InputApplicationTimeline timeline;
    ASSERT_EQ(timeline.Schedule(100, 50, INPUT_DELAY_TICKS), 155);

    EXPECT_EQ(timeline.Schedule(101, 48, 7), 156);
    EXPECT_FALSE(timeline.WasLastScheduleResynchronized());
    EXPECT_EQ(timeline.GetActiveInputDelayTicks(), 7);
}

TEST(InputApplicationTimelineTest, LowerTargetDoesNotReinterpretExistingApplicationTicks)
{
    InputApplicationTimeline timeline;
    ASSERT_EQ(timeline.Schedule(100, 50, 19), 169);

    EXPECT_EQ(timeline.Schedule(101, 50, INPUT_DELAY_TICKS), 170);
    EXPECT_FALSE(timeline.WasLastScheduleResynchronized());
}

TEST(InputApplicationTimelineTest, ApplicationTicksAdvanceAcrossWraparound)
{
    constexpr std::uint32_t MAX_TICK = std::numeric_limits<std::uint32_t>::max();
    InputApplicationTimeline timeline;

    EXPECT_EQ(timeline.Schedule(MAX_TICK - 1, 0, INPUT_DELAY_TICKS), 3U);
    EXPECT_EQ(timeline.Schedule(MAX_TICK, 0, INPUT_DELAY_TICKS), 4U);
    EXPECT_EQ(timeline.Schedule(0, 0, INPUT_DELAY_TICKS), 5U);
    EXPECT_FALSE(timeline.WasLastScheduleResynchronized());
}
