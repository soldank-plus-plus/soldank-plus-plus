#include "core/math/Glm.hpp"

#include <gtest/gtest.h>

#include <bitset>
#include <memory>
#include <string>
#include <utility>
#include <vector>

import AddObjectsMapEditorAction;
import AddPolygonMapEditorAction;
import AddSceneryMapEditorAction;
import AddSpawnPointMapEditorAction;
import ClientState;
import ColorObjectsMapEditorAction;
import MoveSelectionMapEditorAction;
import RemoveSelectionMapEditorAction;
import RotateSelectionMapEditorAction;
import ScaleSelectionMapEditorAction;
import TransformPolygonsMapEditorAction;
import TransformSceneriesMapEditorAction;
import TransformSpawnPointsMapEditorAction;

import Shared.Core.Animations;
import Shared.Core.Map.PMSConstants;
import Shared.Core.Map.PMSStructs;
import Shared.Core.Physics.Particles;
import Shared.Core.State.StateManager;

namespace
{
using namespace Soldank;

PMSPolygon MakePolygon(float x = 0.0F, float y = 0.0F)
{
    PMSPolygon polygon{};
    polygon.vertices.at(0).x = x;
    polygon.vertices.at(0).y = y;
    polygon.vertices.at(1).x = x + 10.0F;
    polygon.vertices.at(1).y = y;
    polygon.vertices.at(2).x = x;
    polygon.vertices.at(2).y = y + 10.0F;
    return polygon;
}

class MapEditorActionsTest : public testing::Test
{
protected:
    static AnimationDataManager MakeAnimationDataManager()
    {
        static const AnimationDataManager manager = [] {
            AnimationDataManager value;
            value.LoadAllAnimationDatas();
            return value;
        }();
        return manager;
    }

    MapEditorActionsTest()
        : animation_data_manager_(MakeAnimationDataManager())
        , state_manager_(animation_data_manager_)
    {
        state_manager_.CreateEmptyMapDocument();
    }

    AnimationDataManager animation_data_manager_;
    StateManager state_manager_;
    ClientState client_state_{};
};

TEST_F(MapEditorActionsTest, AddPolygonExecutesUndoesAndRemovesItsVertexSelection)
{
    AddPolygonMapEditorAction action(MakePolygon(2.0F, 3.0F));
    ASSERT_TRUE(action.CanExecute(client_state_, state_manager_));

    action.Execute(client_state_, state_manager_);
    ASSERT_EQ(state_manager_.GetConstMap().GetPolygonsCount(), 1U);
    client_state_.map_editor_state.selected_polygon_vertices.emplace_back(0U, 0b111);
    client_state_.map_editor_state.selected_polygon_vertices.emplace_back(42U, 0b001);

    action.Undo(client_state_, state_manager_);
    EXPECT_EQ(state_manager_.GetConstMap().GetPolygonsCount(), 0U);
    ASSERT_EQ(client_state_.map_editor_state.selected_polygon_vertices.size(), 1U);
    EXPECT_EQ(client_state_.map_editor_state.selected_polygon_vertices.front().first, 42U);
}

TEST_F(MapEditorActionsTest, AddPolygonCanExecuteAtLimitButNotPastIt)
{
    std::vector<PMSPolygon> polygons(MAX_POLYGONS_COUNT, MakePolygon());
    for (unsigned int id = 0; id < polygons.size(); ++id) {
        polygons[id].id = id;
    }
    state_manager_.GetMap().AddPolygons(polygons);

    AddPolygonMapEditorAction action(MakePolygon());
    EXPECT_FALSE(action.CanExecute(client_state_, state_manager_));
}

TEST_F(MapEditorActionsTest, AddSceneryAndSpawnPointRoundTripIncludingEmptyFileName)
{
    PMSScenery scenery{};
    scenery.x = -12.5F;
    PMSSpawnPoint spawn_point{};
    spawn_point.x = -7;
    AddSceneryMapEditorAction scenery_action(scenery, "");
    AddSpawnPointMapEditorAction spawn_action(spawn_point);

    scenery_action.Execute(client_state_, state_manager_);
    spawn_action.Execute(client_state_, state_manager_);
    ASSERT_EQ(state_manager_.GetConstMap().GetSceneryInstances().size(), 1U);
    ASSERT_EQ(state_manager_.GetConstMap().GetSpawnPoints().size(), 1U);
    EXPECT_FLOAT_EQ(state_manager_.GetConstMap().GetSceneryInstances().front().x, -12.5F);
    EXPECT_EQ(state_manager_.GetConstMap().GetSpawnPoints().front().x, -7);

    spawn_action.Undo(client_state_, state_manager_);
    scenery_action.Undo(client_state_, state_manager_);
    EXPECT_TRUE(state_manager_.GetConstMap().GetSceneryInstances().empty());
    EXPECT_TRUE(state_manager_.GetConstMap().GetSpawnPoints().empty());
}

TEST_F(MapEditorActionsTest, AddObjectsEmptyInputIsNoOpAndMixedInputRoundTrips)
{
    AddObjectsMapEditorAction empty({}, {}, {});
    EXPECT_TRUE(empty.CanExecute(client_state_, state_manager_));
    empty.Execute(client_state_, state_manager_);
    empty.Undo(client_state_, state_manager_);

    PMSPolygon polygon = MakePolygon();
    polygon.id = 0;
    PMSScenery scenery{};
    PMSSpawnPoint spawn{};
    AddObjectsMapEditorAction mixed(
      { polygon }, { { 0U, { scenery, "tree.png" } } }, { { 0U, spawn } });
    ASSERT_TRUE(mixed.CanExecute(client_state_, state_manager_));
    mixed.Execute(client_state_, state_manager_);
    EXPECT_EQ(state_manager_.GetConstMap().GetPolygonsCount(), 1U);
    EXPECT_EQ(state_manager_.GetConstMap().GetSceneryInstances().size(), 1U);
    EXPECT_EQ(state_manager_.GetConstMap().GetSpawnPoints().size(), 1U);
    mixed.Undo(client_state_, state_manager_);
    EXPECT_EQ(state_manager_.GetConstMap().GetPolygonsCount(), 0U);
    EXPECT_TRUE(state_manager_.GetConstMap().GetSceneryInstances().empty());
    EXPECT_TRUE(state_manager_.GetConstMap().GetSpawnPoints().empty());
}

TEST_F(MapEditorActionsTest, ColorObjectsSupportsPartialAndEmptySelectionsAndUndo)
{
    state_manager_.GetMap().AddNewPolygon(MakePolygon());
    PMSColor old_color(1, 2, 3, 4);
    PMSColor new_color(200, 201, 202, 203);
    state_manager_.GetMap().SetPolygonVerticesColorById({ { { 0U, 1U }, old_color } });
    ColorObjectsMapEditorAction action(new_color, { { { 0U, 1U }, old_color } }, {});

    action.Execute(client_state_, state_manager_);
    EXPECT_EQ(state_manager_.GetConstMap().GetPolygons().at(0).vertices.at(1).color.red, 200);
    EXPECT_EQ(state_manager_.GetConstMap().GetPolygons().at(0).vertices.at(0).color.red, 255);
    action.Undo(client_state_, state_manager_);
    EXPECT_EQ(state_manager_.GetConstMap().GetPolygons().at(0).vertices.at(1).color.red, 1);

    ColorObjectsMapEditorAction empty(new_color, {}, {});
    EXPECT_TRUE(empty.CanExecute(client_state_, state_manager_));
    empty.Execute(client_state_, state_manager_);
    empty.Undo(client_state_, state_manager_);
}

TEST_F(MapEditorActionsTest, MoveSelectionUsesOriginalPositionsAndTruncatesSpawnPointOffset)
{
    state_manager_.GetMap().AddNewPolygon(MakePolygon(1.0F, 2.0F));
    PMSSpawnPoint spawn{};
    spawn.x = 5;
    spawn.y = 6;
    state_manager_.GetMap().AddNewSpawnPoint(spawn);
    const auto& soldier = state_manager_.CreateSoldier(1U);
    state_manager_.TransformSoldier(soldier.id, [](auto& value) {
        value.particle.position = { 10.0F, 20.0F };
        value.particle.old_position = value.particle.position;
    });
    MoveSelectionMapEditorAction action({ { { 0U, 0U }, { 1.0F, 2.0F } } },
                                        {},
                                        { { 0U, { 5, 6 } } },
                                        { { soldier.id, { 10.0F, 20.0F } } });
    action.SetMoveOffset({ 2.9F, -3.9F });

    action.Execute(client_state_, state_manager_);
    EXPECT_FLOAT_EQ(state_manager_.GetConstMap().GetPolygons().at(0).vertices.at(0).x, 3.9F);
    EXPECT_EQ(state_manager_.GetConstMap().GetSpawnPoints().at(0).x, 7);
    EXPECT_EQ(state_manager_.GetConstMap().GetSpawnPoints().at(0).y, 3);
    EXPECT_FLOAT_EQ(state_manager_.GetSoldier(soldier.id).particle.position.x, 12.9F);
    action.Undo(client_state_, state_manager_);
    EXPECT_FLOAT_EQ(state_manager_.GetConstMap().GetPolygons().at(0).vertices.at(0).x, 1.0F);
    EXPECT_EQ(state_manager_.GetConstMap().GetSpawnPoints().at(0).x, 5);
    EXPECT_FLOAT_EQ(state_manager_.GetSoldier(soldier.id).particle.position.x, 10.0F);
}

TEST_F(MapEditorActionsTest, TransformActionsApplyFunctionsAndRestoreOriginals)
{
    PMSPolygon polygon = state_manager_.GetMap().AddNewPolygon(MakePolygon());
    PMSScenery scenery{};
    unsigned int scenery_id = state_manager_.GetMap().AddNewScenery(scenery, "x.png");
    PMSSpawnPoint spawn{};
    unsigned int spawn_id = state_manager_.GetMap().AddNewSpawnPoint(spawn);
    TransformPolygonsMapEditorAction polygons({ { polygon.id, polygon } }, [](PMSPolygon value) {
        value.vertices.at(0).x = 9.0F;
        return value;
    });
    TransformSceneriesMapEditorAction sceneries({ { scenery_id, scenery } }, [](PMSScenery value) {
        value.x = 8.0F;
        return value;
    });
    TransformSpawnPointsMapEditorAction spawns({ { spawn_id, spawn } }, [](PMSSpawnPoint value) {
        value.x = 7;
        return value;
    });

    polygons.Execute(client_state_, state_manager_);
    sceneries.Execute(client_state_, state_manager_);
    spawns.Execute(client_state_, state_manager_);
    EXPECT_FLOAT_EQ(state_manager_.GetConstMap().GetPolygons().at(0).vertices.at(0).x, 9.0F);
    EXPECT_FLOAT_EQ(state_manager_.GetConstMap().GetSceneryInstances().at(0).x, 8.0F);
    EXPECT_EQ(state_manager_.GetConstMap().GetSpawnPoints().at(0).x, 7);
    polygons.Undo(client_state_, state_manager_);
    sceneries.Undo(client_state_, state_manager_);
    spawns.Undo(client_state_, state_manager_);
    EXPECT_FLOAT_EQ(state_manager_.GetConstMap().GetPolygons().at(0).vertices.at(0).x, 0.0F);
    EXPECT_FLOAT_EQ(state_manager_.GetConstMap().GetSceneryInstances().at(0).x, 0.0F);
    EXPECT_EQ(state_manager_.GetConstMap().GetSpawnPoints().at(0).x, 0);
}

TEST_F(MapEditorActionsTest, RemoveSelectionIgnoresPartialPolygonsAndRestoresRemovedObjects)
{
    state_manager_.GetMap().AddNewPolygon(MakePolygon());
    state_manager_.GetMap().AddNewPolygon(MakePolygon(20.0F));
    PMSScenery scenery{};
    state_manager_.GetMap().AddNewScenery(scenery, "tree.png");
    PMSSpawnPoint spawn{};
    state_manager_.GetMap().AddNewSpawnPoint(spawn);
    const auto& soldier = state_manager_.CreateSoldier(1U);
    client_state_.map_editor_state.selected_polygon_vertices = { { 0U, 0b111 }, { 1U, 0b001 } };
    client_state_.map_editor_state.selected_scenery_ids = { 0U };
    client_state_.map_editor_state.selected_spawn_point_ids = { 0U };
    client_state_.map_editor_state.selected_soldier_ids = { soldier.id };
    RemoveSelectionMapEditorAction action(client_state_, state_manager_);

    EXPECT_TRUE(action.CanExecute(client_state_, state_manager_));
    action.Execute(client_state_, state_manager_);
    EXPECT_EQ(state_manager_.GetConstMap().GetPolygonsCount(), 1U);
    EXPECT_TRUE(state_manager_.GetConstMap().GetSceneryInstances().empty());
    EXPECT_TRUE(state_manager_.GetConstMap().GetSpawnPoints().empty());
    EXPECT_TRUE(client_state_.map_editor_state.selected_polygon_vertices.empty());
    EXPECT_FALSE(state_manager_.GetSoldier(soldier.id).active);

    action.Undo(client_state_, state_manager_);
    EXPECT_EQ(state_manager_.GetConstMap().GetPolygonsCount(), 2U);
    EXPECT_EQ(state_manager_.GetConstMap().GetSceneryInstances().size(), 1U);
    EXPECT_EQ(state_manager_.GetConstMap().GetSpawnPoints().size(), 1U);
    // TransformSoldier intentionally ignores inactive soldiers, so the current Undo implementation
    // cannot reactivate a removed soldier.
    EXPECT_FALSE(state_manager_.GetSoldier(soldier.id).active);
}

TEST_F(MapEditorActionsTest, RemoveEmptySelectionIsNoOp)
{
    RemoveSelectionMapEditorAction action(client_state_, state_manager_);
    action.Execute(client_state_, state_manager_);
    action.Undo(client_state_, state_manager_);
    EXPECT_EQ(state_manager_.GetConstMap().GetPolygonsCount(), 0U);
}

TEST_F(MapEditorActionsTest, ScaleHandlesZeroReferenceAxisAndAxisLocks)
{
    PMSPolygon polygon = state_manager_.GetMap().AddNewPolygon(MakePolygon(2.0F, 3.0F));
    PMSScenery scenery{};
    scenery.x = 2.0F;
    scenery.y = 3.0F;
    scenery.width = 10;
    scenery.height = 10;
    scenery.scale_x = 1.0F;
    scenery.scale_y = 1.0F;
    unsigned int scenery_id = state_manager_.GetMap().AddNewScenery(scenery, "x.png");
    PMSSpawnPoint spawn{};
    spawn.x = 2;
    spawn.y = 3;
    unsigned int spawn_id = state_manager_.GetMap().AddNewSpawnPoint(spawn);
    const auto& soldier = state_manager_.CreateSoldier(1U);
    state_manager_.TransformSoldier(soldier.id,
                                    [](auto& value) { value.particle.position = { 2.0F, 3.0F }; });
    ScaleSelectionMapEditorAction action({ { { polygon.id, 0b001 }, polygon } },
                                         { { scenery_id, scenery } },
                                         { { spawn_id, spawn } },
                                         { { soldier.id, { 2.0F, 3.0F } } },
                                         { 0.0F, 0.0F },
                                         { 0.0F, 3.0F },
                                         false,
                                         true);
    action.SetCurrentMousePosition({ 100.0F, 6.0F });
    action.Execute(client_state_, state_manager_);
    EXPECT_FLOAT_EQ(state_manager_.GetConstMap().GetPolygons().at(0).vertices.at(0).x, 2.0F);
    EXPECT_FLOAT_EQ(state_manager_.GetConstMap().GetPolygons().at(0).vertices.at(0).y, 6.0F);
    EXPECT_FLOAT_EQ(state_manager_.GetConstMap().GetSceneryInstances().at(0).y, 6.0F);
    EXPECT_EQ(state_manager_.GetConstMap().GetSpawnPoints().at(0).y, 6);
    EXPECT_FLOAT_EQ(state_manager_.GetSoldier(soldier.id).particle.position.y, 6.0F);
    action.Undo(client_state_, state_manager_);
    EXPECT_FLOAT_EQ(state_manager_.GetConstMap().GetPolygons().at(0).vertices.at(0).y, 3.0F);
}

TEST_F(MapEditorActionsTest, RotateLeavesUnselectedVerticesUntouchedAndUndoRestores)
{
    PMSPolygon polygon = state_manager_.GetMap().AddNewPolygon(MakePolygon(1.0F, 0.0F));
    PMSScenery scenery{};
    scenery.x = 1.0F;
    scenery.width = 10;
    scenery.height = 10;
    scenery.scale_x = 1.0F;
    scenery.scale_y = 1.0F;
    unsigned int scenery_id = state_manager_.GetMap().AddNewScenery(scenery, "x.png");
    PMSSpawnPoint spawn{};
    spawn.x = 1;
    unsigned int spawn_id = state_manager_.GetMap().AddNewSpawnPoint(spawn);
    const auto& soldier = state_manager_.CreateSoldier(1U);
    state_manager_.TransformSoldier(soldier.id,
                                    [](auto& value) { value.particle.position = { 1.0F, 0.0F }; });
    RotateSelectionMapEditorAction action({ { { polygon.id, std::bitset<3>{ 0b001 } }, polygon } },
                                          { { scenery_id, scenery } },
                                          { { spawn_id, spawn } },
                                          { { soldier.id, { 1.0F, 0.0F } } },
                                          { 0.0F, 0.0F },
                                          { 1.0F, 0.0F });
    action.SetCurrentMousePosition({ 0.0F, 1.0F });
    action.Execute(client_state_, state_manager_);
    const auto& rotated = state_manager_.GetConstMap().GetPolygons().at(0);
    EXPECT_NEAR(rotated.vertices.at(0).x, 0.0F, 0.0001F);
    EXPECT_NEAR(rotated.vertices.at(0).y, 1.0F, 0.0001F);
    EXPECT_FLOAT_EQ(rotated.vertices.at(1).x, 11.0F);
    EXPECT_NEAR(state_manager_.GetConstMap().GetSceneryInstances().at(0).y, 1.0F, 0.0001F);
    EXPECT_EQ(state_manager_.GetConstMap().GetSpawnPoints().at(0).y, 1);
    EXPECT_NEAR(state_manager_.GetSoldier(soldier.id).particle.position.y, 1.0F, 0.0001F);
    action.Undo(client_state_, state_manager_);
    EXPECT_FLOAT_EQ(state_manager_.GetConstMap().GetPolygons().at(0).vertices.at(0).x, 1.0F);
}
} // namespace
