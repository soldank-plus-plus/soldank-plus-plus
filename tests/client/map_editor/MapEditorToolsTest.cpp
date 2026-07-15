#include "core/math/Glm.hpp"

#include <gtest/gtest.h>

#include <array>
#include <memory>
#include <string>
#include <utility>
#include <vector>

import ClientState;
import ColorPickerTool;
import ColorTool;
import MapEditorAction;
import PolygonTool;
import SceneryTool;
import SelectionTool;
import SpawnpointTool;
import TextureTool;
import TransformTool;
import VertexColorTool;
import VertexSelectionTool;
import WaypointTool;

import Shared.Core.Animations;
import Shared.Core.Map.PMSEnums;
import Shared.Core.Map.PMSStructs;
import Shared.Core.Physics.Particles;
import Shared.Core.State.StateManager;

namespace
{
using namespace Soldank;

PMSPolygon MakePolygon()
{
    PMSPolygon polygon{};
    polygon.vertices.at(0) = { .x = 0.0F, .y = 0.0F, .color = PMSColor(10, 20, 30, 40) };
    polygon.vertices.at(1) = { .x = 10.0F, .y = 0.0F };
    polygon.vertices.at(2) = { .x = 0.0F, .y = 10.0F };
    return polygon;
}

class MapEditorToolsTest : public testing::Test
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

    MapEditorToolsTest()
        : animation_data_manager_(MakeAnimationDataManager())
        , state_manager_(animation_data_manager_)
    {
        state_manager_.CreateEmptyMapDocument();
        client_state_.world_render_options.current_polygon_texture_dimensions = { 100.0F, 100.0F };
    }

    void Capture(std::unique_ptr<MapEditorAction> action) { captured_action_ = std::move(action); }

    AnimationDataManager animation_data_manager_;
    StateManager state_manager_;
    ClientState client_state_{};
    std::unique_ptr<MapEditorAction> captured_action_;
};

template<typename TTool>
void ExerciseNoOpTool(TTool& tool, ClientState& client_state, const StateManager& state_manager)
{
    tool.OnSelect(client_state, state_manager);
    tool.OnSceneLeftMouseButtonClick(client_state, state_manager);
    tool.OnSceneLeftMouseButtonRelease(client_state, state_manager);
    tool.OnSceneRightMouseButtonClick(client_state);
    tool.OnSceneRightMouseButtonRelease();
    tool.OnMouseScreenPositionChange(client_state, {}, { 1.0F, 2.0F });
    tool.OnMouseMapPositionChange(client_state, {}, { 1.0F, 2.0F }, state_manager);
    tool.OnModifierKey1Pressed(client_state);
    tool.OnModifierKey1Released(client_state);
    tool.OnModifierKey2Pressed(client_state);
    tool.OnModifierKey2Released(client_state);
    tool.OnModifierKey3Pressed(client_state);
    tool.OnModifierKey3Released(client_state);
    tool.OnUnselect(client_state);
}

TEST_F(MapEditorToolsTest, PlaceholderToolsAcceptEveryEventWithoutChangingEditorState)
{
    TextureTool texture_tool;
    VertexColorTool vertex_color_tool;
    WaypointTool waypoint_tool;
    ExerciseNoOpTool(texture_tool, client_state_, state_manager_);
    ExerciseNoOpTool(vertex_color_tool, client_state_, state_manager_);
    ExerciseNoOpTool(waypoint_tool, client_state_, state_manager_);
    EXPECT_TRUE(client_state_.map_editor_state.current_tool_action_description.empty());
}

TEST_F(MapEditorToolsTest, PolygonToolBuildsThreeVerticesThenEmitsAction)
{
    PolygonTool tool([this](auto action) { Capture(std::move(action)); });
    tool.OnSelect(client_state_, state_manager_);
    EXPECT_EQ(client_state_.map_editor_state.current_tool_action_description, "Create Polygon");

    tool.OnMouseMapPositionChange(client_state_, {}, { 1.0F, 2.0F }, state_manager_);
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    ASSERT_TRUE(client_state_.map_editor_state.polygon_tool_wip_polygon_edge.has_value());
    tool.OnMouseMapPositionChange(client_state_, {}, { 11.0F, 2.0F }, state_manager_);
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    ASSERT_TRUE(client_state_.map_editor_state.polygon_tool_wip_polygon.has_value());
    tool.OnMouseMapPositionChange(client_state_, {}, { 1.0F, 12.0F }, state_manager_);
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    ASSERT_NE(captured_action_, nullptr);
    captured_action_->Execute(client_state_, state_manager_);
    ASSERT_EQ(state_manager_.GetConstMap().GetPolygonsCount(), 1U);
    EXPECT_FLOAT_EQ(state_manager_.GetConstMap().GetPolygons().at(0).vertices.at(2).y, 12.0F);
}

TEST_F(MapEditorToolsTest, PolygonToolSnapsToGridIncludingNegativeCoordinatesAndCancelsOnUnselect)
{
    PolygonTool tool([this](auto action) { Capture(std::move(action)); });
    client_state_.map_editor_state.is_snap_to_grid_enabled = true;
    client_state_.map_editor_state.grid_interval_division = 4;
    tool.OnMouseMapPositionChange(client_state_, {}, { -5.9F, 5.9F }, state_manager_);
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    ASSERT_TRUE(client_state_.map_editor_state.polygon_tool_wip_polygon_edge);
    EXPECT_FLOAT_EQ(client_state_.map_editor_state.polygon_tool_wip_polygon_edge->vertices.at(0).x,
                    -4.0F);
    EXPECT_FLOAT_EQ(client_state_.map_editor_state.polygon_tool_wip_polygon_edge->vertices.at(0).y,
                    4.0F);
    tool.OnUnselect(client_state_);
    EXPECT_FALSE(client_state_.map_editor_state.polygon_tool_wip_polygon_edge);
    EXPECT_FALSE(client_state_.map_editor_state.polygon_tool_wip_polygon);
}

TEST_F(MapEditorToolsTest, PolygonToolPrefersTheClosestVertexOverGridSnapping)
{
    state_manager_.GetMap().AddNewPolygon(MakePolygon());
    PolygonTool tool([this](auto action) { Capture(std::move(action)); });
    client_state_.map_editor_state.is_snap_to_grid_enabled = true;
    client_state_.map_editor_state.is_snap_to_vertices_enabled = true;
    client_state_.map_editor_state.grid_interval_division = 4;
    tool.OnMouseMapPositionChange(client_state_, {}, { 9.5F, 0.5F }, state_manager_);
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    ASSERT_TRUE(client_state_.map_editor_state.polygon_tool_wip_polygon_edge);
    EXPECT_FLOAT_EQ(client_state_.map_editor_state.polygon_tool_wip_polygon_edge->vertices.at(0).x,
                    10.0F);
    EXPECT_FLOAT_EQ(client_state_.map_editor_state.polygon_tool_wip_polygon_edge->vertices.at(0).y,
                    0.0F);
}

TEST_F(MapEditorToolsTest, SpawnpointToolPreviewsSnapsTruncatesAndOpensPicker)
{
    SpawnpointTool tool([this](auto action) { Capture(std::move(action)); });
    client_state_.input.mouse_map_position = { 3.5F, 4.5F };
    tool.OnSelect(client_state_, state_manager_);
    EXPECT_EQ(client_state_.map_editor_state.spawn_point_preview_position, glm::vec2(3.5F, 4.5F));
    tool.OnMouseMapPositionChange(client_state_, {}, { -7.9F, 9.9F }, state_manager_);
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    ASSERT_NE(captured_action_, nullptr);
    captured_action_->Execute(client_state_, state_manager_);
    EXPECT_EQ(state_manager_.GetConstMap().GetSpawnPoints().at(0).x, -7);
    EXPECT_EQ(state_manager_.GetConstMap().GetSpawnPoints().at(0).y, 9);
    tool.OnSceneRightMouseButtonClick(client_state_);
    EXPECT_TRUE(client_state_.map_editor_state.should_open_spawn_point_type_popup);
}

TEST_F(MapEditorToolsTest, SceneryToolInitializesPreviewAppliesPaletteAndSnaps)
{
    SceneryTool tool([this](auto action) { Capture(std::move(action)); });
    client_state_.map_editor_state.selected_scenery_to_place = "crate.png";
    client_state_.map_editor_state.palette_current_color = { 0.0F, 0.5F, 1.0F, 0.25F };
    tool.OnSelect(client_state_, state_manager_);
    EXPECT_TRUE(client_state_.map_editor_state.scenery_to_place.active);
    client_state_.map_editor_state.is_snap_to_grid_enabled = true;
    client_state_.map_editor_state.grid_interval_division = 4;
    tool.OnMouseMapPositionChange(client_state_, {}, { 5.9F, -5.9F }, state_manager_);
    EXPECT_FLOAT_EQ(client_state_.map_editor_state.scenery_to_place.x, 4.0F);
    EXPECT_FLOAT_EQ(client_state_.map_editor_state.scenery_to_place.y, -4.0F);
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    captured_action_->Execute(client_state_, state_manager_);
    const auto& scenery = state_manager_.GetConstMap().GetSceneryInstances().at(0);
    EXPECT_EQ(scenery.color.red, 0);
    EXPECT_EQ(scenery.color.green, 127);
    EXPECT_EQ(scenery.color.blue, 255);
    EXPECT_EQ(scenery.alpha, 63);
    tool.OnSceneRightMouseButtonClick(client_state_);
    EXPECT_TRUE(client_state_.map_editor_state.should_open_scenery_picker_popup);
}

TEST_F(MapEditorToolsTest, VertexSelectionAcceptsReversedBoundsAndClearsPreviousSelections)
{
    state_manager_.GetMap().AddNewPolygon(MakePolygon());
    PMSScenery scenery{};
    scenery.x = 5.0F;
    scenery.y = 5.0F;
    state_manager_.GetMap().AddNewScenery(scenery, "x.png");
    PMSSpawnPoint spawn{};
    spawn.x = 5;
    spawn.y = 5;
    state_manager_.GetMap().AddNewSpawnPoint(spawn);
    const auto& soldier = state_manager_.CreateSoldier(1U);
    state_manager_.TransformSoldier(soldier.id,
                                    [](auto& value) { value.particle.position = { 5.0F, 5.0F }; });
    client_state_.map_editor_state.selected_spawn_point_ids = { 99U };
    VertexSelectionTool tool;
    tool.OnMouseMapPositionChange(client_state_, {}, { 11.0F, 11.0F }, state_manager_);
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    tool.OnMouseMapPositionChange(client_state_, {}, { -1.0F, -1.0F }, state_manager_);
    tool.OnSceneLeftMouseButtonRelease(client_state_, state_manager_);

    ASSERT_EQ(client_state_.map_editor_state.selected_polygon_vertices.size(), 1U);
    EXPECT_TRUE(client_state_.map_editor_state.selected_polygon_vertices.at(0).second.all());
    EXPECT_EQ(client_state_.map_editor_state.selected_scenery_ids, std::vector<unsigned int>{ 0U });
    EXPECT_EQ(client_state_.map_editor_state.selected_spawn_point_ids,
              std::vector<unsigned int>{ 0U });
    EXPECT_EQ(client_state_.map_editor_state.selected_soldier_ids,
              std::vector<unsigned int>{ soldier.id });
    EXPECT_FALSE(client_state_.map_editor_state.vertex_selection_box);
}

TEST_F(MapEditorToolsTest, VertexSelectionIncludesPointsOnZeroAreaBoundary)
{
    state_manager_.GetMap().AddNewPolygon(MakePolygon());
    VertexSelectionTool tool;
    tool.OnMouseMapPositionChange(client_state_, {}, { 0.0F, 0.0F }, state_manager_);
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    tool.OnSceneLeftMouseButtonRelease(client_state_, state_manager_);
    ASSERT_EQ(client_state_.map_editor_state.selected_polygon_vertices.size(), 1U);
    EXPECT_EQ(client_state_.map_editor_state.selected_polygon_vertices.at(0).second.to_ulong(), 1U);
}

TEST_F(MapEditorToolsTest, ColorPickerPrefersTopmostPolygonVertexAndSupportsPixelMode)
{
    PMSPolygon bottom = MakePolygon();
    PMSPolygon top = MakePolygon();
    top.vertices.at(0).color = PMSColor(100, 110, 120, 130);
    state_manager_.GetMap().AddNewPolygon(bottom);
    state_manager_.GetMap().AddNewPolygon(top);
    client_state_.input.mouse_map_position = { 1.0F, 1.0F };
    ColorPickerTool tool;
    tool.OnSelect(client_state_, state_manager_);
    tool.OnSceneLeftMouseButtonRelease(client_state_, state_manager_);
    EXPECT_FLOAT_EQ(client_state_.map_editor_state.palette_current_color.at(0), 100.0F / 255.0F);

    client_state_.map_editor_state.last_requested_pixel_color = { 0.1F, 0.2F, 0.3F, 0.4F };
    tool.OnModifierKey1Pressed(client_state_);
    tool.OnSceneLeftMouseButtonRelease(client_state_, state_manager_);
    EXPECT_EQ(client_state_.map_editor_state.palette_current_color,
              (std::array<float, 4>{ 0.1F, 0.2F, 0.3F, 0.4F }));
    tool.OnModifierKey1Released(client_state_);
    EXPECT_EQ(client_state_.map_editor_state.current_tool_action_description,
              "Pick a vertex or scenery color");
}

TEST_F(MapEditorToolsTest, ColorPickerFallsBackToSceneryAndLeavesColorWhenNothingIsHit)
{
    PMSScenery scenery{};
    scenery.width = 10;
    scenery.height = 10;
    scenery.scale_x = 1.0F;
    scenery.scale_y = 1.0F;
    scenery.active = true;
    scenery.color = PMSColor(20, 40, 60, 80);
    state_manager_.GetMap().AddNewScenery(scenery, "x.png");
    ColorPickerTool tool;
    client_state_.input.mouse_map_position = { 2.0F, 2.0F };
    tool.OnSceneLeftMouseButtonRelease(client_state_, state_manager_);
    EXPECT_FLOAT_EQ(client_state_.map_editor_state.palette_current_color.at(0), 20.0F / 255.0F);
    auto picked_color = client_state_.map_editor_state.palette_current_color;
    client_state_.input.mouse_map_position = { 100.0F, 100.0F };
    tool.OnSceneLeftMouseButtonRelease(client_state_, state_manager_);
    EXPECT_EQ(client_state_.map_editor_state.palette_current_color, picked_color);
    ExerciseNoOpTool(tool, client_state_, state_manager_);
}

TEST_F(MapEditorToolsTest, ColorToolDoesNothingOffObjectsAndColorsSelectedVertices)
{
    state_manager_.GetMap().AddNewPolygon(MakePolygon());
    ColorTool tool([this](auto action) { Capture(std::move(action)); });
    tool.OnSelect(client_state_, state_manager_);
    tool.OnMouseMapPositionChange(client_state_, {}, { 50.0F, 50.0F }, state_manager_);
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    EXPECT_EQ(captured_action_, nullptr);

    client_state_.map_editor_state.selected_polygon_vertices = { { 0U, 0b101 } };
    client_state_.map_editor_state.palette_current_color = { 1.0F, 0.0F, 0.5F, 1.0F };
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    ASSERT_NE(captured_action_, nullptr);
    captured_action_->Execute(client_state_, state_manager_);
    const auto& polygon = state_manager_.GetConstMap().GetPolygons().at(0);
    EXPECT_EQ(polygon.vertices.at(0).color.red, 255);
    EXPECT_EQ(polygon.vertices.at(0).color.blue, 127);
    EXPECT_EQ(polygon.vertices.at(1).color.blue, 255);
    ExerciseNoOpTool(tool, client_state_, state_manager_);
}

TEST_F(MapEditorToolsTest, ColorToolColorsAllObjectsUnderCursorWhenNothingIsSelected)
{
    state_manager_.GetMap().AddNewPolygon(MakePolygon());
    PMSScenery scenery{};
    scenery.width = 10;
    scenery.height = 10;
    scenery.active = true;
    state_manager_.GetMap().AddNewScenery(scenery, "x.png");
    ColorTool tool([this](auto action) { Capture(std::move(action)); });
    tool.OnMouseMapPositionChange(client_state_, {}, { 1.0F, 1.0F }, state_manager_);
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    ASSERT_NE(captured_action_, nullptr);
    captured_action_->Execute(client_state_, state_manager_);
    EXPECT_EQ(state_manager_.GetConstMap().GetSceneryInstances().at(0).color.red, 255);
}

TEST_F(MapEditorToolsTest, SelectionToolSelectsAddsRemovesAndClearsOnEmptySpace)
{
    state_manager_.GetMap().AddNewPolygon(MakePolygon());
    PMSSpawnPoint spawn{};
    spawn.x = 30;
    spawn.y = 30;
    state_manager_.GetMap().AddNewSpawnPoint(spawn);
    SelectionTool tool;
    tool.OnSelect(client_state_, state_manager_);
    tool.OnMouseMapPositionChange(client_state_, {}, { 1.0F, 1.0F }, state_manager_);
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    ASSERT_EQ(client_state_.map_editor_state.selected_polygon_vertices.size(), 1U);

    tool.OnModifierKey1Pressed(client_state_);
    tool.OnMouseMapPositionChange(client_state_, {}, { 30.0F, 30.0F }, state_manager_);
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    EXPECT_EQ(client_state_.map_editor_state.selected_spawn_point_ids,
              std::vector<unsigned int>{ 0U });
    tool.OnModifierKey1Released(client_state_);
    client_state_.map_editor_state.selected_polygon_vertices.clear();
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);

    tool.OnModifierKey3Pressed(client_state_);
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    EXPECT_TRUE(client_state_.map_editor_state.selected_spawn_point_ids.empty());
    tool.OnModifierKey3Released(client_state_);
    tool.OnMouseMapPositionChange(client_state_, {}, { 100.0F, 100.0F }, state_manager_);
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    EXPECT_TRUE(client_state_.map_editor_state.selected_polygon_vertices.empty());
}

TEST_F(MapEditorToolsTest, SelectionToolPromotesPartialPolygonAndCyclesOverlappingObjects)
{
    state_manager_.GetMap().AddNewPolygon(MakePolygon());
    state_manager_.GetMap().AddNewPolygon(MakePolygon());
    client_state_.map_editor_state.selected_polygon_vertices = { { 0U, 0b001 } };
    SelectionTool tool;
    tool.OnSelect(client_state_, state_manager_);
    EXPECT_TRUE(client_state_.map_editor_state.selected_polygon_vertices.at(0).second.all());
    tool.OnMouseMapPositionChange(client_state_, {}, { 1.0F, 1.0F }, state_manager_);
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    ASSERT_EQ(client_state_.map_editor_state.selected_polygon_vertices.size(), 1U);
    EXPECT_EQ(client_state_.map_editor_state.selected_polygon_vertices.at(0).first, 1U);
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    EXPECT_EQ(client_state_.map_editor_state.selected_polygon_vertices.at(0).first, 0U);
}

TEST_F(MapEditorToolsTest, SelectionToolCyclesOverlappingSpawnPoints)
{
    PMSSpawnPoint spawn_point{};
    spawn_point.x = 10;
    spawn_point.y = 10;
    state_manager_.GetMap().AddNewSpawnPoint(spawn_point);
    state_manager_.GetMap().AddNewSpawnPoint(spawn_point);
    SelectionTool tool;
    tool.OnSelect(client_state_, state_manager_);
    tool.OnMouseMapPositionChange(client_state_, {}, { 10.0F, 10.0F }, state_manager_);

    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    EXPECT_EQ(client_state_.map_editor_state.selected_spawn_point_ids,
              std::vector<unsigned int>{ 0U });
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    EXPECT_EQ(client_state_.map_editor_state.selected_spawn_point_ids,
              std::vector<unsigned int>{ 1U });
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    EXPECT_EQ(client_state_.map_editor_state.selected_spawn_point_ids,
              std::vector<unsigned int>{ 0U });
}

TEST_F(MapEditorToolsTest, SelectionToolCyclesOverlappingSoldiers)
{
    PMSPolygon polygon = MakePolygon();
    for (auto& vertex : polygon.vertices) {
        vertex.x += 100.0F;
        vertex.y += 100.0F;
    }
    state_manager_.GetMap().AddNewPolygon(polygon);
    const auto first_soldier_id = state_manager_.CreateSoldier(1U).id;
    const auto second_soldier_id = state_manager_.CreateSoldier(2U).id;
    state_manager_.TransformSoldier(
      first_soldier_id, [](auto& soldier) { soldier.particle.position = { 10.0F, 10.0F }; });
    state_manager_.TransformSoldier(
      second_soldier_id, [](auto& soldier) { soldier.particle.position = { 10.0F, 10.0F }; });
    SelectionTool tool;
    tool.OnSelect(client_state_, state_manager_);
    tool.OnMouseMapPositionChange(client_state_, {}, { 10.0F, 10.0F }, state_manager_);

    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    EXPECT_EQ(client_state_.map_editor_state.selected_soldier_ids,
              std::vector<unsigned int>{ first_soldier_id });
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    EXPECT_EQ(client_state_.map_editor_state.selected_soldier_ids,
              std::vector<unsigned int>{ second_soldier_id });
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    EXPECT_EQ(client_state_.map_editor_state.selected_soldier_ids,
              std::vector<unsigned int>{ first_soldier_id });
}

TEST_F(MapEditorToolsTest, SelectionToolCyclesEveryOverlappingObjectType)
{
    state_manager_.GetMap().AddNewPolygon(MakePolygon());
    PMSScenery scenery{};
    scenery.width = 10;
    scenery.height = 10;
    scenery.scale_x = 1.0F;
    scenery.scale_y = 1.0F;
    scenery.active = true;
    state_manager_.GetMap().AddNewScenery(scenery, "x.png");
    PMSSpawnPoint spawn_point{};
    spawn_point.x = 1;
    spawn_point.y = 1;
    state_manager_.GetMap().AddNewSpawnPoint(spawn_point);
    const auto soldier_id = state_manager_.CreateSoldier(1U).id;
    state_manager_.TransformSoldier(
      soldier_id, [](auto& soldier) { soldier.particle.position = { 1.0F, 1.0F }; });
    SelectionTool tool;
    tool.OnSelect(client_state_, state_manager_);
    tool.OnMouseMapPositionChange(client_state_, {}, { 1.0F, 1.0F }, state_manager_);

    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    ASSERT_EQ(client_state_.map_editor_state.selected_polygon_vertices.size(), 1U);
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    EXPECT_EQ(client_state_.map_editor_state.selected_scenery_ids, std::vector<unsigned int>{ 0U });
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    EXPECT_EQ(client_state_.map_editor_state.selected_spawn_point_ids,
              std::vector<unsigned int>{ 0U });
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    EXPECT_EQ(client_state_.map_editor_state.selected_soldier_ids,
              std::vector<unsigned int>{ soldier_id });
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    ASSERT_EQ(client_state_.map_editor_state.selected_polygon_vertices.size(), 1U);
    EXPECT_EQ(client_state_.map_editor_state.selected_polygon_vertices.front().first, 0U);
}

TEST_F(MapEditorToolsTest, SelectionToolSkipsMissingObjectTypesWhileCycling)
{
    state_manager_.GetMap().AddNewPolygon(MakePolygon());
    PMSSpawnPoint spawn_point{};
    spawn_point.x = 1;
    spawn_point.y = 1;
    state_manager_.GetMap().AddNewSpawnPoint(spawn_point);
    SelectionTool tool;
    tool.OnSelect(client_state_, state_manager_);
    tool.OnMouseMapPositionChange(client_state_, {}, { 1.0F, 1.0F }, state_manager_);

    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    ASSERT_EQ(client_state_.map_editor_state.selected_polygon_vertices.size(), 1U);
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    EXPECT_EQ(client_state_.map_editor_state.selected_spawn_point_ids,
              std::vector<unsigned int>{ 0U });
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    ASSERT_EQ(client_state_.map_editor_state.selected_polygon_vertices.size(), 1U);
    EXPECT_EQ(client_state_.map_editor_state.selected_polygon_vertices.front().first, 0U);
}

TEST_F(MapEditorToolsTest, SelectionToolSelectsAndRemovesSceneryAndPolygon)
{
    PMSScenery scenery{};
    scenery.width = 10;
    scenery.height = 10;
    scenery.scale_x = 1.0F;
    scenery.scale_y = 1.0F;
    scenery.active = true;
    state_manager_.GetMap().AddNewScenery(scenery, "x.png");
    state_manager_.GetMap().AddNewPolygon(MakePolygon());
    SelectionTool tool;
    tool.OnSelect(client_state_, state_manager_);
    tool.OnMouseMapPositionChange(client_state_, {}, { 1.0F, 1.0F }, state_manager_);
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    ASSERT_EQ(client_state_.map_editor_state.selected_polygon_vertices.size(), 1U);
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    ASSERT_EQ(client_state_.map_editor_state.selected_scenery_ids.size(), 1U);
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    tool.OnModifierKey1Pressed(client_state_);
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    EXPECT_EQ(client_state_.map_editor_state.selected_polygon_vertices.size(), 1U);
    client_state_.map_editor_state.selected_polygon_vertices = { { 0U, 0b111 } };
    client_state_.map_editor_state.selected_scenery_ids = { 0U };
    tool.OnModifierKey3Pressed(client_state_);
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    tool.OnUnselect(client_state_);
}

TEST_F(MapEditorToolsTest, SelectionToolSelectsAddsAndRemovesSoldiers)
{
    state_manager_.GetMap().AddNewPolygon([] {
        PMSPolygon polygon = MakePolygon();
        for (auto& vertex : polygon.vertices) {
            vertex.x += 100.0F;
            vertex.y += 100.0F;
        }
        return polygon;
    }());
    const auto& soldier = state_manager_.CreateSoldier(1U);
    state_manager_.TransformSoldier(
      soldier.id, [](auto& value) { value.particle.position = { 10.0F, 10.0F }; });
    SelectionTool tool;
    tool.OnSelect(client_state_, state_manager_);
    tool.OnMouseMapPositionChange(client_state_, {}, { 10.0F, 10.0F }, state_manager_);
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    ASSERT_EQ(client_state_.map_editor_state.selected_soldier_ids,
              std::vector<unsigned int>{ soldier.id });
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    tool.OnModifierKey1Pressed(client_state_);
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    EXPECT_EQ(client_state_.map_editor_state.selected_soldier_ids.size(), 1U);
    tool.OnModifierKey3Pressed(client_state_);
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    EXPECT_TRUE(client_state_.map_editor_state.selected_soldier_ids.empty());
}

TEST_F(MapEditorToolsTest, TransformToolMovesSelectionAndCommitsAction)
{
    state_manager_.GetMap().AddNewPolygon(MakePolygon());
    PMSScenery scenery{};
    scenery.x = 20.0F;
    scenery.y = 20.0F;
    scenery.width = 10;
    scenery.height = 10;
    scenery.scale_x = 1.0F;
    scenery.scale_y = 1.0F;
    scenery.active = true;
    state_manager_.GetMap().AddNewScenery(scenery, "x.png");
    PMSSpawnPoint spawn{};
    spawn.x = 40;
    spawn.y = 40;
    state_manager_.GetMap().AddNewSpawnPoint(spawn);
    const auto& soldier = state_manager_.CreateSoldier(1U);
    state_manager_.TransformSoldier(
      soldier.id, [](auto& value) { value.particle.position = { 60.0F, 60.0F }; });
    client_state_.map_editor_state.selected_polygon_vertices = { { 0U, 0b111 } };
    client_state_.map_editor_state.selected_scenery_ids = { 0U };
    client_state_.map_editor_state.selected_spawn_point_ids = { 0U };
    client_state_.map_editor_state.selected_soldier_ids = { soldier.id };
    unsigned int previews = 0;
    TransformTool tool([this](auto action) { Capture(std::move(action)); },
                       [this, &previews](MapEditorAction* action) {
                           ++previews;
                           action->Execute(client_state_, state_manager_);
                       });
    tool.OnSelect(client_state_, state_manager_);
    ASSERT_TRUE(client_state_.map_editor_state.vertex_selection_box);
    client_state_.input.mouse_map_position = { 2.0F, 2.0F };
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    tool.OnMouseMapPositionChange(client_state_, {}, { 7.0F, 9.0F }, state_manager_);
    EXPECT_EQ(previews, 1U);
    EXPECT_FLOAT_EQ(state_manager_.GetConstMap().GetPolygons().at(0).vertices.at(0).x, 5.0F);
    EXPECT_FLOAT_EQ(state_manager_.GetConstMap().GetSceneryInstances().at(0).x, 25.0F);
    EXPECT_EQ(state_manager_.GetConstMap().GetSpawnPoints().at(0).x, 45);
    EXPECT_FLOAT_EQ(state_manager_.GetSoldier(soldier.id).particle.position.x, 65.0F);
    tool.OnSceneLeftMouseButtonRelease(client_state_, state_manager_);
    EXPECT_NE(captured_action_, nullptr);
    tool.OnUnselect(client_state_);
    EXPECT_FALSE(client_state_.map_editor_state.vertex_selection_box);
}

TEST_F(MapEditorToolsTest, SelectionAndTransformToolsHandleAnEmptyMap)
{
    SelectionTool selection;
    selection.OnSelect(client_state_, state_manager_);
    selection.OnMouseMapPositionChange(client_state_, {}, { 1.0F, 1.0F }, state_manager_);
    selection.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    EXPECT_TRUE(client_state_.map_editor_state.selected_polygon_vertices.empty());
    selection.OnSceneLeftMouseButtonRelease(client_state_, state_manager_);
    selection.OnSceneRightMouseButtonClick(client_state_);
    selection.OnSceneRightMouseButtonRelease();
    selection.OnMouseScreenPositionChange(client_state_, {}, {});
    selection.OnModifierKey2Pressed(client_state_);
    selection.OnModifierKey2Released(client_state_);
    selection.OnUnselect(client_state_);

    unsigned int committed_actions = 0;
    TransformTool transform([&committed_actions](auto /*action*/) { ++committed_actions; },
                            [](MapEditorAction* /*action*/) {});
    transform.OnSelect(client_state_, state_manager_);
    EXPECT_FALSE(client_state_.map_editor_state.vertex_selection_box);
    transform.OnModifierKey2Pressed(client_state_);
    transform.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    transform.OnMouseMapPositionChange(client_state_, {}, { 2.0F, 2.0F }, state_manager_);
    transform.OnSceneLeftMouseButtonRelease(client_state_, state_manager_);
    transform.OnModifierKey3Pressed(client_state_);
    transform.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    transform.OnSceneLeftMouseButtonRelease(client_state_, state_manager_);
    EXPECT_EQ(committed_actions, 0U);
}

TEST_F(MapEditorToolsTest, TransformToolScaleAndRotateRequireASelectionHandle)
{
    state_manager_.GetMap().AddNewPolygon(MakePolygon());
    PMSScenery scenery{};
    scenery.x = 20.0F;
    scenery.y = 20.0F;
    scenery.width = 10;
    scenery.height = 10;
    scenery.scale_x = 1.0F;
    scenery.scale_y = 1.0F;
    scenery.active = true;
    state_manager_.GetMap().AddNewScenery(scenery, "x.png");
    PMSSpawnPoint spawn{};
    spawn.x = 40;
    spawn.y = 40;
    state_manager_.GetMap().AddNewSpawnPoint(spawn);
    const auto& soldier = state_manager_.CreateSoldier(1U);
    state_manager_.TransformSoldier(
      soldier.id, [](auto& value) { value.particle.position = { 60.0F, 60.0F }; });
    client_state_.map_editor_state.selected_polygon_vertices = { { 0U, 0b111 } };
    client_state_.map_editor_state.selected_scenery_ids = { 0U };
    client_state_.map_editor_state.selected_spawn_point_ids = { 0U };
    client_state_.map_editor_state.selected_soldier_ids = { soldier.id };
    std::vector<std::unique_ptr<MapEditorAction>> actions;
    TransformTool tool(
      [&actions](auto action) { actions.push_back(std::move(action)); },
      [this](MapEditorAction* action) { action->Execute(client_state_, state_manager_); });
    tool.OnSelect(client_state_, state_manager_);
    client_state_.input.mouse_map_position = { 0.0F, 0.0F };
    tool.OnModifierKey2Pressed(client_state_);
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    tool.OnMouseMapPositionChange(client_state_, {}, { -5.0F, -5.0F }, state_manager_);
    tool.OnSceneLeftMouseButtonRelease(client_state_, state_manager_);
    ASSERT_EQ(actions.size(), 1U);

    tool.OnModifierKey2Released(client_state_);
    tool.OnModifierKey3Pressed(client_state_);
    client_state_.input.mouse_map_position =
      client_state_.map_editor_state.vertex_selection_box->first;
    tool.OnSceneLeftMouseButtonClick(client_state_, state_manager_);
    tool.OnMouseMapPositionChange(client_state_, {}, { 5.0F, 5.0F }, state_manager_);
    tool.OnSceneLeftMouseButtonRelease(client_state_, state_manager_);
    EXPECT_EQ(actions.size(), 2U);
    tool.OnModifierKey3Released(client_state_);
    ExerciseNoOpTool(tool, client_state_, state_manager_);
}

TEST_F(MapEditorToolsTest, ImplementedToolsAcceptTheirInertCallbacks)
{
    PolygonTool polygon([this](auto action) { Capture(std::move(action)); });
    SceneryTool scenery([this](auto action) { Capture(std::move(action)); });
    SpawnpointTool spawn([this](auto action) { Capture(std::move(action)); });
    VertexSelectionTool vertex_selection;

    polygon.OnSceneLeftMouseButtonRelease(client_state_, state_manager_);
    polygon.OnSceneRightMouseButtonRelease();
    polygon.OnMouseScreenPositionChange(client_state_, {}, {});
    polygon.OnModifierKey1Pressed(client_state_);
    polygon.OnModifierKey1Released(client_state_);
    polygon.OnModifierKey2Pressed(client_state_);
    polygon.OnModifierKey2Released(client_state_);
    polygon.OnModifierKey3Pressed(client_state_);
    polygon.OnModifierKey3Released(client_state_);
    polygon.OnSceneRightMouseButtonClick(client_state_);
    EXPECT_TRUE(client_state_.map_editor_state.should_open_polygon_type_popup);

    scenery.OnSceneLeftMouseButtonRelease(client_state_, state_manager_);
    scenery.OnSceneRightMouseButtonRelease();
    scenery.OnMouseScreenPositionChange(client_state_, {}, {});
    scenery.OnModifierKey1Pressed(client_state_);
    scenery.OnModifierKey1Released(client_state_);
    scenery.OnModifierKey2Pressed(client_state_);
    scenery.OnModifierKey2Released(client_state_);
    scenery.OnModifierKey3Pressed(client_state_);
    scenery.OnModifierKey3Released(client_state_);
    scenery.OnUnselect(client_state_);

    spawn.OnSceneLeftMouseButtonRelease(client_state_, state_manager_);
    spawn.OnSceneRightMouseButtonRelease();
    spawn.OnMouseScreenPositionChange(client_state_, {}, {});
    spawn.OnModifierKey1Pressed(client_state_);
    spawn.OnModifierKey1Released(client_state_);
    spawn.OnModifierKey2Pressed(client_state_);
    spawn.OnModifierKey2Released(client_state_);
    spawn.OnModifierKey3Pressed(client_state_);
    spawn.OnModifierKey3Released(client_state_);
    spawn.OnUnselect(client_state_);

    vertex_selection.OnSelect(client_state_, state_manager_);
    vertex_selection.OnSceneRightMouseButtonClick(client_state_);
    vertex_selection.OnSceneRightMouseButtonRelease();
    vertex_selection.OnMouseScreenPositionChange(client_state_, {}, {});
    vertex_selection.OnModifierKey1Pressed(client_state_);
    vertex_selection.OnModifierKey1Released(client_state_);
    vertex_selection.OnModifierKey2Pressed(client_state_);
    vertex_selection.OnModifierKey2Released(client_state_);
    vertex_selection.OnModifierKey3Pressed(client_state_);
    vertex_selection.OnModifierKey3Released(client_state_);
    vertex_selection.OnUnselect(client_state_);
}
} // namespace
