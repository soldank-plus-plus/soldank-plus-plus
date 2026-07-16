#include "core/math/Glm.hpp"

#include <GLFW/glfw3.h>
#include <gtest/gtest.h>

#include <array>
#include <bitset>
#include <filesystem>
#include <fstream>
#include <memory>
#include <span>
#include <string>
#include <variant>
#include <vector>

import ClientState;
import MapEditor;
import MapEditorAction;
import MapEditorState;
import MapEditor.Config;
import MapEditor.EditorAssetBrowser;
import MapEditor.EditorCommandHistory;
import MapEditor.EditorDocument;
import MapEditor.EditorEventRouter;
import MapEditor.EditorMapProperties;
import MapEditor.EditorShortcutController;
import MapEditor.EditorToolController;
import MapEditor.EditorUiOptions;
import MapEditor.EditorViewportController;

import Shared.Core.Animations;
import Shared.Core.Map.PMSConstants;
import Shared.Core.Map.PMSEnums;
import Shared.Core.Map.PMSStructs;
import Shared.Core.State.StateManager;

namespace
{
using namespace Soldank;

PMSPolygon MakePolygon()
{
    PMSPolygon polygon{};
    polygon.vertices.at(0).x = 0.0F;
    polygon.vertices.at(0).y = 0.0F;
    polygon.vertices.at(1).x = 10.0F;
    polygon.vertices.at(1).y = 0.0F;
    polygon.vertices.at(2).x = 0.0F;
    polygon.vertices.at(2).y = 10.0F;
    return polygon;
}

class CountingAction final : public MapEditorAction
{
public:
    CountingAction(int& value, bool can_execute = true)
        : value_(value)
        , can_execute_(can_execute)
    {
    }

    bool CanExecute(const ClientState&, const StateManager&) final { return can_execute_; }
    void Execute(ClientState&, StateManager&) final { ++value_; }
    void Undo(ClientState&, StateManager&) final { --value_; }

private:
    int& value_;
    bool can_execute_;
};

class RecordingSink final : public EditorEventSink
{
public:
    void OnEditorEvents(std::span<const EditorEvent> events) final
    {
        event_count += events.size();
        last_was_tool_selection = std::holds_alternative<EditorToolSelectedEvent>(events.back());
    }

    std::size_t event_count = 0;
    bool last_was_tool_selection = false;
};

class MapEditorControllersTest : public testing::Test
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

    MapEditorControllersTest()
        : animation_data_manager_(MakeAnimationDataManager())
        , state_manager_(animation_data_manager_)
    {
        state_manager_.CreateEmptyMapDocument();
        client_state_.input.window_width = 800.0F;
        client_state_.input.window_height = 600.0F;
        client_state_.world_render_options.current_polygon_texture_dimensions = { 100.0F, 100.0F };
    }

    AnimationDataManager animation_data_manager_;
    StateManager state_manager_;
    ClientState client_state_{};
};

TEST_F(MapEditorControllersTest, CommandHistoryExecutesUndoesRedoesAndRejectsActions)
{
    EditorCommandHistory history;
    int value = 0;
    EXPECT_FALSE(history.Execute(
      client_state_, state_manager_, std::make_unique<CountingAction>(value, false)));
    EXPECT_TRUE(
      history.Execute(client_state_, state_manager_, std::make_unique<CountingAction>(value)));
    EXPECT_EQ(value, 1);
    EXPECT_TRUE(client_state_.map_editor_state.is_undo_enabled);

    history.Undo(client_state_, state_manager_);
    EXPECT_EQ(value, 0);
    EXPECT_TRUE(client_state_.map_editor_state.is_redo_enabled);
    history.Redo(client_state_, state_manager_);
    EXPECT_EQ(value, 1);
    history.Redo(client_state_, state_manager_);
    history.Undo(client_state_, state_manager_);
    history.Undo(client_state_, state_manager_);
    EXPECT_TRUE(client_state_.map_editor_state.is_map_changed);

    for (int i = 0; i < 52; ++i) {
        EXPECT_TRUE(
          history.Execute(client_state_, state_manager_, std::make_unique<CountingAction>(value)));
    }
}

TEST_F(MapEditorControllersTest, ShortcutControllerRecognizesModifiersAndEveryToolKey)
{
    EditorShortcutController shortcuts;
    EXPECT_FALSE(shortcuts.IsSaveShortcut(GLFW_KEY_S));
    shortcuts.OnKeyPressed(GLFW_KEY_LEFT_CONTROL);
    EXPECT_TRUE(shortcuts.IsSaveShortcut(GLFW_KEY_S));
    EXPECT_TRUE(shortcuts.IsUndoShortcut(GLFW_KEY_Z));
    EXPECT_TRUE(shortcuts.IsMapSettingsShortcut(GLFW_KEY_M));
    EXPECT_TRUE(shortcuts.IsCopyShortcut(GLFW_KEY_C));
    EXPECT_TRUE(shortcuts.IsPasteShortcut(GLFW_KEY_V));
    shortcuts.OnKeyPressed(GLFW_KEY_LEFT_SHIFT);
    EXPECT_TRUE(shortcuts.IsRedoShortcut(GLFW_KEY_Z));

    const std::vector<int> tool_keys = { GLFW_KEY_A, GLFW_KEY_Q, GLFW_KEY_S, GLFW_KEY_W,
                                         GLFW_KEY_D, GLFW_KEY_E, GLFW_KEY_F, GLFW_KEY_R,
                                         GLFW_KEY_G, GLFW_KEY_T, GLFW_KEY_H };
    const std::array<int, 11> tool_shortcut_keys{ GLFW_KEY_A, GLFW_KEY_Q, GLFW_KEY_S, GLFW_KEY_W,
                                                  GLFW_KEY_D, GLFW_KEY_E, GLFW_KEY_F, GLFW_KEY_R,
                                                  GLFW_KEY_G, GLFW_KEY_T, GLFW_KEY_H };
    for (int key : tool_keys) {
        EXPECT_TRUE(shortcuts.GetToolForKey(key, 0, tool_shortcut_keys));
    }
    std::array<int, 11> modified_tool_shortcut_keys = tool_shortcut_keys;
    modified_tool_shortcut_keys.front() =
      EncodeShortcut(GLFW_KEY_C, GLFW_MOD_CONTROL | GLFW_MOD_SHIFT);
    EXPECT_EQ(shortcuts.GetToolForKey(
                GLFW_KEY_C, GLFW_MOD_CONTROL | GLFW_MOD_SHIFT, modified_tool_shortcut_keys),
              ToolType::Transform);
    EXPECT_FALSE(
      shortcuts.GetToolForKey(GLFW_KEY_C, GLFW_MOD_CONTROL, modified_tool_shortcut_keys));
    EXPECT_FALSE(shortcuts.GetToolForKey(GLFW_KEY_ESCAPE, 0, tool_shortcut_keys));
    shortcuts.OnKeyReleased(GLFW_KEY_LEFT_SHIFT);
    shortcuts.OnKeyReleased(GLFW_KEY_LEFT_CONTROL);
    EXPECT_FALSE(shortcuts.IsUndoShortcut(GLFW_KEY_Z));
}

TEST_F(MapEditorControllersTest, ViewportControllerDragsAndZoomsAroundMouse)
{
    EditorViewportController viewport;
    client_state_.camera.position = { 10.0F, 20.0F };
    client_state_.input.mouse_map_position = { 100.0F, 80.0F };
    viewport.OnMouseScreenPositionChange(client_state_, { 20.0F, 30.0F });
    viewport.BeginCameraDrag(client_state_);
    viewport.OnMouseScreenPositionChange(client_state_, { 40.0F, 60.0F });
    EXPECT_NE(client_state_.camera.position, glm::vec2(10.0F, 20.0F));
    viewport.EndCameraDrag();
    const glm::vec2 position = client_state_.camera.position;
    viewport.OnMouseScreenPositionChange(client_state_, { 80.0F, 90.0F });
    EXPECT_EQ(client_state_.camera.position, position);

    const float initial_zoom = client_state_.camera.view.GetZoom();
    viewport.ZoomInAtMouse(client_state_);
    EXPECT_LT(client_state_.camera.view.GetZoom(), initial_zoom);
    viewport.ZoomOutAtMouse(client_state_);
    EXPECT_FLOAT_EQ(client_state_.camera.view.GetZoom(), initial_zoom);
}

TEST_F(MapEditorControllersTest, EventRouterAndToolControllerExposeTheirState)
{
    EditorEventRouter router;
    RecordingSink first;
    RecordingSink second;
    router.AddSink(first);
    router.AddSink(second);
    router.Emit(EditorToolSelectedEvent{ ToolType::Polygon });
    EXPECT_EQ(first.event_count, 1U);
    EXPECT_TRUE(second.last_was_tool_selection);

    int actions = 0;
    EditorToolController tools([&actions](std::unique_ptr<MapEditorAction>) { ++actions; },
                               [](MapEditorAction* action) { return action; });
    EXPECT_TRUE(tools.IsActive());
    tools.Select(ToolType::Polygon, client_state_, state_manager_);
    EXPECT_EQ(client_state_.map_editor_state.current_tool_action_description, "Create Polygon");
    tools.Deactivate();
    tools.Select(ToolType::Selection, client_state_, state_manager_);
    EXPECT_FALSE(tools.IsActive());
    tools.Activate();
    tools.Select(ToolType::Selection, client_state_, state_manager_);
    EXPECT_TRUE(tools.IsActive());
    EXPECT_EQ(actions, 0);
}

TEST_F(MapEditorControllersTest, UiOptionsAndAssetBrowserReturnCompleteSortedChoices)
{
    EXPECT_EQ(EditorUiOptions::GetToolOptions().size(), 11U);
    EXPECT_EQ(EditorUiOptions::GetDisabledToolTypes().size(), 3U);
    EXPECT_EQ(EditorUiOptions::GetSpawnPointOptions().size(), 17U);
    EXPECT_EQ(EditorUiOptions::GetPolygonTypeOptions().size(), 24U);
    EXPECT_EQ(EditorUiOptions::GetWeatherOptions().size(), 4U);
    EXPECT_EQ(EditorUiOptions::GetStepOptions().size(), 3U);
    EXPECT_EQ(EditorUiOptions::GetJetFuelOptions().size(), 8U);
    EXPECT_EQ(EditorUiOptions::GetSupportedImageExtensions().size(), 5U);
    EXPECT_EQ(EditorUiOptions::AlphaToOpacityPercent(0.0F), 0);
    EXPECT_EQ(EditorUiOptions::AlphaToOpacityPercent(0.495F), 50);
    EXPECT_EQ(EditorUiOptions::AlphaToOpacityPercent(1.0F), 100);
    EXPECT_EQ(EditorUiOptions::AlphaToOpacityPercent(2.0F), 100);
    EXPECT_FLOAT_EQ(EditorUiOptions::OpacityPercentToAlpha(0), 0.0F);
    EXPECT_FLOAT_EQ(EditorUiOptions::OpacityPercentToAlpha(50), 0.5F);
    EXPECT_FLOAT_EQ(EditorUiOptions::OpacityPercentToAlpha(100), 1.0F);
    EXPECT_FLOAT_EQ(EditorUiOptions::OpacityPercentToAlpha(-1), 0.0F);

    std::filesystem::create_directories("textures/subdirectory");
    std::filesystem::create_directories("scenery-gfx");
    std::ofstream("textures/z.png");
    std::ofstream("textures/a.jpg");
    std::ofstream("textures/ignored.txt");
    std::ofstream("textures/no_extension");
    std::ofstream("scenery-gfx/tree.bmp");
    EXPECT_EQ(EditorAssetBrowser::LoadTextureNames(),
              (std::vector<std::string>{ "a.jpg", "z.png" }));
    EXPECT_EQ(EditorAssetBrowser::LoadSceneryNames(), (std::vector<std::string>{ "tree.bmp" }));
    std::filesystem::remove_all("textures");
    std::filesystem::remove_all("scenery-gfx");
}

TEST_F(MapEditorControllersTest, DocumentAndMapPropertiesUpdateStateAndMap)
{
    EditorDocument document(client_state_, state_manager_);
    document.SaveCurrentMapOrOpenSaveAs();
    EXPECT_TRUE(client_state_.map_editor_state.should_open_save_as_modal);
    document.OpenMapSettings();
    EXPECT_TRUE(client_state_.map_editor_state.should_open_map_settings_modal);
    client_state_.map_editor_state.is_map_changed = true;
    document.MarkClean();
    EXPECT_FALSE(client_state_.map_editor_state.is_map_changed);

    EditorMapProperties properties(client_state_.map_editor_state, state_manager_);
    client_state_.map_editor_state.event_set_map_name.Notify("properties-map");
    client_state_.map_editor_state.event_set_map_description.Notify("description");
    client_state_.map_editor_state.event_set_map_weather_type.Notify(PMSWeatherType::Snow);
    client_state_.map_editor_state.event_set_map_step_type.Notify(PMSStepType::SoftGround);
    client_state_.map_editor_state.event_set_map_grenades_count.Notify(4);
    client_state_.map_editor_state.event_set_map_medikits_count.Notify(5);
    client_state_.map_editor_state.event_set_map_jet_count.Notify(320);
    const PMSColor top(1, 2, 3, 4);
    const PMSColor bottom(5, 6, 7, 8);
    client_state_.map_editor_state.event_set_map_background_top_color.Notify(top);
    client_state_.map_editor_state.event_set_map_background_bottom_color.Notify(bottom);
    client_state_.map_editor_state.event_set_map_texture_name.Notify("stone.png");

    const auto& map = state_manager_.GetConstMap();
    EXPECT_EQ(map.GetName(), "properties-map");
    EXPECT_EQ(map.GetDescription(), "description");
    EXPECT_EQ(map.GetWeatherType(), PMSWeatherType::Snow);
    EXPECT_EQ(map.GetStepType(), PMSStepType::SoftGround);
    EXPECT_EQ(map.GetGrenadesCount(), 4);
    EXPECT_EQ(map.GetMedikitsCount(), 5);
    EXPECT_EQ(map.GetJetCount(), 320);
    EXPECT_EQ(map.GetTextureName(), "stone.png");

    EXPECT_EQ(client_state_.map_editor_state.map_description_input.size(),
              DESCRIPTION_MAX_LENGTH + 1U);
    const std::string maximum_description(DESCRIPTION_MAX_LENGTH, 'x');
    client_state_.map_editor_state.event_set_map_description.Notify(maximum_description);
    EXPECT_NO_THROW(state_manager_.GetMap().SaveMap("maximum-description.pms"));
    std::filesystem::remove("maximum-description.pms");
}

TEST_F(MapEditorControllersTest, MapEditorRoutesInputActionsPropertiesAndLocking)
{
    MapEditor editor(client_state_, state_manager_);
    EXPECT_EQ(client_state_.map_editor_state.current_tool_action_description, "Select Objects");
    EXPECT_EQ(client_state_.map_editor_state.palette_saved_colors.front(),
              glm::vec4(1.0F, 1.0F, 1.0F, 1.0F));

    client_state_.event_key_pressed.Notify(GLFW_KEY_Q, 0);
    EXPECT_EQ(client_state_.map_editor_state.selected_tool, ToolType::Polygon);
    client_state_.event_mouse_map_position_changed.Notify(glm::vec2{}, glm::vec2(0.0F, 0.0F));
    client_state_.event_left_mouse_button_clicked.Notify();
    client_state_.event_mouse_map_position_changed.Notify(glm::vec2{}, glm::vec2(10.0F, 0.0F));
    client_state_.event_left_mouse_button_clicked.Notify();
    client_state_.event_mouse_map_position_changed.Notify(glm::vec2{}, glm::vec2(0.0F, 10.0F));
    client_state_.event_left_mouse_button_clicked.Notify();
    EXPECT_EQ(state_manager_.GetConstMap().GetPolygonsCount(), 1U);
    EXPECT_TRUE(client_state_.map_editor_state.is_undo_enabled);
    client_state_.map_editor_state.event_pressed_undo.Notify();
    EXPECT_EQ(state_manager_.GetConstMap().GetPolygonsCount(), 0U);
    client_state_.map_editor_state.event_pressed_redo.Notify();
    EXPECT_EQ(state_manager_.GetConstMap().GetPolygonsCount(), 1U);

    PMSScenery scenery{};
    const unsigned int scenery_id = state_manager_.GetMap().AddNewScenery(scenery, "tree.png");
    PMSSpawnPoint spawn{};
    const unsigned int spawn_id = state_manager_.GetMap().AddNewSpawnPoint(spawn);
    client_state_.map_editor_state.selected_polygon_vertices = { { 0U, std::bitset<3>(0b111) } };
    client_state_.map_editor_state.selected_scenery_ids = { scenery_id };
    client_state_.map_editor_state.selected_spawn_point_ids = { spawn_id };
    client_state_.map_editor_state.event_selected_polygons_bounciness_changed.Notify(42.0F);
    client_state_.map_editor_state.event_selected_polygons_type_changed.Notify(PMSPolygonType::Ice);
    client_state_.map_editor_state.event_selected_sceneries_level_changed.Notify(2);
    client_state_.map_editor_state.event_selected_spawn_points_type_changed.Notify(
      PMSSpawnPointType::Alpha);
    EXPECT_FLOAT_EQ(state_manager_.GetConstMap().GetPolygons().at(0).bounciness, 42.0F);
    EXPECT_EQ(state_manager_.GetConstMap().GetPolygons().at(0).polygon_type, PMSPolygonType::Ice);
    EXPECT_EQ(state_manager_.GetConstMap().GetSceneryInstances().at(0).level, 2);
    EXPECT_EQ(state_manager_.GetConstMap().GetSpawnPoints().at(0).type, PMSSpawnPointType::Alpha);

    client_state_.event_key_pressed.Notify(GLFW_KEY_LEFT_CONTROL, GLFW_MOD_CONTROL);
    client_state_.event_key_pressed.Notify(GLFW_KEY_C, GLFW_MOD_CONTROL);
    client_state_.event_key_pressed.Notify(GLFW_KEY_V, GLFW_MOD_CONTROL);
    client_state_.event_key_released.Notify(GLFW_KEY_LEFT_CONTROL, 0);
    EXPECT_EQ(state_manager_.GetConstMap().GetPolygonsCount(), 2U);
    EXPECT_EQ(state_manager_.GetConstMap().GetSceneryInstances().size(), 2U);
    EXPECT_EQ(state_manager_.GetConstMap().GetSpawnPoints().size(), 2U);

    client_state_.event_key_pressed.Notify(GLFW_KEY_LEFT_CONTROL, GLFW_MOD_CONTROL);
    client_state_.event_key_pressed.Notify(GLFW_KEY_M, GLFW_MOD_CONTROL);
    client_state_.event_key_pressed.Notify(GLFW_KEY_S, GLFW_MOD_CONTROL);
    client_state_.event_key_released.Notify(GLFW_KEY_LEFT_CONTROL, 0);
    EXPECT_TRUE(client_state_.map_editor_state.should_open_map_settings_modal);
    EXPECT_TRUE(client_state_.map_editor_state.should_open_save_as_modal);

    client_state_.event_middle_mouse_button_clicked.Notify();
    client_state_.event_mouse_screen_position_changed.Notify(glm::vec2{}, glm::vec2(5.0F, 8.0F));
    client_state_.event_middle_mouse_button_released.Notify();
    client_state_.event_mouse_wheel_scrolled_up.Notify();
    client_state_.event_mouse_wheel_scrolled_down.Notify();
    client_state_.event_right_mouse_button_clicked.Notify();
    client_state_.event_right_mouse_button_released.Notify();
    client_state_.event_left_mouse_button_released.Notify();

    editor.Lock();
    const auto polygon_count = state_manager_.GetConstMap().GetPolygonsCount();
    client_state_.map_editor_state.event_selected_new_tool.Notify(ToolType::Selection);
    client_state_.event_left_mouse_button_clicked.Notify();
    client_state_.event_left_mouse_button_released.Notify();
    client_state_.event_right_mouse_button_clicked.Notify();
    client_state_.event_right_mouse_button_released.Notify();
    client_state_.event_middle_mouse_button_clicked.Notify();
    client_state_.event_middle_mouse_button_released.Notify();
    client_state_.event_mouse_wheel_scrolled_up.Notify();
    client_state_.event_mouse_wheel_scrolled_down.Notify();
    client_state_.event_mouse_screen_position_changed.Notify(glm::vec2{}, glm::vec2{});
    client_state_.event_mouse_map_position_changed.Notify(glm::vec2{}, glm::vec2{});
    client_state_.event_key_pressed.Notify(GLFW_KEY_DELETE, 0);
    EXPECT_EQ(state_manager_.GetConstMap().GetPolygonsCount(), polygon_count);
    editor.Unlock();

    client_state_.map_editor_state.is_mouse_hovering_over_ui = true;
    client_state_.event_left_mouse_button_clicked.Notify();
    client_state_.event_right_mouse_button_clicked.Notify();
    client_state_.event_middle_mouse_button_clicked.Notify();
    client_state_.map_editor_state.is_modal_or_popup_open = true;
    client_state_.event_key_pressed.Notify(GLFW_KEY_DELETE, 0);
    client_state_.event_mouse_wheel_scrolled_up.Notify();
}

TEST_F(MapEditorControllersTest, MapEditorConfigSavesAndLoadsPaletteColors)
{
    const std::filesystem::path config_path = "map-editor-palette-test.toml";
    const std::array<glm::vec4, 2> saved_colors{
        glm::vec4(1.0F, 128.0F / 255.0F, 0.0F, 64.0F / 255.0F),
        glm::vec4(0.0F, 64.0F / 255.0F, 1.0F, 1.0F),
    };

    ASSERT_TRUE(MapEditorConfig::SavePalette(config_path, saved_colors));

    std::array<glm::vec4, 2> loaded_colors{};
    ASSERT_TRUE(MapEditorConfig::LoadPalette(config_path, loaded_colors));
    EXPECT_EQ(loaded_colors, saved_colors);

    std::filesystem::remove(config_path);
}

TEST_F(MapEditorControllersTest, MapEditorConfigSavesAndLoadsPlayModeShortcut)
{
    const std::filesystem::path config_path = "map-editor-settings-test.toml";
    const std::array<glm::vec4, 1> saved_colors{ glm::vec4(1.0F) };

    const std::array<int, 11> saved_tool_shortcuts{ EncodeShortcut(GLFW_KEY_1, GLFW_MOD_CONTROL),
                                                    GLFW_KEY_2,
                                                    GLFW_KEY_3,
                                                    GLFW_KEY_4,
                                                    GLFW_KEY_5,
                                                    GLFW_KEY_6,
                                                    GLFW_KEY_7,
                                                    GLFW_KEY_8,
                                                    GLFW_KEY_9,
                                                    GLFW_KEY_0,
                                                    GLFW_KEY_MINUS };
    ASSERT_TRUE(
      MapEditorConfig::SaveSettings(config_path,
                                    saved_colors,
                                    EncodeShortcut(GLFW_KEY_P, GLFW_MOD_CONTROL | GLFW_MOD_SHIFT),
                                    1.5F,
                                    saved_tool_shortcuts));

    std::array<glm::vec4, 1> loaded_colors{};
    int play_mode_shortcut_key = GLFW_KEY_F5;
    float ui_scale = 1.0F;
    std::array<int, 11> loaded_tool_shortcuts{};
    ASSERT_TRUE(MapEditorConfig::LoadSettings(
      config_path, loaded_colors, play_mode_shortcut_key, ui_scale, loaded_tool_shortcuts));
    EXPECT_EQ(loaded_colors, saved_colors);
    EXPECT_EQ(play_mode_shortcut_key,
              EncodeShortcut(GLFW_KEY_P, GLFW_MOD_CONTROL | GLFW_MOD_SHIFT));
    EXPECT_FLOAT_EQ(ui_scale, 1.5F);
    EXPECT_EQ(loaded_tool_shortcuts, saved_tool_shortcuts);

    std::filesystem::remove(config_path);
}

TEST_F(MapEditorControllersTest, MapEditorConfigRejectsInvalidPaletteWithoutChangingColors)
{
    const std::filesystem::path config_path = "invalid-map-editor-palette-test.toml";
    {
        std::ofstream config_file(config_path);
        config_file << "[palette]\ncolors = [[256, 0, 0, 255]]\n";
    }
    std::array<glm::vec4, 1> colors{ glm::vec4(0.25F, 0.5F, 0.75F, 1.0F) };
    const auto original_colors = colors;

    EXPECT_FALSE(MapEditorConfig::LoadPalette(config_path, colors));
    EXPECT_EQ(colors, original_colors);

    std::filesystem::remove(config_path);
}
} // namespace
