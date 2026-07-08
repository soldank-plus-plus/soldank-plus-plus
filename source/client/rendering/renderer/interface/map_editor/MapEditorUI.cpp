module;

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "core/math/Glm.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

export module MapEditorUI;

import ClientState;
import MapEditor.EditorAssetBrowser;
import MapEditor.EditorUiOptions;
import MapEditorToolDetailsWindow;
import MapEditorState;

import Shared.Core.State.StateManager;
import Shared.Core.Map.PMSConstants;
import Shared.Core.Map.PMSEnums;
import Shared.Core.Map.PMSStructs;
import Shared.Core.Map.Map;

export namespace Soldank::MapEditorUI
{
void Render(const StateManager& game_state_manager, ClientState& client_state);
}

namespace Soldank::MapEditorUI
{
ImGuiWindowFlags GetDefaultWindowFlags()
{
    return ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize |
           ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollWithMouse |
           ImGuiWindowFlags_NoCollapse;
}

void BeginFrame(ClientState& client_state)
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(client_state.mouse_screen_position.x, client_state.mouse_screen_position.y);
    io.MouseDrawCursor = io.WantCaptureMouse;
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    client_state.map_editor_state.is_mouse_hovering_over_ui = io.WantCaptureMouse;
    client_state.map_editor_state.is_modal_or_popup_open = false;
}

void EndFrame()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void RenderMainMenuBar(const StateManager& game_state_manager, ClientState& client_state)
{
    if (!ImGui::BeginMainMenuBar()) {
        return;
    }

    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Save", "CTRL+S")) {
            if (game_state_manager.GetConstMap().GetName()) {
                client_state.map_editor_state.event_save_map.Notify(
                  "maps/" + *game_state_manager.GetConstMap().GetName());
                client_state.map_editor_state.is_map_changed = false;
            } else {
                client_state.map_editor_state.should_open_save_as_modal = true;
            }
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Edit")) {
        if (ImGui::MenuItem(
              "Undo", "CTRL+Z", false, client_state.map_editor_state.is_undo_enabled)) {
            client_state.map_editor_state.event_pressed_undo.Notify();
        }
        if (ImGui::MenuItem(
              "Redo", "CTRL+SHIFT+Z", false, client_state.map_editor_state.is_redo_enabled)) {
            client_state.map_editor_state.event_pressed_redo.Notify();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Map settings...", "CTRL+M")) {
            client_state.map_editor_state.should_open_map_settings_modal = true;
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Run")) {
        if (ImGui::MenuItem("Play", "F5")) {
            client_state.map_editor_state.event_pressed_play.Notify();
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View")) {
        ImGui::MenuItem(
          "Snap to grid", nullptr, &client_state.map_editor_state.is_snap_to_grid_enabled);
        ImGui::MenuItem(
          "Snap to vertices", nullptr, &client_state.map_editor_state.is_snap_to_vertices_enabled);
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Windows")) {
        if (ImGui::MenuItem("Show all")) {
            client_state.map_editor_state.is_tools_window_visible = true;
            client_state.map_editor_state.is_properties_window_visible = true;
            client_state.map_editor_state.is_display_window_visible = true;
            client_state.map_editor_state.is_palette_window_visible = true;
        }
        if (ImGui::MenuItem("Hide all")) {
            client_state.map_editor_state.is_tools_window_visible = false;
            client_state.map_editor_state.is_properties_window_visible = false;
            client_state.map_editor_state.is_display_window_visible = false;
            client_state.map_editor_state.is_palette_window_visible = false;
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Tools", "", client_state.map_editor_state.is_tools_window_visible)) {
            client_state.map_editor_state.is_tools_window_visible =
              !client_state.map_editor_state.is_tools_window_visible;
        }
        if (ImGui::MenuItem(
              "Properties", "", client_state.map_editor_state.is_properties_window_visible)) {
            client_state.map_editor_state.is_properties_window_visible =
              !client_state.map_editor_state.is_properties_window_visible;
        }
        if (ImGui::MenuItem(
              "Display", "", client_state.map_editor_state.is_display_window_visible)) {
            client_state.map_editor_state.is_display_window_visible =
              !client_state.map_editor_state.is_display_window_visible;
        }
        if (ImGui::MenuItem(
              "Palette", "", client_state.map_editor_state.is_palette_window_visible)) {
            client_state.map_editor_state.is_palette_window_visible =
              !client_state.map_editor_state.is_palette_window_visible;
        }
        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
}

template<std::size_t Size>
void CopyToInputBuffer(std::array<char, Size>& input_buffer, std::string_view value)
{
    input_buffer.fill(0);
    const std::size_t copy_size = std::min(value.size(), input_buffer.size() - 1);
    std::ranges::copy_n(value.begin(), copy_size, input_buffer.begin());
}

template<std::size_t Size>
std::string ReadInputBuffer(const std::array<char, Size>& input_buffer)
{
    std::string value(input_buffer.begin(), input_buffer.end());
    std::erase(value, 0);
    return value;
}

void RenderSaveAsModal(const StateManager& game_state_manager, ClientState& client_state)
{
    if (client_state.map_editor_state.should_open_save_as_modal) {
        client_state.map_editor_state.should_open_save_as_modal = false;
        client_state.map_editor_state.save_as_map_name_input.fill(0);
        if (game_state_manager.GetConstMap().GetName()) {
            CopyToInputBuffer(client_state.map_editor_state.save_as_map_name_input,
                              *game_state_manager.GetConstMap().GetName());
        }
        ImGui::OpenPopup("Save as...");
    }

    if (ImGui::BeginPopupModal("Save as...", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        client_state.map_editor_state.is_modal_or_popup_open = true;

        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
            !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)) {
            ImGui::SetKeyboardFocusHere(0);
        }
        ImGui::InputText("##NewMapNameInput",
                         client_state.map_editor_state.save_as_map_name_input.data(),
                         MAP_NAME_MAX_LENGTH);

        float cancel_button_width =
          ImGui::CalcTextSize("Cancel").x + ImGui::GetStyle().FramePadding.x * 2.F;
        float save_button_width =
          ImGui::CalcTextSize("Save").x + ImGui::GetStyle().FramePadding.x * 2.F;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x -
                             cancel_button_width - save_button_width -
                             ImGui::GetStyle().ItemSpacing.x);
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Save")) {
            std::string new_map_name =
              ReadInputBuffer(client_state.map_editor_state.save_as_map_name_input);
            new_map_name += ".pms";
            client_state.map_editor_state.event_set_map_name.Notify(new_map_name);
            client_state.map_editor_state.event_save_map.Notify("maps/" + new_map_name);
            client_state.map_editor_state.is_map_changed = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void RenderToolsWindow(ClientState& client_state, ImGuiWindowFlags default_window_flags)
{
    if (!client_state.map_editor_state.is_tools_window_visible) {
        return;
    }

    ImGui::Begin("Tools", nullptr, default_window_flags);

    for (const auto& tool_option : EditorUiOptions::GetToolOptions()) {
        if (std::ranges::contains(EditorUiOptions::GetDisabledToolTypes(), tool_option.second)) {
            ImGui::Selectable(tool_option.first.data(),
                              client_state.map_editor_state.selected_tool == tool_option.second,
                              ImGuiSelectableFlags_Disabled);
            ImGui::SetItemTooltip("This tool is not implemented yet. Coming soon!");
            continue;
        }

        if (ImGui::Selectable(tool_option.first.data(),
                              client_state.map_editor_state.selected_tool == tool_option.second)) {
            if (client_state.map_editor_state.selected_tool != tool_option.second) {
                client_state.map_editor_state.event_selected_new_tool.Notify(tool_option.second);
            }
            client_state.map_editor_state.selected_tool = tool_option.second;
        }
    }

    ImGui::End();
}

void RenderPropertiesWindow(const StateManager& game_state_manager,
                            ClientState& client_state,
                            ImGuiWindowFlags default_window_flags)
{
    if (!client_state.map_editor_state.is_properties_window_visible) {
        return;
    }

    ImGui::Begin("Properties", nullptr, default_window_flags);
    ImGui::Text("Polygons: %zu/%d",
                game_state_manager.GetConstMap().GetPolygons().size(),
                MAX_POLYGONS_COUNT);
    ImGui::Text("Sceneries: %zu/%d",
                game_state_manager.GetConstMap().GetSceneryInstances().size(),
                MAX_SCENERIES_COUNT);
    ImGui::Text("Spawns: %zu/%d",
                game_state_manager.GetConstMap().GetSpawnPoints().size(),
                MAX_SPAWN_POINTS_COUNT);
    ImGui::Text("Colliders: %zu/128", game_state_manager.GetConstMap().GetColliders().size());
    ImGui::Text("Waypoints: %zu/500", game_state_manager.GetConstMap().GetWayPoints().size());
    ImGui::Text("Connections: 0"); // TODO: Add connections of waypoints
    glm::vec2 map_dimensions;
    map_dimensions.x =
      game_state_manager.GetConstMap().GetBoundaries()[Map::MapBoundary::RightBoundary] -
      game_state_manager.GetConstMap().GetBoundaries()[Map::MapBoundary::LeftBoundary];
    map_dimensions.y =
      game_state_manager.GetConstMap().GetBoundaries()[Map::MapBoundary::BottomBoundary] -
      game_state_manager.GetConstMap().GetBoundaries()[Map::MapBoundary::TopBoundary];
    ImGui::Text("Dimensions: %.0fx%.0f", map_dimensions.x, map_dimensions.y);
    ImGui::End();
}

void RenderDisplayWindow(ClientState& client_state, ImGuiWindowFlags default_window_flags)
{
    if (!client_state.map_editor_state.is_display_window_visible) {
        return;
    }

    ImGui::Begin("Display", nullptr, default_window_flags);

    ImGui::Checkbox("Background", &client_state.draw_background);
    ImGui::Checkbox("Polygons", &client_state.draw_polygons);
    ImGui::Checkbox("Sceneries", &client_state.draw_sceneries);
    ImGui::Checkbox("Spawn points", &client_state.map_editor_state.draw_spawn_points);
    ImGui::Checkbox("Wireframe", &client_state.map_editor_state.draw_wireframe);
    ImGui::Checkbox("Grid", &client_state.map_editor_state.is_grid_visible);

    ImGui::End();
}

void RenderPaletteWindow(ClientState& client_state, ImGuiWindowFlags default_window_flags)
{
    if (!client_state.map_editor_state.is_palette_window_visible) {
        return;
    }

    float palette_width = 204.0F;
    int table_column_count = 12;

    ImGui::Begin("Palette", nullptr, default_window_flags);

    ImGui::SetNextItemWidth(palette_width);
    ImGui::ColorPicker4("##PaletteColorPicker",
                        client_state.map_editor_state.palette_current_color.data(),
                        ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_DisplayRGB |
                          ImGuiColorEditFlags_DisplayHex | ImGuiColorEditFlags_NoSidePreview);

    ImVec2 cell_padding(0.0F, 0.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, cell_padding);
    if (ImGui::BeginTable("PaletteSavedColorsTable",
                          table_column_count,
                          ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX |
                            ImGuiTableFlags_BordersOuter)) {

        for (int i = 0; auto& saved_color : client_state.map_editor_state.palette_saved_colors) {

            if (i % table_column_count == 0) {
                ImGui::TableNextRow(ImGuiTableRowFlags_None, 1.0F);
            }
            ImGui::TableSetColumnIndex(i % table_column_count);

            ImVec4 im_color{ saved_color.x, saved_color.y, saved_color.z, saved_color.w };

            std::string label = "PaletteColorButton";
            label += std::to_string(i);

            ImVec2 size(palette_width / (float)table_column_count,
                        palette_width / (float)table_column_count);

            if (ImGui::ColorButton(label.c_str(),
                                   im_color,
                                   ImGuiColorEditFlags_AlphaPreview |
                                     ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder,
                                   size)) {
                client_state.map_editor_state.palette_current_color.at(0) = im_color.x;
                client_state.map_editor_state.palette_current_color.at(1) = im_color.y;
                client_state.map_editor_state.palette_current_color.at(2) = im_color.z;
                client_state.map_editor_state.palette_current_color.at(3) = im_color.w;
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
                saved_color.x = client_state.map_editor_state.palette_current_color.at(0);
                saved_color.y = client_state.map_editor_state.palette_current_color.at(1);
                saved_color.z = client_state.map_editor_state.palette_current_color.at(2);
                saved_color.w = client_state.map_editor_state.palette_current_color.at(3);
            }
            ++i;
        }
        ImGui::EndTable();
    }
    ImGui::PopStyleVar();
    ImGui::End();
}

void RenderMapTabBar(const StateManager& game_state_manager, ClientState& client_state)
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + ImGui::GetFrameHeight()));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, ImGui::GetFrameHeight()));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoBringToFrontOnFocus |
                             ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_MenuBar;

    if (ImGui::Begin("MapTabBarWindow", nullptr, flags)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginTabBar("#MapTabBar")) {
                ImGuiTabItemFlags tab_flags = ImGuiTabItemFlags_SetSelected;
                if (client_state.map_editor_state.is_map_changed) {
                    tab_flags |= ImGuiTabItemFlags_UnsavedDocument;
                }
                if (ImGui::BeginTabItem(
                      game_state_manager.GetConstMap().GetName().value_or("Untitled").c_str(),
                      nullptr,
                      tab_flags)) {
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("+", nullptr)) {
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
            ImGui::EndMenuBar();
        }
        ImGui::End();
    }
}

void RenderStatusBar(const StateManager& game_state_manager, ClientState& client_state)
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(
      ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - ImGui::GetFrameHeight()));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, ImGui::GetFrameHeight()));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoBringToFrontOnFocus |
                             ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_MenuBar;

    if (ImGui::Begin("StatusBar", nullptr, flags)) {
        if (ImGui::BeginMenuBar()) {
            ImGui::Text("%s",
                        game_state_manager.GetConstMap().GetName().value_or("Untitled").c_str());
            ImGui::SameLine(0, viewport->Size.x / 10.0F);
            ImGui::Text("Zoom: %.0f%%",
                        // TODO: need to invert zoom on camera class level instead of this
                        (1.0F / client_state.camera.GetZoom()) * 100.0F);
            ImGui::SameLine(0, viewport->Size.x / 10.0F);
            ImGui::Text("%s",
                        client_state.map_editor_state.current_tool_action_description.c_str());
            ImGui::SameLine(0, viewport->Size.x / 10.0F);
            ImGui::Text("Mouse position: %.0f, %.0f",
                        client_state.mouse_map_position.x,
                        client_state.mouse_map_position.y);
            ImGui::EndMenuBar();
        }
        ImGui::End();
    }
}

void RenderSpawnPointPopup(ClientState& client_state)
{
    if (client_state.map_editor_state.should_open_spawn_point_type_popup) {
        ImGui::OpenPopup("SpawnPointPopupMenu");
        client_state.map_editor_state.should_open_spawn_point_type_popup = false;
    }
    if (ImGui::BeginPopup("SpawnPointPopupMenu", ImGuiWindowFlags_NoMove)) {
        client_state.map_editor_state.is_modal_or_popup_open = true;

        for (const auto& spawn_point_option : EditorUiOptions::GetSpawnPointOptions()) {
            if (ImGui::Selectable(spawn_point_option.first.data(),
                                  client_state.map_editor_state.selected_spawn_point_type ==
                                    spawn_point_option.second)) {
                client_state.map_editor_state.selected_spawn_point_type = spawn_point_option.second;
            }
        }
        ImGui::EndPopup();
    }
}

void RenderPolygonTypePopup(ClientState& client_state)
{
    if (client_state.map_editor_state.should_open_polygon_type_popup) {
        ImGui::OpenPopup("PolygonTypePopupMenu");
        client_state.map_editor_state.should_open_polygon_type_popup = false;
    }

    if (ImGui::BeginPopup("PolygonTypePopupMenu", ImGuiWindowFlags_NoMove)) {
        client_state.map_editor_state.is_modal_or_popup_open = true;

        for (const auto& polygon_type_option : EditorUiOptions::GetPolygonTypeOptions()) {
            if (ImGui::Selectable(polygon_type_option.first.data(),
                                  client_state.map_editor_state.polygon_tool_polygon_type ==
                                    polygon_type_option.second)) {
                client_state.map_editor_state.polygon_tool_polygon_type =
                  polygon_type_option.second;
            }
        }
        ImGui::EndPopup();
    }
}

void RenderSceneryPickerPopup(ClientState& client_state)
{
    if (client_state.map_editor_state.should_open_scenery_picker_popup) {
        client_state.map_editor_state.all_sceneries_in_directory =
          EditorAssetBrowser::LoadSceneryNames();

        if (client_state.map_editor_state.selected_scenery_to_place.empty() &&
            !client_state.map_editor_state.all_sceneries_in_directory.empty()) {

            client_state.map_editor_state.selected_scenery_to_place =
              client_state.map_editor_state.all_sceneries_in_directory.front();
        }

        client_state.map_editor_state.should_open_scenery_picker_popup = false;

        client_state.map_editor_state.scenery_search_filter.fill(0);
        ImGui::OpenPopup("SceneryPickerPopupMenu");
    }

    if (ImGui::BeginPopup("SceneryPickerPopupMenu")) {
        client_state.map_editor_state.is_modal_or_popup_open = true;

        ImGui::SeparatorText("Search:");
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
            !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)) {
            ImGui::SetKeyboardFocusHere(0);
        }
        ImGui::InputText("##ScenerySearchInput",
                         client_state.map_editor_state.scenery_search_filter.data(),
                         client_state.map_editor_state.scenery_search_filter.size());
        std::string search_filter =
          ReadInputBuffer(client_state.map_editor_state.scenery_search_filter);
        ImGui::Separator();
        for (const auto& scenery_file_name :
             client_state.map_editor_state.all_sceneries_in_directory) {

            if (!search_filter.empty() &&
                scenery_file_name.find(search_filter) == std::string::npos) {
                continue;
            }

            if (ImGui::Selectable(scenery_file_name.c_str(),
                                  client_state.map_editor_state.selected_scenery_to_place ==
                                    scenery_file_name)) {
                client_state.map_editor_state.selected_scenery_to_place = scenery_file_name;
                client_state.map_editor_state.event_scenery_texture_changed.Notify(
                  scenery_file_name);
            }
        }
        ImGui::EndPopup();
    }
}

void RenderMapSettingsWeatherAndStep(const StateManager& game_state_manager,
                                     ClientState& client_state)
{
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::BeginTable("weather_and_step_table", 2, ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::SeparatorText("Weather type");
        ImGui::TableSetColumnIndex(1);
        ImGui::SeparatorText("Step type");
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::BeginCombo("##WeatherInput",
                              game_state_manager.GetConstMap().GetWeatherTypeText().c_str())) {
            for (const auto& weather_option : EditorUiOptions::GetWeatherOptions()) {
                if (ImGui::Selectable(weather_option.first.data(),
                                      game_state_manager.GetConstMap().GetWeatherType() ==
                                        weather_option.second)) {
                    client_state.map_editor_state.event_set_map_weather_type.Notify(
                      weather_option.second);
                }
            }
            ImGui::EndCombo();
        }

        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::BeginCombo("##StepsInput",
                              game_state_manager.GetConstMap().GetStepTypeText().c_str())) {
            for (const auto& step_option : EditorUiOptions::GetStepOptions()) {
                if (ImGui::Selectable(step_option.first.data(),
                                      game_state_manager.GetConstMap().GetStepType() ==
                                        step_option.second)) {
                    client_state.map_editor_state.event_set_map_step_type.Notify(
                      step_option.second);
                }
            }
            ImGui::EndCombo();
        }
        ImGui::EndTable();
    }
}

void RenderMapSettingsKits(const StateManager& game_state_manager,
                           ClientState& client_state,
                           std::string_view drag_int_tooltip)
{
    ImGui::Separator();
    float drag_int_width = ImGui::CalcTextSize("Medical kits: 99").x + 50.0F;
    int grenades_count = game_state_manager.GetConstMap().GetGrenadesCount();
    ImGui::SetNextItemWidth(drag_int_width);
    if (ImGui::DragInt("##GrenadesInput", &grenades_count, 0.1F, 0, 12, "Grenades: %d")) {
        if (grenades_count < 0) {
            grenades_count = 0;
        }
        if (grenades_count > 12) {
            grenades_count = 12;
        }
        client_state.map_editor_state.event_set_map_grenades_count.Notify(grenades_count);
    }
    ImGui::SetItemTooltip("%s", drag_int_tooltip.data());

    ImGui::SameLine();

    int medikits_count = game_state_manager.GetConstMap().GetMedikitsCount();
    ImGui::SetNextItemWidth(drag_int_width);
    if (ImGui::DragInt("##MedikitsInput", &medikits_count, 0.1F, 0, 12, "Medical kits: %d")) {
        if (medikits_count < 0) {
            medikits_count = 0;
        }
        if (medikits_count > 12) {
            medikits_count = 12;
        }
        client_state.map_editor_state.event_set_map_medikits_count.Notify(medikits_count);
    }
    ImGui::SetItemTooltip("%s", drag_int_tooltip.data());
}

void RenderMapSettingsJetFuel(const StateManager& game_state_manager,
                              ClientState& client_state,
                              std::string_view drag_int_tooltip)
{
    ImVec2 jet_fuel_buttons_size(ImGui::GetContentRegionAvail().x / 4.0F - 9.0F, 0);
    ImGui::SeparatorText("Jet fuel:");

    int option_index = 0;
    for (const auto& jet_fuel_option : EditorUiOptions::GetJetFuelOptions()) {
        if (option_index > 0 && option_index % 4 != 0) {
            ImGui::SameLine();
        }

        std::string button_label =
          std::string(jet_fuel_option.label) + "##JetFuelInput" + std::string(jet_fuel_option.id);
        if (ImGui::Button(button_label.c_str(), jet_fuel_buttons_size)) {
            client_state.map_editor_state.event_set_map_jet_count.Notify(jet_fuel_option.value);
        }
        ImGui::SetItemTooltip("Value: %d", jet_fuel_option.value);
        ++option_index;
    }

    int jet_count = game_state_manager.GetConstMap().GetJetCount();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::DragInt("##JetFuelCustomInput", &jet_count, 1.0F, 0, 25999)) {
        if (jet_count < 0) {
            jet_count = 0;
        }
        if (jet_count > 25999) {
            jet_count = 25999;
        }
        client_state.map_editor_state.event_set_map_jet_count.Notify(jet_count);
    }
    ImGui::SetItemTooltip("%s", drag_int_tooltip.data());
}

void RenderMapSettingsTextureAndBackground(const StateManager& game_state_manager,
                                           ClientState& client_state)
{
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::BeginTable("texture_and_background_table", 2, ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::SeparatorText("Background");
        ImGui::TableSetColumnIndex(1);
        ImGui::SeparatorText("Texture");

        ImGui::TableNextRow();

        ImVec2 color_button_size(ImGui::GetContentRegionAvail().x,
                                 ImGui::GetContentRegionAvail().x / 2.0F + 15.0F);

        ImGui::TableSetColumnIndex(0);
        PMSColor top_background_color = game_state_manager.GetConstMap().GetBackgroundTopColor();
        ImVec4 im_top_background_color = ImVec4((float)top_background_color.red / 255.0F,
                                                (float)top_background_color.green / 255.0F,
                                                (float)top_background_color.blue / 255.0F,
                                                (float)top_background_color.alpha / 255.0F);
        if (ImGui::ColorButton("TopBackgroundColorButton",
                               im_top_background_color,
                               ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoAlpha,
                               color_button_size)) {
            ImGui::OpenPopup("TopBackgroundColorPicker");
        }
        if (ImGui::BeginPopup("TopBackgroundColorPicker")) {
            if (ImGui::ColorPicker4("##topcolorpicker1",
                                    &im_top_background_color.x,
                                    ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoAlpha)) {
                top_background_color.red = (unsigned char)(im_top_background_color.x * 255.0F);
                top_background_color.green = (unsigned char)(im_top_background_color.y * 255.0F);
                top_background_color.blue = (unsigned char)(im_top_background_color.z * 255.0F);
                top_background_color.alpha = (unsigned char)(im_top_background_color.w * 255.0F);
                client_state.map_editor_state.event_set_map_background_top_color.Notify(
                  top_background_color);
            }
            ImGui::EndPopup();
        }

        PMSColor bottom_background_color =
          game_state_manager.GetConstMap().GetBackgroundBottomColor();
        ImVec4 im_bottom_background_color = ImVec4((float)bottom_background_color.red / 255.0F,
                                                   (float)bottom_background_color.green / 255.0F,
                                                   (float)bottom_background_color.blue / 255.0F,
                                                   (float)bottom_background_color.alpha / 255.0F);
        if (ImGui::ColorButton("BottomBackgroundColorButton",
                               im_bottom_background_color,
                               ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoAlpha,
                               color_button_size)) {
            ImGui::OpenPopup("BottomBackgroundColorPicker");
        }
        if (ImGui::BeginPopup("BottomBackgroundColorPicker")) {
            if (ImGui::ColorPicker4("##bottomcolorpicker1",
                                    &im_bottom_background_color.x,
                                    ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoAlpha)) {
                bottom_background_color.red =
                  (unsigned char)(im_bottom_background_color.x * 255.0F);
                bottom_background_color.green =
                  (unsigned char)(im_bottom_background_color.y * 255.0F);
                bottom_background_color.blue =
                  (unsigned char)(im_bottom_background_color.z * 255.0F);
                bottom_background_color.alpha =
                  (unsigned char)(im_bottom_background_color.w * 255.0F);
                client_state.map_editor_state.event_set_map_background_bottom_color.Notify(
                  bottom_background_color);
            }
            ImGui::EndPopup();
        }

        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::BeginCombo("##TextureComboPicker",
                              game_state_manager.GetConstMap().GetTextureName().c_str())) {

            if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
                !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)) {
                ImGui::SetKeyboardFocusHere(0);
            }
            ImGui::InputText("##TextureSearchFilterInput",
                             client_state.map_editor_state.texture_search_filter.data(),
                             client_state.map_editor_state.texture_search_filter.size());
            std::string search_filter =
              ReadInputBuffer(client_state.map_editor_state.texture_search_filter);
            ImGui::Separator();

            std::filesystem::path map_texture_in_png =
              game_state_manager.GetConstMap().GetTextureName();
            map_texture_in_png.replace_extension(".png");
            for (const auto& texture_file_name :
                 client_state.map_editor_state.all_textures_in_directory) {

                if (!search_filter.empty() &&
                    texture_file_name.find(search_filter) == std::string::npos) {
                    continue;
                }

                if (ImGui::Selectable(texture_file_name.c_str(),
                                      texture_file_name ==
                                          game_state_manager.GetConstMap().GetTextureName() ||
                                        texture_file_name == map_texture_in_png.string())) {

                    client_state.map_editor_state.event_set_map_texture_name.Notify(
                      texture_file_name);
                }
            }
            ImGui::EndCombo();
        }

        ImGui::Image((ImTextureID)(intptr_t)client_state.map_editor_state.polygon_texture_opengl_id,
                     { ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().x });

        ImGui::EndTable();
    }
}

void RenderMapSettingsModal(const StateManager& game_state_manager, ClientState& client_state)
{
    if (client_state.map_editor_state.should_open_map_settings_modal) {
        client_state.map_editor_state.should_open_map_settings_modal = false;
        client_state.map_editor_state.all_textures_in_directory =
          EditorAssetBrowser::LoadTextureNames();

        client_state.map_editor_state.texture_search_filter.fill(0);

        ImGui::OpenPopup("Map settings");
    }

    if (ImGui::BeginPopupModal("Map settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        client_state.map_editor_state.is_modal_or_popup_open = true;

        constexpr std::string_view DRAG_INT_TOOLTIP =
          "Drag left or right to change.\n\nDouble-click to input text manually.";
        client_state.map_editor_state.map_description_input =
          game_state_manager.GetConstMap().GetDescription();
        client_state.map_editor_state.map_description_input += (char)0;
        ImGui::SeparatorText("Description");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::InputText("##DescriptionInput",
                             client_state.map_editor_state.map_description_input.data(),
                             DESCRIPTION_MAX_LENGTH)) {
            client_state.map_editor_state.event_set_map_description.Notify(
              client_state.map_editor_state.map_description_input);
        }

        RenderMapSettingsWeatherAndStep(game_state_manager, client_state);
        RenderMapSettingsKits(game_state_manager, client_state, DRAG_INT_TOOLTIP);
        RenderMapSettingsJetFuel(game_state_manager, client_state, DRAG_INT_TOOLTIP);
        RenderMapSettingsTextureAndBackground(game_state_manager, client_state);

        ImGui::Separator();

        float close_button_width =
          ImGui::CalcTextSize("CLOSE").x + ImGui::GetStyle().FramePadding.x * 2.F;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x -
                             close_button_width);
        if (ImGui::Button("CLOSE")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void Render(const StateManager& game_state_manager, ClientState& client_state)
{
    BeginFrame(client_state);

    const ImGuiWindowFlags default_window_flags = GetDefaultWindowFlags();
    {
        RenderMainMenuBar(game_state_manager, client_state);
        RenderSaveAsModal(game_state_manager, client_state);
        RenderMapSettingsModal(game_state_manager, client_state);
    }

    RenderToolsWindow(client_state, default_window_flags);
    RenderPropertiesWindow(game_state_manager, client_state, default_window_flags);
    RenderDisplayWindow(client_state, default_window_flags);
    RenderPaletteWindow(client_state, default_window_flags);

    if (client_state.map_editor_state.is_tool_details_window_visible) {
        MapEditorToolDetailsWindow::Render(game_state_manager, client_state);
    }

    RenderMapTabBar(game_state_manager, client_state);
    RenderStatusBar(game_state_manager, client_state);
    RenderSpawnPointPopup(client_state);
    RenderPolygonTypePopup(client_state);
    RenderSceneryPickerPopup(client_state);

    EndFrame();
}
} // namespace Soldank::MapEditorUI
