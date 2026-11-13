#include <gtest/gtest.h>

import Shared.Core.Map.PMSEnums;
import Shared.Core.Map.Polygons.PolygonBackgroundState;

using namespace Soldank;

TEST(PolygonBackgroundStateTest, TransitionRegionsEnterRetainAndExit)
{
    PolygonBackgroundState state{ .phase = PolygonBackgroundPhase::Normal, .polygon = -1 };
    PolygonBackgroundContact normal(state);
    EXPECT_FALSE(normal.Test(PMSPolygonType::Background, 4));
    normal.Finish();
    EXPECT_EQ(state.phase, PolygonBackgroundPhase::Normal);
    PolygonBackgroundContact enter(state);
    EXPECT_TRUE(enter.Test(PMSPolygonType::BackgroundTransition, 9));
    EXPECT_TRUE(enter.Test(PMSPolygonType::Background, 17));
    EXPECT_FALSE(enter.Test(PMSPolygonType::Normal, 2));
    enter.Finish();
    EXPECT_EQ(state.phase, PolygonBackgroundPhase::Transition);
    EXPECT_EQ(state.polygon, 17);
    const auto snapshot = state;
    PolygonBackgroundContact exit(state);
    exit.Finish();
    EXPECT_EQ(state.phase, PolygonBackgroundPhase::Normal);
    EXPECT_EQ(state.polygon, -1);
    state = snapshot;
    PolygonBackgroundContact restored(state);
    EXPECT_TRUE(restored.Test(PMSPolygonType::Background, 17));
    restored.Finish();
    EXPECT_EQ(state, snapshot);
}

TEST(PolygonBackgroundStateTest, SentinelsAndBoundsAreValidated)
{
    EXPECT_TRUE(IsValidBackgroundState({}, 0));
    EXPECT_TRUE(
      IsValidBackgroundState({ .phase = PolygonBackgroundPhase::Normal, .polygon = -1 }, 0));
    EXPECT_FALSE(
      IsValidBackgroundState({ .phase = PolygonBackgroundPhase::Normal, .polygon = -2 }, 0));
    EXPECT_FALSE(IsValidBackgroundState({ .phase = static_cast<PolygonBackgroundPhase>(2) }, 30));
    EXPECT_FALSE(IsValidBackgroundState({ .polygon = 30 }, 30));
    EXPECT_FALSE(IsValidBackgroundState({ .polygon = -3 }, 30));
    EXPECT_TRUE(IsValidBackgroundState({ .polygon = 29 }, 30));
}
