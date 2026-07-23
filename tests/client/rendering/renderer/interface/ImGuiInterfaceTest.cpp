#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_te_context.h"
#include "imgui_te_engine.h"

#include <gtest/gtest.h>

#include <bitset>
#include <string>

import ClientState;
import DebugUI;
import ImGuiThemes;
import MapEditorState;
import MapEditorUI;

import Shared.Core.Animations;
import Shared.Core.Map.Map;
import Shared.Core.Map.PMSEnums;
import Shared.Core.Map.PMSStructs;
import Shared.Core.State.StateManager;

namespace
{
using namespace Soldank;

class ImGuiInterfaceTest : public testing::Test
{
public:
    ClientState& GetClientState() { return client_state_; }
    StateManager& GetStateManager() { return state_manager_; }

    struct ObservedEvents
    {
        bool undo_pressed = false;
        bool redo_pressed = false;
        bool play_pressed = false;
        bool close_requested = false;
        bool copied = false;
        bool pasted = false;
        bool layer_changed = false;
        bool ui_scale_changed = false;
        bool shortcuts_changed = false;
        std::string saved_map_path;
        std::string new_map_name;
        std::string description;
        PMSWeatherType weather = PMSWeatherType::None;
        PMSStepType step = PMSStepType::HardGround;
        int grenades = -1;
        int medikits = -1;
        int jet_count = -1;
    };

    ObservedEvents& GetObservedEvents() { return observed_events_; }

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

    ImGuiInterfaceTest()
        : animation_data_manager_(MakeAnimationDataManager())
        , state_manager_(animation_data_manager_)
    {
        state_manager_.CreateEmptyMapDocument();
        client_state_.input.window_width = 1280.0F;
        client_state_.input.window_height = 720.0F;
    }

    void SetUp() override
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        SetupImGuiTheme();

        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = { client_state_.input.window_width, client_state_.input.window_height };
        io.DeltaTime = 1.0F / 60.0F;
        io.IniFilename = nullptr;
        io.LogFilename = nullptr;
        unsigned char* font_pixels = nullptr;
        int font_width = 0;
        int font_height = 0;
        io.Fonts->GetTexDataAsRGBA32(&font_pixels, &font_width, &font_height);

        engine_ = ImGuiTestEngine_CreateContext();
        ImGuiTestEngineIO& test_io = ImGuiTestEngine_GetIO(engine_);
        test_io.ConfigSavedSettings = false;
        test_io.ConfigCaptureEnabled = false;
        test_io.ConfigRunSpeed = ImGuiTestRunSpeed_Fast;
        test_io.ConfigVerboseLevel = ImGuiTestVerboseLevel_Warning;
        test_io.ConfigVerboseLevelOnError = ImGuiTestVerboseLevel_Debug;
        test_io.ConfigLogToTTY = true;
        ImGuiTestEngine_Start(engine_, ImGui::GetCurrentContext());
    }

    void TearDown() override
    {
        ImGuiTestEngine_Stop(engine_);
        ImGui::DestroyContext();
        ImGuiTestEngine_DestroyContext(engine_);
    }

    void RunTest(ImGuiTest* test)
    {
        ImGuiTestEngine_QueueTest(engine_, test);

        constexpr int MAX_FRAME_COUNT = 2000;
        int frame_count = 0;
        while (!ImGuiTestEngine_IsTestQueueEmpty(engine_) && frame_count < MAX_FRAME_COUNT) {
            ImGuiIO& io = ImGui::GetIO();
            io.DisplaySize = { client_state_.input.window_width,
                               client_state_.input.window_height };
            io.DeltaTime = 1.0F / 60.0F;

            ImGui::NewFrame();
            ImGui::Render();
            ImGuiTestEngine_PostSwap(engine_);
            ++frame_count;
        }

        int tested_count = 0;
        int success_count = 0;
        ImGuiTestEngine_GetResult(engine_, tested_count, success_count);
        EXPECT_TRUE(ImGuiTestEngine_IsTestQueueEmpty(engine_));
        EXPECT_EQ(tested_count, 1);
        EXPECT_EQ(success_count, 1);
    }

    static ImGuiInterfaceTest& GetFixture(ImGuiTestContext* context)
    {
        return *static_cast<ImGuiInterfaceTest*>(context->Test->UserData);
    }

    static ImGuiID GetScopedItemId(ImGuiTestContext* context,
                                   const char* window_name,
                                   const char* scope_name,
                                   const char* item_name)
    {
        ImGuiWindow* window = ImGui::FindWindowByName(window_name);
        if (window == nullptr) {
            context->LogError("Unable to find window '%s'", window_name);
            return 0;
        }
        const ImGuiID scope_id = window->GetID(scope_name);
        return ImHashStr(item_name, 0, scope_id);
    }

    static void RenderMapEditor(ImGuiTestContext* context)
    {
        ImGuiInterfaceTest& fixture = GetFixture(context);
        MapEditorUI::RenderFrameContents(fixture.state_manager_, fixture.client_state_);
    }

    static void RenderDebugUI(ImGuiTestContext* context)
    {
        ImGuiInterfaceTest& fixture = GetFixture(context);
        DebugUI::RenderFrameContents(fixture.state_manager_, fixture.client_state_, 60);
    }

    AnimationDataManager animation_data_manager_;
    StateManager state_manager_;
    ClientState client_state_{};
    ObservedEvents observed_events_{};
    ImGuiTestEngine* engine_ = nullptr;
};

TEST_F(ImGuiInterfaceTest, ViewMenuTogglesSnapOptions)
{
    client_state_.map_editor_state.is_snap_to_grid_enabled = false;
    client_state_.map_editor_state.is_snap_to_vertices_enabled = false;

    ImGuiTest* test = IM_REGISTER_TEST(engine_, "map_editor_interface", "view_menu_snap");
    test->UserData = this;
    test->GuiFunc = RenderMapEditor;
    test->TestFunc = [](ImGuiTestContext* context) {
        ImGuiInterfaceTest& fixture = GetFixture(context);

        context->MenuCheck("//##MainMenuBar/View/Snap to grid");
        IM_CHECK(fixture.GetClientState().map_editor_state.is_snap_to_grid_enabled);
        context->MenuUncheck("//##MainMenuBar/View/Snap to grid");
        IM_CHECK(!fixture.GetClientState().map_editor_state.is_snap_to_grid_enabled);

        context->MenuCheck("//##MainMenuBar/View/Snap to vertices");
        IM_CHECK(fixture.GetClientState().map_editor_state.is_snap_to_vertices_enabled);
        context->MenuUncheck("//##MainMenuBar/View/Snap to vertices");
        IM_CHECK(!fixture.GetClientState().map_editor_state.is_snap_to_vertices_enabled);
    };

    RunTest(test);
}

TEST_F(ImGuiInterfaceTest, DisplayWindowTogglesRenderOptions)
{
    client_state_.map_editor_state.is_tools_window_visible = false;
    client_state_.map_editor_state.is_properties_window_visible = false;
    client_state_.map_editor_state.is_palette_window_visible = false;
    client_state_.map_editor_state.is_tool_details_window_visible = false;

    ImGuiTest* test = IM_REGISTER_TEST(engine_, "map_editor_interface", "display_options");
    test->UserData = this;
    test->GuiFunc = RenderMapEditor;
    test->TestFunc = [](ImGuiTestContext* context) {
        ImGuiInterfaceTest& fixture = GetFixture(context);
        context->SetRef("Display");

        context->ItemUncheck("Background");
        IM_CHECK(!fixture.GetClientState().world_render_options.draw_background);
        context->ItemUncheck("Polygons");
        IM_CHECK(!fixture.GetClientState().world_render_options.draw_polygons);
        context->ItemUncheck("Sceneries");
        IM_CHECK(!fixture.GetClientState().world_render_options.draw_sceneries);
        context->ItemUncheck("Spawn points");
        IM_CHECK(!fixture.GetClientState().map_editor_state.draw_spawn_points);
        context->ItemCheck("Wireframe");
        IM_CHECK(fixture.GetClientState().map_editor_state.draw_wireframe);
        context->ItemCheck("Grid");
        IM_CHECK(fixture.GetClientState().map_editor_state.is_grid_visible);
    };

    RunTest(test);
}

TEST_F(ImGuiInterfaceTest, DebugWindowsModifyClientState)
{
    client_state_.debug_render.is_game_debug_interface_enabled = true;
    client_state_.network.network_lag = 0;

    ImGuiTest* test = IM_REGISTER_TEST(engine_, "debug_interface", "client_state_options");
    test->UserData = this;
    test->GuiFunc = RenderDebugUI;
    test->TestFunc = [](ImGuiTestContext* context) {
        ImGuiInterfaceTest& fixture = GetFixture(context);

        context->SetRef("Network window (Works only when connected to a server)");
        context->ItemUncheck("Server reconciliation");
        IM_CHECK(!fixture.GetClientState().network.server_reconciliation);
        context->ItemUncheck("Client side prediction");
        IM_CHECK(!fixture.GetClientState().network.client_side_prediction);
        context->ItemUncheck("Objects interpolation");
        IM_CHECK(!fixture.GetClientState().network.objects_interpolation);
        context->ItemInputValue("Fake lag (milliseconds)", 125);
        IM_CHECK_EQ(fixture.GetClientState().network.network_lag, 125);

        context->SetRef("Debug window");
        context->ItemCheck("Draw colliding polygons");
        IM_CHECK(fixture.GetClientState().debug_render.draw_colliding_polygons);
        context->ItemUncheck("Smooth camera");
        IM_CHECK(!fixture.GetClientState().camera.smooth);
        context->ItemCheck("Smooth camera");
        IM_CHECK(fixture.GetClientState().camera.smooth);
        context->ItemClick("\\/kill");
        IM_CHECK(fixture.GetClientState().kill_button_just_pressed);
    };

    RunTest(test);
}

TEST_F(ImGuiInterfaceTest, MainMenusControlWindowsAndSaveAs)
{
    client_state_.map_editor_state.is_undo_enabled = true;
    client_state_.map_editor_state.is_redo_enabled = true;
    client_state_.map_editor_state.is_map_changed = true;
    client_state_.map_editor_state.event_pressed_undo.AddObserver(
      [this] { observed_events_.undo_pressed = true; });
    client_state_.map_editor_state.event_pressed_redo.AddObserver(
      [this] { observed_events_.redo_pressed = true; });
    client_state_.map_editor_state.event_pressed_play.AddObserver(
      [this] { observed_events_.play_pressed = true; });
    client_state_.map_editor_state.event_close_application_requested.AddObserver(
      [this] { observed_events_.close_requested = true; });
    client_state_.map_editor_state.event_save_map.AddObserver(
      [this](const std::string& path) { observed_events_.saved_map_path = path; });
    client_state_.map_editor_state.event_set_map_name.AddObserver(
      [this](const std::string& name) { observed_events_.new_map_name = name; });

    ImGuiTest* test = IM_REGISTER_TEST(engine_, "map_editor_interface", "main_menus_and_save_as");
    test->UserData = this;
    test->GuiFunc = RenderMapEditor;
    test->TestFunc = [](ImGuiTestContext* context) {
        ImGuiInterfaceTest& fixture = GetFixture(context);
        ClientState& client_state = fixture.GetClientState();
        ObservedEvents& events = fixture.GetObservedEvents();

        context->MenuClick("//##MainMenuBar/Edit/Undo");
        IM_CHECK(events.undo_pressed);
        context->MenuClick("//##MainMenuBar/Edit/Redo");
        IM_CHECK(events.redo_pressed);
        context->MenuClick("//##MainMenuBar/Run/Play");
        IM_CHECK(events.play_pressed);

        context->MenuClick("//##MainMenuBar/Windows/Hide all");
        IM_CHECK(!client_state.map_editor_state.is_tools_window_visible);
        IM_CHECK(!client_state.map_editor_state.is_properties_window_visible);
        IM_CHECK(!client_state.map_editor_state.is_display_window_visible);
        IM_CHECK(!client_state.map_editor_state.is_palette_window_visible);
        context->MenuClick("//##MainMenuBar/Windows/Show all");
        IM_CHECK(client_state.map_editor_state.is_tools_window_visible);
        IM_CHECK(client_state.map_editor_state.is_properties_window_visible);
        IM_CHECK(client_state.map_editor_state.is_display_window_visible);
        IM_CHECK(client_state.map_editor_state.is_palette_window_visible);

        context->MenuClick("//##MainMenuBar/Windows/Tools");
        context->MenuClick("//##MainMenuBar/Windows/Properties");
        context->MenuClick("//##MainMenuBar/Windows/Display");
        context->MenuClick("//##MainMenuBar/Windows/Palette");
        IM_CHECK(!client_state.map_editor_state.is_tools_window_visible);
        IM_CHECK(!client_state.map_editor_state.is_properties_window_visible);
        IM_CHECK(!client_state.map_editor_state.is_display_window_visible);
        IM_CHECK(!client_state.map_editor_state.is_palette_window_visible);

        context->MenuClick("//##MainMenuBar/File/Save");
        context->SetRef("Save as...");
        context->ItemInputValue("##NewMapNameInput", "coverage-map");
        context->ItemClick("Save");
        IM_CHECK_EQ(events.new_map_name, "coverage-map.pms");
        IM_CHECK_EQ(events.saved_map_path, "maps/coverage-map.pms");
        IM_CHECK(!client_state.map_editor_state.is_map_changed);
    };

    RunTest(test);
}

TEST_F(ImGuiInterfaceTest, ToolsPaletteAndContextPopupsModifyEditorState)
{
    client_state_.map_editor_state.event_pressed_copy.AddObserver(
      [this] { observed_events_.copied = true; });
    client_state_.map_editor_state.event_pressed_paste.AddObserver(
      [this] { observed_events_.pasted = true; });
    client_state_.map_editor_state.event_selection_layer_order_changed.AddObserver(
      [this](SelectionLayerOrder) { observed_events_.layer_changed = true; });
    client_state_.map_editor_state.selected_polygon_vertices.emplace_back(0U, std::bitset<3>{ 1U });
    client_state_.map_editor_state.is_tool_details_window_visible = false;

    ImGuiTest* test = IM_REGISTER_TEST(engine_, "map_editor_interface", "tools_and_popups");
    test->UserData = this;
    test->GuiFunc = RenderMapEditor;
    test->TestFunc = [](ImGuiTestContext* context) {
        ImGuiInterfaceTest& fixture = GetFixture(context);
        ClientState& client_state = fixture.GetClientState();
        ObservedEvents& events = fixture.GetObservedEvents();

        context->SetRef("Tools");
        context->ItemClick("Polygon tool (Q)");
        IM_CHECK(client_state.map_editor_state.selected_tool == ToolType::Polygon);
        context->ItemClick("Scenery tool (R)");
        IM_CHECK(client_state.map_editor_state.selected_tool == ToolType::Scenery);
        context->ItemClick("Spawnpoint tool (T)");
        IM_CHECK(client_state.map_editor_state.selected_tool == ToolType::Spawnpoint);
        context->ItemClick("Color picker tool (H)");
        IM_CHECK(client_state.map_editor_state.selected_tool == ToolType::ColorPicker);

        context->SetRef("Palette");
        context->ItemInputValue("##PaletteOpacityPercent", 40);
        IM_CHECK_EQ(client_state.map_editor_state.palette_current_color.at(3), 0.4F);

        client_state.map_editor_state.should_open_spawn_point_type_popup = true;
        context->Yield();
        context->ItemClick("//$FOCUSED/Alpha Team");
        IM_CHECK(client_state.map_editor_state.selected_spawn_point_type ==
                 PMSSpawnPointType::Alpha);

        client_state.map_editor_state.should_open_polygon_type_popup = true;
        context->Yield();
        context->ItemClick("//$FOCUSED/Bouncy");
        IM_CHECK(client_state.map_editor_state.polygon_tool_polygon_type == PMSPolygonType::Bouncy);

        client_state.map_editor_state.should_open_selection_context_menu = true;
        context->Yield();
        context->ItemClick("//$FOCUSED/Bring To Front");
        IM_CHECK(events.layer_changed);

        client_state.map_editor_state.should_open_selection_context_menu = true;
        context->Yield();
        context->ItemClick("//$FOCUSED/Copy");
        IM_CHECK(events.copied);

        client_state.map_editor_state.should_open_selection_context_menu = true;
        context->Yield();
        context->ItemClick("//$FOCUSED/Paste");
        IM_CHECK(events.pasted);
    };

    RunTest(test);
}

TEST_F(ImGuiInterfaceTest, MapSettingsModalExercisesEditableOptions)
{
    client_state_.input.window_width = 2560.0F;
    client_state_.input.window_height = 1440.0F;
    client_state_.map_editor_state.event_set_map_description.AddObserver(
      [this](const std::string& value) { observed_events_.description = value; });
    client_state_.map_editor_state.event_set_map_weather_type.AddObserver(
      [this](PMSWeatherType value) { observed_events_.weather = value; });
    client_state_.map_editor_state.event_set_map_step_type.AddObserver(
      [this](PMSStepType value) { observed_events_.step = value; });
    client_state_.map_editor_state.event_set_map_grenades_count.AddObserver(
      [this](unsigned char value) { observed_events_.grenades = value; });
    client_state_.map_editor_state.event_set_map_medikits_count.AddObserver(
      [this](unsigned char value) { observed_events_.medikits = value; });
    client_state_.map_editor_state.event_set_map_jet_count.AddObserver(
      [this](int value) { observed_events_.jet_count = value; });
    client_state_.map_editor_state.should_open_map_settings_modal = true;

    ImGuiTest* test = IM_REGISTER_TEST(engine_, "map_editor_interface", "map_settings");
    test->UserData = this;
    test->GuiFunc = RenderMapEditor;
    test->TestFunc = [](ImGuiTestContext* context) {
        ImGuiInterfaceTest& fixture = GetFixture(context);
        ObservedEvents& events = fixture.GetObservedEvents();

        context->SetRef("Map settings");
        context->ItemInputValue("##DescriptionInput", "Covered description");
        IM_CHECK_EQ(events.description, "Covered description");

        context->ItemClick(
          GetScopedItemId(context, "Map settings", "weather_and_step_table", "##WeatherInput"));
        context->ItemClick("//$FOCUSED/Rain");
        IM_CHECK(events.weather == PMSWeatherType::Rain);
        context->ItemClick(
          GetScopedItemId(context, "Map settings", "weather_and_step_table", "##StepsInput"));
        context->ItemClick("//$FOCUSED/Soft");
        IM_CHECK(events.step == PMSStepType::SoftGround);

        context->ItemInputValue("##GrenadesInput", 7);
        IM_CHECK_EQ(events.grenades, 7);
        context->ItemInputValue("##MedikitsInput", 8);
        IM_CHECK_EQ(events.medikits, 8);
        context->ItemClick("Normal##JetFuelInputNormal");
        IM_CHECK_EQ(events.jet_count, 190);
        context->ItemInputValue("##JetFuelCustomInput", 321);
        IM_CHECK_EQ(events.jet_count, 321);

        context->ItemClick(GetScopedItemId(
          context, "Map settings", "texture_and_background_table", "##TextureComboPicker"));
        IM_CHECK(context->ItemExists("//$FOCUSED/##TextureSearchFilterInput"));
        context->PopupCloseOne();
        context->ItemClick("CLOSE");
    };

    RunTest(test);
}

TEST_F(ImGuiInterfaceTest, SettingsModalCoversGeneralAndShortcutSections)
{
    client_state_.input.window_width = 2560.0F;
    client_state_.input.window_height = 1440.0F;
    client_state_.map_editor_state.event_ui_scale_changed.AddObserver(
      [this] { observed_events_.ui_scale_changed = true; });
    client_state_.map_editor_state.event_shortcuts_changed.AddObserver(
      [this] { observed_events_.shortcuts_changed = true; });
    client_state_.map_editor_state.should_open_settings_modal = true;

    ImGuiTest* test = IM_REGISTER_TEST(engine_, "map_editor_interface", "settings");
    test->UserData = this;
    test->GuiFunc = RenderMapEditor;
    test->TestFunc = [](ImGuiTestContext* context) {
        ImGuiInterfaceTest& fixture = GetFixture(context);
        ClientState& client_state = fixture.GetClientState();
        ObservedEvents& events = fixture.GetObservedEvents();

        context->SetRef("Settings");
        context->Yield();
        context->SetRef(context->WindowInfo("Settings content").Window);
        context->ItemInputValue("UI scale", 150.0F);
        IM_CHECK_EQ(client_state.map_editor_state.ui_scale, 1.5F);
        IM_CHECK(events.ui_scale_changed);
        context->ItemClick("Reset default settings");
        IM_CHECK_EQ(client_state.map_editor_state.ui_scale, 1.0F);

        context->SetRef(context->WindowInfo("//Settings/Settings navigation").Window);
        context->ItemClick("Shortcuts");
        IM_CHECK(client_state.map_editor_state.selected_settings_section ==
                 SettingsSection::Shortcuts);
        context->Yield();
        ImGuiWindow* content_window = context->WindowInfo("//Settings/Settings content").Window;
        context->SetRef(content_window);
        context->ItemClick(GetScopedItemId(
          context, content_window->Name, "PlayTestShortcuts", "Switch to play mode"));
        context->SetRef("Settings");
        context->ItemClick("Assign shortcut");
        IM_CHECK(client_state.map_editor_state.is_play_mode_shortcut_capture_active);
        context->ItemClick("Remove shortcut");
        IM_CHECK(events.shortcuts_changed);

        context->SetRef(content_window);
        context->ItemClick(
          GetScopedItemId(context, content_window->Name, "ToolShortcuts", "Polygon"));
        context->SetRef("Settings");
        context->ItemClick("Remove shortcut");
        context->SetRef(content_window);
        context->ItemClick(GetScopedItemId(context, content_window->Name, "save", "Save"));
        context->SetRef("Settings");
        context->ItemClick("Remove shortcut");
        context->ItemClick("CLOSE");
    };

    RunTest(test);
}

TEST_F(ImGuiInterfaceTest, ToolDetailsRenderPolygonSceneryAndSelections)
{
    PMSPolygon polygon{};
    polygon.id = 0;
    polygon.polygon_type = PMSPolygonType::Bouncy;
    polygon.bounciness = 1.5F;
    polygon.vertices.at(0).x = 10.0F;
    polygon.vertices.at(1).x = 20.0F;
    polygon.vertices.at(2).x = 30.0F;
    state_manager_.GetMap().AddNewPolygon(polygon);

    PMSScenery scenery{};
    scenery.level = 1;
    scenery.scale_x = 1.0F;
    scenery.scale_y = 1.0F;
    state_manager_.GetMap().AddNewScenery(scenery, "coverage.png");

    PMSSpawnPoint spawn_point{};
    spawn_point.type = PMSSpawnPointType::General;
    spawn_point.x = 12;
    spawn_point.y = 34;
    state_manager_.GetMap().AddNewSpawnPoint(spawn_point);

    client_state_.map_editor_state.is_tools_window_visible = false;
    client_state_.map_editor_state.is_properties_window_visible = false;
    client_state_.map_editor_state.is_display_window_visible = false;
    client_state_.map_editor_state.is_palette_window_visible = false;
    client_state_.map_editor_state.selected_tool = ToolType::Polygon;
    client_state_.map_editor_state.polygon_tool_polygon_type = PMSPolygonType::Bouncy;
    client_state_.map_editor_state.polygon_tool_wip_polygon_edge = polygon;
    client_state_.map_editor_state.polygon_tool_wip_polygon = polygon;

    ImGuiTest* test = IM_REGISTER_TEST(engine_, "map_editor_interface", "tool_details");
    test->UserData = this;
    test->GuiFunc = RenderMapEditor;
    test->TestFunc = [](ImGuiTestContext* context) {
        ImGuiInterfaceTest& fixture = GetFixture(context);
        ClientState& client_state = fixture.GetClientState();

        context->SetRef("Tool Details");
        context->ComboClick("##PolygonTypeComboInput/Icy");
        IM_CHECK(client_state.map_editor_state.polygon_tool_polygon_type == PMSPolygonType::Ice);

        client_state.map_editor_state.selected_tool = ToolType::Scenery;
        client_state.map_editor_state.selected_scenery_to_place = "coverage.png";
        context->Yield();
        context->ItemInputValue("##SceneryToolWIPSceneryRotation", 90.0F);
        context->ItemInputValue("##SceneryToolWIPSceneryScaleX", 125.0F);
        context->ItemInputValue("##SceneryToolWIPSceneryScaleY", 75.0F);

        client_state.map_editor_state.selected_tool = ToolType::Selection;
        client_state.map_editor_state.selected_polygon_vertices = { { 0U, std::bitset<3>{ 7U } } };
        context->Yield();
        IM_CHECK(context->ItemExists("Selected 1 polygons"));
        context->ComboClick("##ToolDetailsPolygonTypeComboInput/Normal");

        client_state.map_editor_state.selected_polygon_vertices.clear();
        client_state.map_editor_state.selected_scenery_ids = { 0U };
        context->Yield();
        IM_CHECK(context->ItemExists("Selected 1 sceneries"));
        context->ComboClick("##SceneryLevelComboInput/Front");

        client_state.map_editor_state.selected_scenery_ids.clear();
        client_state.map_editor_state.selected_spawn_point_ids = { 0U };
        context->Yield();
        IM_CHECK(context->ItemExists("Selected 1 spawn points"));
        context->ComboClick("##SpawnPointTypeComboInput/Bravo Team");
        context->ItemClick("Spawn player here");
    };

    RunTest(test);
}

TEST_F(ImGuiInterfaceTest, DebugRecordingCoversSoldierAndWeaponControls)
{
    const auto& soldier = state_manager_.CreateSoldier(1U);
    client_state_.client_soldier_id = soldier.id;
    state_manager_.TransformSoldier(soldier.id, [](auto& value) {
        value.active = true;
        value.on_ground = true;
    });
    client_state_.debug_render.is_game_debug_interface_enabled = true;

    ImGuiTest* test = IM_REGISTER_TEST(engine_, "debug_interface", "recording_and_weapons");
    test->UserData = this;
    test->GuiFunc = RenderDebugUI;
    test->TestFunc = [](ImGuiTestContext* context) {
        ImGuiInterfaceTest& fixture = GetFixture(context);
        ClientState& client_state = fixture.GetClientState();
        StateManager& state_manager = fixture.GetStateManager();

        context->SetRef("Debug window");
        context->ItemCheck("Draw soldier hitboxes");
        context->ItemCheck("Draw bullet hitboxes");
        context->ItemCheck("Draw item hitboxes");
        context->ItemCheck("Draw collision sectors");
        context->ItemCheck("Draw map boundaries");

        context->SetRef("Network window (Works only when connected to a server)");
        context->ItemCheck("Draw server POV client position");

        context->SetRef("Weapons");
        context->ItemClick("HK MP5");
        context->ItemClick("Combat Knife");

        context->SetRef("Animation Logs");
        context->ItemClick("Start Recording");
        state_manager.SoldierControlApply(*client_state.client_soldier_id,
                                          [](const auto&, auto& control) {
                                              control.left = true;
                                              control.right = true;
                                              control.up = true;
                                              control.down = true;
                                              control.fire = true;
                                              control.jets = true;
                                              control.change = true;
                                              control.throw_grenade = true;
                                              control.drop = true;
                                              control.reload = true;
                                              control.prone = true;
                                              control.flag_throw = true;
                                          });
        state_manager.SetGameTick(10U);
        context->Yield();
        context->ItemClick("Stop Recording");
        IM_CHECK(ImGui::FindWindowByName("Actions Log") != nullptr);

        client_state.map_editor_state.is_play_test_escape_menu_open = true;
        context->Yield();
        context->SetRef("Main menu");
        context->ItemClick("Exit game");
    };

    RunTest(test);
}
} // namespace
