#include "rendering/renderer/interface/map_editor/MapEditorUI.hpp"

#include "rendering/renderer/interface/map_editor/MapEditorToolDetailsWindow.hpp"

#include "core/map/PMSConstants.hpp"
#include "core/map/PMSEnums.hpp"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <filesystem>
#include <string>

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
    client_state.map_editor_state.is_modal_or_popup_open = false;

    {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Save", "CTRL+S")) {
                    game_state.map.SaveMap("maps/" + game_state.map.GetName());
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit")) {
                if (ImGui::MenuItem(
                      "Undo", "CTRL+Z", false, client_state.map_editor_state.is_undo_enabled)) {
                    client_state.map_editor_state.event_pressed_undo.Notify();
                }
                if (ImGui::MenuItem("Redo",
                                    "CTRL+SHIFT+Z",
                                    false,
                                    client_state.map_editor_state.is_redo_enabled)) {
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
                ImGui::MenuItem("Snap to vertices",
                                nullptr,
                                &client_state.map_editor_state.is_snap_to_vertices_enabled);
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

        static std::array<char, TEXTURE_NAME_MAX_LENGTH> texture_search_filter;

        if (client_state.map_editor_state.should_open_map_settings_modal) {
            client_state.map_editor_state.should_open_map_settings_modal = false;
            std::filesystem::path textures_directory_path = "textures";
            client_state.map_editor_state.all_textures_in_directory.clear();
            for (const auto& entry : std::filesystem::directory_iterator(textures_directory_path)) {
                if (entry.is_directory()) {
                    continue;
                }

                if (!entry.path().has_extension()) {
                    continue;
                }

                if (entry.path().extension() == ".bmp" || entry.path().extension() == ".jpg" ||
                    entry.path().extension() == ".jpeg" || entry.path().extension() == ".gif" ||
                    entry.path().extension() == ".png") {
                    client_state.map_editor_state.all_textures_in_directory.push_back(
                      entry.path().filename().string());
                }
            }

            texture_search_filter.fill(0);

            ImGui::OpenPopup("Map settings");
        }

        if (ImGui::BeginPopupModal("Map settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            client_state.map_editor_state.is_modal_or_popup_open = true;

            static std::string drag_int_tooltip =
              "Drag left or right to change.\n\nDouble-click to input text manually.";
            client_state.map_editor_state.map_description_input = game_state.map.GetDescription();
            client_state.map_editor_state.map_description_input += (char)0;
            ImGui::SeparatorText("Description");
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::InputText("##DescriptionInput",
                                 client_state.map_editor_state.map_description_input.data(),
                                 DESCRIPTION_MAX_LENGTH)) {
                game_state.map.SetDescription(client_state.map_editor_state.map_description_input);
            }

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::BeginTable("weather_and_step_table", 2, ImGuiTableFlags_SizingStretchSame)) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::SeparatorText("Weather type");
                ImGui::TableSetColumnIndex(1);
                ImGui::SeparatorText("Step type");
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                static std::vector<std::pair<std::string, PMSWeatherType>> weather_options = {
                    { "None", PMSWeatherType::None },
                    { "Rain", PMSWeatherType::Rain },
                    { "Sandstorm", PMSWeatherType::Sandstorm },
                    { "Snow", PMSWeatherType::Snow },
                };
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (ImGui::BeginCombo("##WeatherInput",
                                      game_state.map.GetWeatherTypeText().c_str())) {
                    for (const auto& weather_option : weather_options) {
                        if (ImGui::Selectable(weather_option.first.c_str(),
                                              game_state.map.GetWeatherType() ==
                                                weather_option.second)) {
                            game_state.map.SetWeatherType(weather_option.second);
                        }
                    }
                    ImGui::EndCombo();
                }

                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                static std::vector<std::pair<std::string, PMSStepType>> step_options = {
                    { "Hard", PMSStepType::HardGround },
                    { "Soft", PMSStepType::SoftGround },
                    { "None", PMSStepType::None },
                };
                // ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (ImGui::BeginCombo("##StepsInput", game_state.map.GetStepTypeText().c_str())) {
                    for (const auto& step_option : step_options) {
                        if (ImGui::Selectable(step_option.first.c_str(),
                                              game_state.map.GetStepType() == step_option.second)) {
                            game_state.map.SetStepType(step_option.second);
                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::EndTable();
            }

            ImGui::Separator();
            float drag_int_width = ImGui::CalcTextSize("Medical kits: 99").x + 50.0F;
            int grenades_count = game_state.map.GetGrenadesCount();
            ImGui::SetNextItemWidth(drag_int_width);
            if (ImGui::DragInt("##GrenadesInput", &grenades_count, 0.1F, 0, 12, "Grenades: %d")) {
                if (grenades_count < 0) {
                    grenades_count = 0;
                }
                if (grenades_count > 12) {
                    grenades_count = 12;
                }
                game_state.map.SetGrenadesCount(grenades_count);
            }
            ImGui::SetItemTooltip("%s", drag_int_tooltip.c_str());

            ImGui::SameLine();

            int medikits_count = game_state.map.GetMedikitsCount();
            ImGui::SetNextItemWidth(drag_int_width);
            if (ImGui::DragInt(
                  "##MedikitsInput", &medikits_count, 0.1F, 0, 12, "Medical kits: %d")) {
                if (medikits_count < 0) {
                    medikits_count = 0;
                }
                if (medikits_count > 12) {
                    medikits_count = 12;
                }
                game_state.map.SetMedikitsCount(medikits_count);
            }
            ImGui::SetItemTooltip("%s", drag_int_tooltip.c_str());

            ImVec2 jet_fuel_buttons_size(ImGui::GetContentRegionAvail().x / 4.0F - 9.0F, 0);
            ImGui::SeparatorText("Jet fuel:");
            if (ImGui::Button("None##JetFuelInputNone", jet_fuel_buttons_size)) {
                game_state.map.SetJetCount(0);
            }
            ImGui::SetItemTooltip("Value: 0");
            ImGui::SameLine();
            if (ImGui::Button("Minimal##JetFuelInputMinimal", jet_fuel_buttons_size)) {
                game_state.map.SetJetCount(12);
            }
            ImGui::SetItemTooltip("Value: 12");
            ImGui::SameLine();
            if (ImGui::Button("Very Low##JetFuelInputVeryLow", jet_fuel_buttons_size)) {
                game_state.map.SetJetCount(45);
            }
            ImGui::SetItemTooltip("Value: 45");
            ImGui::SameLine();
            if (ImGui::Button("Low##JetFuelInputLow", jet_fuel_buttons_size)) {
                game_state.map.SetJetCount(95);
            }

            // new line
            ImGui::SetItemTooltip("Value: 95");
            if (ImGui::Button("Normal##JetFuelInputNormal", jet_fuel_buttons_size)) {
                game_state.map.SetJetCount(190);
            }
            ImGui::SetItemTooltip("Value: 190");
            ImGui::SameLine();
            if (ImGui::Button("High##JetFuelInputHigh", jet_fuel_buttons_size)) {
                game_state.map.SetJetCount(320);
            }
            ImGui::SetItemTooltip("Value: 320");
            ImGui::SameLine();
            if (ImGui::Button("Extreme##JetFuelInputExtreme", jet_fuel_buttons_size)) {
                game_state.map.SetJetCount(800);
            }
            ImGui::SetItemTooltip("Value: 800");
            ImGui::SameLine();
            if (ImGui::Button("Infinite##JetFuelInputInfinite", jet_fuel_buttons_size)) {
                game_state.map.SetJetCount(25999);
            }
            ImGui::SetItemTooltip("Value: 25999");
            int jet_count = game_state.map.GetJetCount();
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::DragInt("##JetFuelCustomInput", &jet_count, 1.0F, 0, 25999)) {
                if (jet_count < 0) {
                    jet_count = 0;
                }
                if (jet_count > 25999) {
                    jet_count = 25999;
                }
                game_state.map.SetJetCount(jet_count);
            }
            ImGui::SetItemTooltip("%s", drag_int_tooltip.c_str());

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::BeginTable(
                  "texture_and_background_table", 2, ImGuiTableFlags_SizingStretchSame)) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::SeparatorText("Background");
                ImGui::TableSetColumnIndex(1);
                ImGui::SeparatorText("Texture");

                ImGui::TableNextRow();

                ImVec2 color_button_size(ImGui::GetContentRegionAvail().x,
                                         ImGui::GetContentRegionAvail().x / 2.0F + 15.0F);

                ImGui::TableSetColumnIndex(0);
                PMSColor top_background_color = game_state.map.GetBackgroundTopColor();
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
                                            ImGuiColorEditFlags_NoLabel |
                                              ImGuiColorEditFlags_NoAlpha)) {
                        top_background_color.red =
                          (unsigned char)(im_top_background_color.x * 255.0F);
                        top_background_color.green =
                          (unsigned char)(im_top_background_color.y * 255.0F);
                        top_background_color.blue =
                          (unsigned char)(im_top_background_color.z * 255.0F);
                        top_background_color.alpha =
                          (unsigned char)(im_top_background_color.w * 255.0F);
                        game_state.map.SetBackgroundTopColor(top_background_color);
                    }
                    ImGui::EndPopup();
                }

                PMSColor bottom_background_color = game_state.map.GetBackgroundBottomColor();
                ImVec4 im_bottom_background_color =
                  ImVec4((float)bottom_background_color.red / 255.0F,
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
                                            ImGuiColorEditFlags_NoLabel |
                                              ImGuiColorEditFlags_NoAlpha)) {
                        bottom_background_color.red =
                          (unsigned char)(im_bottom_background_color.x * 255.0F);
                        bottom_background_color.green =
                          (unsigned char)(im_bottom_background_color.y * 255.0F);
                        bottom_background_color.blue =
                          (unsigned char)(im_bottom_background_color.z * 255.0F);
                        bottom_background_color.alpha =
                          (unsigned char)(im_bottom_background_color.w * 255.0F);
                        game_state.map.SetBackgroundBottomColor(bottom_background_color);
                    }
                    ImGui::EndPopup();
                }

                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (ImGui::BeginCombo("##TextureComboPicker",
                                      game_state.map.GetTextureName().c_str())) {

                    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
                        !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)) {
                        ImGui::SetKeyboardFocusHere(0);
                    }
                    ImGui::InputText("##TextureSearchFilterInput",
                                     texture_search_filter.data(),
                                     texture_search_filter.size());
                    std::string search_filter(texture_search_filter.begin(),
                                              texture_search_filter.end());
                    std::erase(search_filter, 0);
                    ImGui::Separator();

                    std::filesystem::path map_texture_in_png = game_state.map.GetTextureName();
                    map_texture_in_png.replace_extension(".png");
                    for (const auto& texture_file_name :
                         client_state.map_editor_state.all_textures_in_directory) {

                        if (!search_filter.empty() &&
                            texture_file_name.find(search_filter) == std::string::npos) {
                            continue;
                        }

                        if (ImGui::Selectable(texture_file_name.c_str(),
                                              texture_file_name ==
                                                  game_state.map.GetTextureName() ||
                                                texture_file_name == map_texture_in_png.string())) {

                            game_state.map.SetTextureName(texture_file_name);
                        }
                    }
                    ImGui::EndCombo();
                }

                ImGui::Image(
                  (ImTextureID)(intptr_t)client_state.map_editor_state.polygon_texture_opengl_id,
                  { ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().x });

                ImGui::EndTable();
            }

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
        ImGui::Checkbox("Grid", &client_state.map_editor_state.is_grid_visible);

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

    if (client_state.map_editor_state.is_tool_details_window_visible) {
        MapEditorToolDetailsWindow::Render(game_state, client_state);
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

        if (ImGui::Begin("MapTabBarWindow", nullptr, flags)) {
            if (ImGui::BeginMenuBar()) {
                if (ImGui::BeginTabBar("#MapTabBar")) {
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

    {
        if (client_state.map_editor_state.should_open_spawn_point_type_popup) {
            ImGui::OpenPopup("SpawnPointPopupMenu");
            client_state.map_editor_state.should_open_spawn_point_type_popup = false;
        }
        if (ImGui::BeginPopup("SpawnPointPopupMenu", ImGuiWindowFlags_NoMove)) {
            client_state.map_editor_state.is_modal_or_popup_open = true;

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

    {
        if (client_state.map_editor_state.should_open_polygon_type_popup) {
            ImGui::OpenPopup("PolygonTypePopupMenu");
            client_state.map_editor_state.should_open_polygon_type_popup = false;
        }

        if (ImGui::BeginPopup("PolygonTypePopupMenu", ImGuiWindowFlags_NoMove)) {
            client_state.map_editor_state.is_modal_or_popup_open = true;

            static std::vector<std::pair<std::string, PMSPolygonType>> polygon_type_options = {
                { "Normal", PMSPolygonType::Normal },
                { "All Bullets Collide", PMSPolygonType::OnlyBulletsCollide },
                { "All Players Collide", PMSPolygonType::OnlyPlayersCollide },
                { "Doesn't Collide", PMSPolygonType::NoCollide },
                { "Icy", PMSPolygonType::Ice },
                { "Deadly", PMSPolygonType::Deadly },
                { "Bloody Deadly", PMSPolygonType::BloodyDeadly },
                { "Hurting", PMSPolygonType::Hurts },
                { "Regenerative", PMSPolygonType::Regenerates },
                { "Lava", PMSPolygonType::Lava },
                { "Only Alpha Bullets Collide", PMSPolygonType::AlphaBullets },
                { "Only Alpha Players Collide", PMSPolygonType::AlphaPlayers },
                { "Only Bravo Bullets Collide", PMSPolygonType::BravoBullets },
                { "Only Bravo Players Collide", PMSPolygonType::BravoPlayers },
                { "Only Charlie Bullets Collide", PMSPolygonType::CharlieBullets },
                { "Only Charlie Players Collide", PMSPolygonType::CharliePlayers },
                { "Only Delta Bullets Collide", PMSPolygonType::DeltaBullets },
                { "Only Delta Players Collide", PMSPolygonType::DeltaPlayers },
                { "Bouncy", PMSPolygonType::Bouncy },
                { "Explosive", PMSPolygonType::Explosive },
                { "Hurting Flaggers", PMSPolygonType::HurtFlaggers },
                { "Only Flaggers Collide", PMSPolygonType::FlaggerCollides },
                { "Only Non Flaggers Collide", PMSPolygonType::NonFlaggerCollides },
                { "Only Flag Collide", PMSPolygonType::FlagCollides },
            };

            for (const auto& polygon_type_option : polygon_type_options) {
                if (ImGui::Selectable(polygon_type_option.first.c_str(),
                                      client_state.map_editor_state.polygon_tool_polygon_type ==
                                        polygon_type_option.second)) {
                    client_state.map_editor_state.polygon_tool_polygon_type =
                      polygon_type_option.second;
                }
            }
            ImGui::EndPopup();
        }
    }

    {
        static std::array<char, SCENERY_NAME_MAX_LENGTH> scenery_search_filter;

        if (client_state.map_editor_state.should_open_scenery_picker_popup) {
            std::filesystem::path sceneries_directory_path = "scenery-gfx";
            client_state.map_editor_state.all_sceneries_in_directory.clear();
            for (const auto& entry :
                 std::filesystem::directory_iterator(sceneries_directory_path)) {
                if (entry.is_directory()) {
                    continue;
                }

                if (!entry.path().has_extension()) {
                    continue;
                }

                if (entry.path().extension() == ".bmp" || entry.path().extension() == ".jpg" ||
                    entry.path().extension() == ".jpeg" || entry.path().extension() == ".gif" ||
                    entry.path().extension() == ".png") {
                    client_state.map_editor_state.all_sceneries_in_directory.push_back(
                      entry.path().filename().string());
                }
            }

            if (client_state.map_editor_state.selected_scenery_to_place.empty() &&
                !client_state.map_editor_state.all_sceneries_in_directory.empty()) {

                client_state.map_editor_state.selected_scenery_to_place =
                  client_state.map_editor_state.all_sceneries_in_directory.front();
            }

            client_state.map_editor_state.should_open_scenery_picker_popup = false;

            scenery_search_filter.fill(0);
            ImGui::OpenPopup("SceneryPickerPopupMenu");
        }

        if (ImGui::BeginPopup("SceneryPickerPopupMenu")) {
            client_state.map_editor_state.is_modal_or_popup_open = true;

            ImGui::SeparatorText("Search:");
            if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
                !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)) {
                ImGui::SetKeyboardFocusHere(0);
            }
            ImGui::InputText(
              "##ScenerySearchInput", scenery_search_filter.data(), scenery_search_filter.size());
            std::string search_filter(scenery_search_filter.begin(), scenery_search_filter.end());
            std::erase(search_filter, 0);
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

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
} // namespace Soldank::MapEditorUI
