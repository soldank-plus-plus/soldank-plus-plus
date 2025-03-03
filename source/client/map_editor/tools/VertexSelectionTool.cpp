#include "map_editor/tools/VertexSelectionTool.hpp"
#include <spdlog/spdlog.h>

namespace Soldank
{
void VertexSelectionTool::OnSelect(ClientState& client_state,
                                   const StateManager& game_state_manager)
{
}

void VertexSelectionTool::OnUnselect(ClientState& client_state)
{
    client_state.map_editor_state.vertex_selection_box = std::nullopt;
}

void VertexSelectionTool::OnSceneLeftMouseButtonClick(ClientState& client_state,
                                                      const StateManager& /*game_state_manager*/)
{
    client_state.map_editor_state.vertex_selection_box = { { mouse_map_position_ },
                                                           { mouse_map_position_ } };
    client_state.map_editor_state.current_tool_action_description = "Select Vertices";
}

void VertexSelectionTool::OnSceneLeftMouseButtonRelease(ClientState& client_state,
                                                        const StateManager& game_state_manager)
{
    const Map& map = game_state_manager.GetConstMap();

    glm::vec2 selection_start_position = client_state.map_editor_state.vertex_selection_box->first;
    glm::vec2 selection_end_position = client_state.map_editor_state.vertex_selection_box->second;

    float left = std::min(selection_start_position.x, selection_end_position.x);
    float right = std::max(selection_start_position.x, selection_end_position.x);
    float bottom = std::max(selection_start_position.y, selection_end_position.y);
    float top = std::min(selection_start_position.y, selection_end_position.y);
    client_state.map_editor_state.selected_polygon_vertices.clear();
    for (const auto& polygon : map.GetPolygons()) {
        std::bitset<3> selected_vertices = { 0b000 };
        for (unsigned int vertex_id = 0; vertex_id < polygon.vertices.size(); ++vertex_id) {
            const auto& vertex = polygon.vertices.at(vertex_id);
            if (vertex.x >= left && vertex.x <= right && vertex.y >= top && vertex.y <= bottom) {
                selected_vertices[vertex_id] = true;
            }
        }
        if (selected_vertices.any()) {
            client_state.map_editor_state.selected_polygon_vertices.emplace_back(polygon.id,
                                                                                 selected_vertices);
        }
        client_state.map_editor_state.event_polygon_selected.Notify(polygon, selected_vertices);
    }

    client_state.map_editor_state.selected_scenery_ids.clear();
    for (unsigned int scenery_id = 0; scenery_id < map.GetSceneryInstances().size(); ++scenery_id) {
        const auto& scenery = map.GetSceneryInstances().at(scenery_id);
        if (scenery.x >= left && scenery.x <= right && scenery.y >= top && scenery.y <= bottom) {
            client_state.map_editor_state.selected_scenery_ids.push_back(scenery_id);
        }
    }

    client_state.map_editor_state.selected_spawn_point_ids.clear();
    for (unsigned int spawn_point_id = 0; spawn_point_id < map.GetSpawnPoints().size();
         ++spawn_point_id) {
        const auto& spawn_point = map.GetSpawnPoints().at(spawn_point_id);
        if (spawn_point.x >= left && spawn_point.x <= right && spawn_point.y >= top &&
            spawn_point.y <= bottom) {
            client_state.map_editor_state.selected_spawn_point_ids.push_back(spawn_point_id);
        }
    }

    client_state.map_editor_state.selected_soldier_ids.clear();
    game_state_manager.ForEachSoldier([&](const auto& soldier) {
        if (soldier.particle.position.x >= left && soldier.particle.position.x <= right &&
            soldier.particle.position.y >= top && soldier.particle.position.y <= bottom) {
            client_state.map_editor_state.selected_soldier_ids.push_back(soldier.id);
        }
    });

    client_state.map_editor_state.vertex_selection_box = std::nullopt;
}

void VertexSelectionTool::OnSceneRightMouseButtonClick(ClientState& client_state) {}

void VertexSelectionTool::OnSceneRightMouseButtonRelease() {}

void VertexSelectionTool::OnMouseScreenPositionChange(ClientState& client_state,
                                                      glm::vec2 last_mouse_position,
                                                      glm::vec2 new_mouse_position)
{
}

void VertexSelectionTool::OnMouseMapPositionChange(ClientState& client_state,
                                                   glm::vec2 /*last_mouse_position*/,
                                                   glm::vec2 new_mouse_position,
                                                   const StateManager& /*game_state*/)
{
    mouse_map_position_ = new_mouse_position;

    if (client_state.map_editor_state.vertex_selection_box) {
        client_state.map_editor_state.vertex_selection_box->second = new_mouse_position;
    }
}

void VertexSelectionTool::OnModifierKey1Pressed(ClientState& client_state) {}

void VertexSelectionTool::OnModifierKey1Released(ClientState& client_state) {}

void VertexSelectionTool::OnModifierKey2Pressed(ClientState& client_state) {}

void VertexSelectionTool::OnModifierKey2Released(ClientState& client_state) {}

void VertexSelectionTool::OnModifierKey3Pressed(ClientState& client_state) {}

void VertexSelectionTool::OnModifierKey3Released(ClientState& client_state) {}
} // namespace Soldank
