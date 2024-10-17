#include "rendering/renderer/interface/map_editor/MapEditorUI.hpp"

#include "core/map/PMSEnums.hpp"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

namespace Soldank::MapEditorUI
{
void Render(State& game_state, ClientState& client_state)
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(client_state.mouse.x, client_state.mouse.y);
    io.MouseDrawCursor = io.WantCaptureMouse;
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    client_state.map_editor_state.is_mouse_hovering_over_ui = io.WantCaptureMouse;

    {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Save", "CTRL+S")) {
                    game_state.map.SaveMap("maps/" + game_state.map.GetName());
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit")) {
                if (ImGui::MenuItem("Undo", "CTRL+Z")) {
                    client_state.map_editor_state.event_pressed_undo.Notify();
                }
                if (ImGui::MenuItem("Redo", "CTRL+SHIFT+Z")) {
                    client_state.map_editor_state.event_pressed_redo.Notify();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Run")) {
                if (ImGui::MenuItem("Play", "F5")) {
                    client_state.map_editor_state.event_pressed_play.Notify();
                }
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
                if (ImGui::MenuItem(
                      "Tools", "", client_state.map_editor_state.is_tools_window_visible)) {
                    client_state.map_editor_state.is_tools_window_visible =
                      !client_state.map_editor_state.is_tools_window_visible;
                }
                if (ImGui::MenuItem("Properties",
                                    "",
                                    client_state.map_editor_state.is_properties_window_visible)) {
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
    }

    if (client_state.map_editor_state.is_tools_window_visible) {
        ImGui::Begin("Tools");

        static std::vector<std::pair<std::string, ToolType>> tool_options = {
            { "Transform tool (A)", ToolType::Transform },
            { "Polygon tool (Q)", ToolType::Polygon },
            { "Vertex selection tool (S)", ToolType::VertexSelection },
            { "Selection tool (W)", ToolType::Selection },
            { "Vertex color tool (D)", ToolType::VertexColor },
            { "Color tool (E)", ToolType::Color },
            { "Texture tool (F)", ToolType::Texture },
            { "Scenery tool (R)", ToolType::Scenery },
            { "Waypoint tool (G)", ToolType::Waypoint },
            { "Spawnpoint tool (T)", ToolType::Spawnpoint },
            { "Color picker tool (H)", ToolType::ColorPicker },
        };

        for (const auto& tool_option : tool_options) {
            if (ImGui::Selectable(tool_option.first.c_str(),
                                  client_state.map_editor_state.selected_tool ==
                                    tool_option.second)) {
                if (client_state.map_editor_state.selected_tool != tool_option.second) {
                    client_state.map_editor_state.event_selected_new_tool.Notify(
                      tool_option.second);
                }
                client_state.map_editor_state.selected_tool = tool_option.second;
            }
        }

        ImGui::End();
    }

    if (client_state.map_editor_state.is_properties_window_visible) {
        ImGui::Begin("Properties");
        ImGui::Text("Polygons: %zu", game_state.map.GetPolygons().size());
        ImGui::Text("Sceneries: %zu/500", game_state.map.GetSceneryInstances().size());
        ImGui::Text("Spawns: %zu/128", game_state.map.GetSpawnPoints().size());
        ImGui::Text("Colliders: %zu/128", game_state.map.GetColliders().size());
        ImGui::Text("Waypoints: %zu/500", game_state.map.GetWayPoints().size());
        ImGui::Text("Connections: 0"); // TODO: Add connections of waypoints
        glm::vec2 map_dimensions;
        map_dimensions.x = game_state.map.GetBoundaries()[Map::MapBoundary::RightBoundary] -
                           game_state.map.GetBoundaries()[Map::MapBoundary::LeftBoundary];
        map_dimensions.y = game_state.map.GetBoundaries()[Map::MapBoundary::BottomBoundary] -
                           game_state.map.GetBoundaries()[Map::MapBoundary::TopBoundary];
        ImGui::Text("Dimensions: %.0fx%.0f", map_dimensions.x, map_dimensions.y);
        ImGui::End();
    }

    if (client_state.map_editor_state.is_display_window_visible) {
        static bool test = false;
        ImGui::Begin("Display");

        ImGui::Checkbox("Background", &client_state.draw_background);
        ImGui::Checkbox("Polygons", &client_state.draw_polygons);
        ImGui::Checkbox("Sceneries", &client_state.draw_sceneries);
        ImGui::Checkbox("Spawn points", &client_state.map_editor_state.draw_spawn_points);
        ImGui::Checkbox("Wireframe", &test);

        ImGui::End();
    }

    if (client_state.map_editor_state.is_palette_window_visible) {
        float palette_width = 204.0F;
        int table_column_count = 12;

        ImGui::Begin("Palette",
                     nullptr,
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize |
                       ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollWithMouse |
                       ImGuiWindowFlags_NoCollapse);

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

            for (int i = 0;
                 auto& saved_color : client_state.map_editor_state.palette_saved_colors) {

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
                                         ImGuiColorEditFlags_NoDragDrop |
                                         ImGuiColorEditFlags_NoBorder,
                                       size)) {
                    client_state.map_editor_state.palette_current_color.at(0) = im_color.x;
                    client_state.map_editor_state.palette_current_color.at(1) = im_color.y;
                    client_state.map_editor_state.palette_current_color.at(2) = im_color.z;
                    client_state.map_editor_state.palette_current_color.at(3) = im_color.w;
                }

                if (ImGui::IsItemHovered()) {
                    if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
                        saved_color.x = client_state.map_editor_state.palette_current_color.at(0);
                        saved_color.y = client_state.map_editor_state.palette_current_color.at(1);
                        saved_color.z = client_state.map_editor_state.palette_current_color.at(2);
                        saved_color.w = client_state.map_editor_state.palette_current_color.at(3);
                    }
                }
                ++i;
            }
            ImGui::EndTable();
        }
        ImGui::PopStyleVar();
        ImGui::End();
    }

    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();

        // Set position to the top of the viewport and move it below the main menu bar
        ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + ImGui::GetFrameHeight()));

        // Extend width to viewport width
        ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, ImGui::GetFrameHeight()));

        // Add menu bar flag and disable everything else
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoScrollWithMouse |
                                 ImGuiWindowFlags_NoSavedSettings |
                                 ImGuiWindowFlags_NoBringToFrontOnFocus |
                                 ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_MenuBar;

        if (ImGui::Begin("MapTabBar", nullptr, flags)) {
            if (ImGui::BeginMenuBar()) {
                if (ImGui::BeginTabBar("#asd")) {
                    if (ImGui::BeginTabItem(game_state.map.GetName().c_str(),
                                            nullptr,
                                            ImGuiTabItemFlags_SetSelected /*|
                                              ImGuiTabItemFlags_UnsavedDocument*/)) {
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

    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();

        // Set position to the bottom of the viewport
        ImGui::SetNextWindowPos(
          ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - ImGui::GetFrameHeight()));

        // Extend width to viewport width
        ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, ImGui::GetFrameHeight()));

        // Add menu bar flag and disable everything else
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoScrollWithMouse |
                                 ImGuiWindowFlags_NoSavedSettings |
                                 ImGuiWindowFlags_NoBringToFrontOnFocus |
                                 ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_MenuBar;

        if (ImGui::Begin("StatusBar", nullptr, flags)) {
            if (ImGui::BeginMenuBar()) {
                ImGui::Text("%s", game_state.map.GetName().c_str());
                ImGui::SameLine(0, viewport->Size.x / 10.0F);
                ImGui::Text("Zoom: %.0f%%", client_state.camera_component.GetZoom() * 100.0F);
                ImGui::SameLine(0, viewport->Size.x / 10.0F);
                ImGui::Text("Current tool/action description");
                ImGui::SameLine(0, viewport->Size.x / 10.0F);
                ImGui::Text("Mouse position: %.0f, %.0f",
                            client_state.mouse_map_position.x,
                            client_state.mouse_map_position.y);
                ImGui::EndMenuBar();
            }
            ImGui::End();
        }
    }

    {
        if (client_state.map_editor_state.should_open_spawn_point_type_popup) {
            ImGui::OpenPopup("SpawnPointPopupMenu");
            client_state.map_editor_state.should_open_spawn_point_type_popup = false;
        }
        if (ImGui::BeginPopup("SpawnPointPopupMenu", ImGuiWindowFlags_NoMove)) {
            static std::vector<std::pair<std::string, PMSSpawnPointType>> spawn_point_options = {
                { "Player Spawn", PMSSpawnPointType::General },
                { "Alpha Team", PMSSpawnPointType::Alpha },
                { "Bravo Team", PMSSpawnPointType::Bravo },
                { "Charlie Team", PMSSpawnPointType::Charlie },
                { "Delta Team", PMSSpawnPointType::Delta },
                { "Alpha Flag", PMSSpawnPointType::AlphaFlag },
                { "Bravo Flag", PMSSpawnPointType::BravoFlag },
                { "Pointmatch Flag", PMSSpawnPointType::YellowFlag },
                { "Grenade Kit", PMSSpawnPointType::Grenades },
                { "Medical Kit", PMSSpawnPointType::Medkits },
                { "Cluster Grenade Kit", PMSSpawnPointType::Clusters },
                { "Vest Kit", PMSSpawnPointType::Vest },
                { "Flamer Kit", PMSSpawnPointType::Flamer },
                { "Berserker Kit", PMSSpawnPointType::Berserker },
                { "Predator Kit", PMSSpawnPointType::Predator },
                { "Rambo Bow", PMSSpawnPointType::RamboBow },
                { "Stationary Gun", PMSSpawnPointType::StatGun },
            };

            for (const auto& spawn_point_option : spawn_point_options) {
                if (ImGui::Selectable(spawn_point_option.first.c_str(),
                                      client_state.map_editor_state.selected_spawn_point_type ==
                                        spawn_point_option.second)) {
                    client_state.map_editor_state.selected_spawn_point_type =
                      spawn_point_option.second;
                }
            }
            ImGui::EndPopup();
        }
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
} // namespace Soldank::MapEditorUI
