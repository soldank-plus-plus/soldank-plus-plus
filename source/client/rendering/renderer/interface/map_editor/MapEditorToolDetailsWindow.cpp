#include "rendering/renderer/interface/map_editor/MapEditorToolDetailsWindow.hpp"

#include "core/map/PMSEnums.hpp"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <numbers>

namespace Soldank::MapEditorToolDetailsWindow
{
enum class ToolDetailsSubWindowOpenType
{
    None = 0,
    Polygons,
    Sceneries,
    SpawnPoints
};

void RenderPolygonToolDetails(State& game_state, ClientState& client_state)
{
    unsigned short new_polygon_id = game_state.map.GetPolygonsCount() + 1;

    ImGui::Text("Placing polygon: %hu/5000", new_polygon_id);

    unsigned int placed_vertices_count = 0;
    glm::vec2 first_vertex_position;
    glm::vec2 second_vertex_position;
    glm::vec2 third_vertex_position;

    first_vertex_position = client_state.mouse_map_position;

    if (client_state.map_editor_state.polygon_tool_wip_polygon_edge) {
        placed_vertices_count = 1;
        first_vertex_position = {
            client_state.map_editor_state.polygon_tool_wip_polygon_edge->vertices.at(0).x,
            client_state.map_editor_state.polygon_tool_wip_polygon_edge->vertices.at(0).y
        };

        second_vertex_position = {
            client_state.map_editor_state.polygon_tool_wip_polygon_edge->vertices.at(1).x,
            client_state.map_editor_state.polygon_tool_wip_polygon_edge->vertices.at(1).y
        };
    }

    if (client_state.map_editor_state.polygon_tool_wip_polygon) {
        placed_vertices_count = 2;
        first_vertex_position = {
            client_state.map_editor_state.polygon_tool_wip_polygon->vertices.at(0).x,
            client_state.map_editor_state.polygon_tool_wip_polygon->vertices.at(0).y
        };

        second_vertex_position = {
            client_state.map_editor_state.polygon_tool_wip_polygon->vertices.at(1).x,
            client_state.map_editor_state.polygon_tool_wip_polygon->vertices.at(1).y
        };

        third_vertex_position = {
            client_state.map_editor_state.polygon_tool_wip_polygon->vertices.at(2).x,
            client_state.map_editor_state.polygon_tool_wip_polygon->vertices.at(2).y
        };
    }

    ImGui::SeparatorText("Type");
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
    std::string current_polygon_type;
    for (const auto& polygon_type_option : polygon_type_options) {
        if (client_state.map_editor_state.polygon_tool_polygon_type == polygon_type_option.second) {
            current_polygon_type = polygon_type_option.first;
        }
    }
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::BeginCombo("##PolygonTypeComboInput", current_polygon_type.c_str())) {
        client_state.map_editor_state.is_modal_or_popup_open = true;
        for (const auto& polygon_type_option : polygon_type_options) {
            if (ImGui::Selectable(polygon_type_option.first.c_str(),
                                  client_state.map_editor_state.polygon_tool_polygon_type ==
                                    polygon_type_option.second)) {
                client_state.map_editor_state.polygon_tool_polygon_type =
                  polygon_type_option.second;
            }
        }
        ImGui::EndCombo();
    }

    ImGui::SeparatorText("Bounciness");
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::BeginDisabled(client_state.map_editor_state.polygon_tool_polygon_type !=
                         PMSPolygonType::Bouncy);
    if (ImGui::DragFloat("##PolygonToolWIPPolygonBounciness",
                         &client_state.map_editor_state.polygon_tool_wip_polygon_bounciness,
                         1.0F,
                         0.0F,
                         9999999.0F,
                         "%.3f%%")) {
        if (client_state.map_editor_state.polygon_tool_wip_polygon_bounciness < 0.0F) {
            client_state.map_editor_state.polygon_tool_wip_polygon_bounciness = 0.0F;
        }
        if (client_state.map_editor_state.polygon_tool_wip_polygon_bounciness > 9999999.0F) {
            client_state.map_editor_state.polygon_tool_wip_polygon_bounciness = 9999999.0F;
        }
    }
    ImGui::EndDisabled();

    if (placed_vertices_count >= 0) {
        ImGui::SeparatorText("Vertex 1");
        ImGui::Text("Position: (%.2f, %.2f)", first_vertex_position.x, first_vertex_position.y);
    }

    if (placed_vertices_count >= 1) {
        ImGui::SeparatorText("Vertex 2");
        ImGui::Text("Position: (%.2f, %.2f)", second_vertex_position.x, second_vertex_position.y);
    }

    if (placed_vertices_count >= 2) {
        ImGui::SeparatorText("Vertex 3");
        ImGui::Text("Position: (%.2f, %.2f)", third_vertex_position.x, third_vertex_position.y);
    }
}

void RenderSelectionToolDetails(State& game_state, ClientState& client_state)
{
    static ToolDetailsSubWindowOpenType sub_window_open_type = ToolDetailsSubWindowOpenType::None;

    switch (sub_window_open_type) {
        case ToolDetailsSubWindowOpenType::None: {
            if (!client_state.map_editor_state.selected_spawn_point_ids.empty()) {
                sub_window_open_type = ToolDetailsSubWindowOpenType::SpawnPoints;
            }

            if (!client_state.map_editor_state.selected_scenery_ids.empty()) {
                sub_window_open_type = ToolDetailsSubWindowOpenType::Sceneries;
            }

            if (!client_state.map_editor_state.selected_polygon_vertices.empty()) {
                sub_window_open_type = ToolDetailsSubWindowOpenType::Polygons;
            }
            break;
        }
        case ToolDetailsSubWindowOpenType::Polygons: {
            if (client_state.map_editor_state.selected_polygon_vertices.empty()) {
                sub_window_open_type = ToolDetailsSubWindowOpenType::None;
            }
            break;
        }
        case ToolDetailsSubWindowOpenType::Sceneries: {
            if (client_state.map_editor_state.selected_scenery_ids.empty()) {
                sub_window_open_type = ToolDetailsSubWindowOpenType::None;
            }
            break;
        }
        case ToolDetailsSubWindowOpenType::SpawnPoints: {
            if (client_state.map_editor_state.selected_spawn_point_ids.empty()) {
                sub_window_open_type = ToolDetailsSubWindowOpenType::None;
            }
            break;
        }
    }

    std::string polygons_header =
      "Selected " + std::to_string(client_state.map_editor_state.selected_polygon_vertices.size()) +
      " polygons";
    if (sub_window_open_type == ToolDetailsSubWindowOpenType::Polygons) {
        ImGui::SetNextItemOpen(true);
    } else {
        ImGui::SetNextItemOpen(false);
    }
    if (ImGui::CollapsingHeader(polygons_header.c_str())) {
        sub_window_open_type = ToolDetailsSubWindowOpenType::Polygons;
        ImGui::Text("test");
    }

    std::string sceneries_header =
      "Selected " + std::to_string(client_state.map_editor_state.selected_scenery_ids.size()) +
      " sceneries";
    if (sub_window_open_type == ToolDetailsSubWindowOpenType::Sceneries) {
        ImGui::SetNextItemOpen(true);
    } else {
        ImGui::SetNextItemOpen(false);
    }
    if (ImGui::CollapsingHeader(sceneries_header.c_str())) {
        sub_window_open_type = ToolDetailsSubWindowOpenType::Sceneries;

        if (!client_state.map_editor_state.selected_scenery_ids.empty()) {
            const auto& scenery = game_state.map.GetSceneryInstances().at(
              client_state.map_editor_state.selected_scenery_ids.at(0));
            const auto& scenery_type = game_state.map.GetSceneryTypes().at(scenery.style - 1);

            std::string current_level_text;

            if (scenery.level == 0) {
                current_level_text = "Back";
            } else if (scenery.level == 1) {
                current_level_text = "Middle";
            } else if (scenery.level == 2) {
                current_level_text = "Front";
            }

            if (ImGui::BeginCombo("##SceneryLevelComboInput", current_level_text.c_str())) {
                if (ImGui::Selectable("Back", scenery.level == 0)) {
                    client_state.map_editor_state.event_selected_sceneries_level_changed.Notify(0);
                }
                if (ImGui::Selectable("Middle", scenery.level == 1)) {
                    client_state.map_editor_state.event_selected_sceneries_level_changed.Notify(1);
                }
                if (ImGui::Selectable("Front", scenery.level == 2)) {
                    client_state.map_editor_state.event_selected_sceneries_level_changed.Notify(2);
                }
                ImGui::EndCombo();
            }

            ImGui::Text("Position: %.2f %.2f", scenery.x, scenery.y);
            ImGui::Text("File: %s", scenery_type.name.c_str());
        }
    }

    std::string spawn_points_header =
      "Selected " + std::to_string(client_state.map_editor_state.selected_spawn_point_ids.size()) +
      " spawn points";
    if (sub_window_open_type == ToolDetailsSubWindowOpenType::SpawnPoints) {
        ImGui::SetNextItemOpen(true);
    } else {
        ImGui::SetNextItemOpen(false);
    }
    if (ImGui::CollapsingHeader(spawn_points_header.c_str())) {
        sub_window_open_type = ToolDetailsSubWindowOpenType::SpawnPoints;

        if (!client_state.map_editor_state.selected_spawn_point_ids.empty()) {
            const auto& spawn_point = game_state.map.GetSpawnPoints().at(
              client_state.map_editor_state.selected_spawn_point_ids.at(0));

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
            std::string current_spawn_point_type;
            for (const auto& spawn_point_option : spawn_point_options) {
                if (spawn_point.type == spawn_point_option.second) {
                    current_spawn_point_type = spawn_point_option.first;
                }
            }
            if (ImGui::BeginCombo("##SpawnPointTypeComboInput", current_spawn_point_type.c_str())) {
                for (const auto& spawn_point_option : spawn_point_options) {
                    if (ImGui::Selectable(spawn_point_option.first.c_str(),
                                          spawn_point.type == spawn_point_option.second)) {
                        client_state.map_editor_state.event_selected_spawn_points_type_changed
                          .Notify(spawn_point_option.second);
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::Text("Position %d %d", spawn_point.x, spawn_point.y);
        }
    }
}

void RenderSceneryToolDetails(State& game_state, ClientState& client_state)
{
    unsigned short new_scenery_id = game_state.map.GetSceneryInstances().size() + 1;
    ImGui::Text("Placing scenery: %hu/500", new_scenery_id);
    ImGui::Text("Scenery type: %s",
                client_state.map_editor_state.selected_scenery_to_place.c_str());
    ImGui::SeparatorText("Rotation");
    float rotation_in_degrees =
      client_state.map_editor_state.scenery_to_place.rotation * 180.0F / std::numbers::pi_v<float>;
    if (ImGui::DragFloat("##SceneryToolWIPSceneryRotation",
                         &rotation_in_degrees,
                         1.0F,
                         0.0F,
                         360.0F,
                         "%.0f°",
                         ImGuiSliderFlags_AlwaysClamp)) {
        client_state.map_editor_state.scenery_to_place.rotation =
          rotation_in_degrees * std::numbers::pi_v<float> / 180.0F;
    }
    ImGui::SeparatorText("Scale");
    ImGui::SetNextItemWidth(105.0F);
    float scale_x_in_percent = client_state.map_editor_state.scenery_to_place.scale_x * 100.0F;
    if (ImGui::DragFloat("##SceneryToolWIPSceneryScaleX",
                         &scale_x_in_percent,
                         1.0F,
                         -100000.0F,
                         100000.0F,
                         "X: %.2f%%",
                         ImGuiSliderFlags_AlwaysClamp)) {
        client_state.map_editor_state.scenery_to_place.scale_x = scale_x_in_percent / 100.0F;
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(105.0F);
    float scale_y_in_percent = client_state.map_editor_state.scenery_to_place.scale_y * 100.0F;
    if (ImGui::DragFloat("##SceneryToolWIPSceneryScaleY",
                         &scale_y_in_percent,
                         1.0F,
                         -100000.0F,
                         100000.0F,
                         "Y: %.2f%%",
                         ImGuiSliderFlags_AlwaysClamp)) {
        client_state.map_editor_state.scenery_to_place.scale_y = scale_y_in_percent / 100.0F;
    }
}

void Render(State& game_state, ClientState& client_state)
{
    {
        if (ImGui::Begin("Tool Details",
                         nullptr,
                         ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize |
                           ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollWithMouse |
                           ImGuiWindowFlags_NoCollapse)) {

            switch (client_state.map_editor_state.selected_tool) {
                case ToolType::Transform:
                    break;
                case ToolType::Polygon: {
                    RenderPolygonToolDetails(game_state, client_state);
                    break;
                }
                case ToolType::VertexSelection:
                case ToolType::Selection: {
                    RenderSelectionToolDetails(game_state, client_state);
                    break;
                }
                case ToolType::VertexColor:
                case ToolType::Color:
                case ToolType::Texture:
                    break;
                case ToolType::Scenery: {
                    RenderSceneryToolDetails(game_state, client_state);
                    break;
                }
                case ToolType::Waypoint:
                case ToolType::Spawnpoint:
                case ToolType::ColorPicker:
                    break;
            }

            ImGui::End();
        }
    }
}
} // namespace Soldank::MapEditorToolDetailsWindow
