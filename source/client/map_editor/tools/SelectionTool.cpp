#include "map_editor/tools/SelectionTool.hpp"

namespace Soldank
{
void SelectionTool::OnSelect() {}

void SelectionTool::OnUnselect() {}

void SelectionTool::OnSceneLeftMouseButtonClick(ClientState& client_state, const State& game_state)
{
    const auto& map = game_state.map;

    unsigned int start_index = 0;
    unsigned int selected_polygons_count =
      client_state.map_editor_state.selected_polygon_ids.size();
    unsigned int selected_sceneries_count =
      client_state.map_editor_state.selected_scenery_ids.size();
    unsigned int selected_objects_count = selected_polygons_count + selected_sceneries_count;
    bool look_for_polygon_initially = true;

    if (selected_objects_count == 1) {
        if (selected_polygons_count == 1) {
            unsigned int selected_polygon_id =
              client_state.map_editor_state.selected_polygon_ids.at(0);
            const auto& polygon = map.GetPolygons().at(selected_polygon_id);

            if (Map::PointInPoly(mouse_map_position_, polygon)) {
                // If we have only one polygon selected and we still click inside of it then we
                // want to "rotate" through objects
                start_index = selected_polygon_id + 1;
                look_for_polygon_initially = true;
            }
        } else if (selected_sceneries_count == 1) {
            unsigned int selected_scenery_id =
              client_state.map_editor_state.selected_scenery_ids.at(0);
            const auto& scenery = map.GetSceneryInstances().at(selected_scenery_id);

            if (Map::PointInScenery(mouse_map_position_, scenery)) {
                // If we have only one scenery selected and we still click inside of it then we
                // want to "rotate" through objects
                start_index = selected_scenery_id + 1;
                look_for_polygon_initially = false;
            }
        }
    }

    client_state.map_editor_state.selected_polygon_ids.clear();
    client_state.map_editor_state.selected_scenery_ids.clear();

    SelectNextObject(client_state, game_state, start_index, look_for_polygon_initially);
}

void SelectionTool::OnSceneLeftMouseButtonRelease() {}

void SelectionTool::OnSceneRightMouseButtonClick() {}

void SelectionTool::OnSceneRightMouseButtonRelease() {}

void SelectionTool::OnMouseScreenPositionChange(ClientState& /*client_state*/,
                                                glm::vec2 /*last_mouse_position*/,
                                                glm::vec2 /*new_mouse_position*/)
{
}

void SelectionTool::OnMouseMapPositionChange(ClientState& /*client_state*/,
                                             glm::vec2 /*last_mouse_position*/,
                                             glm::vec2 new_mouse_position)
{
    mouse_map_position_ = new_mouse_position;
}

void SelectionTool::SelectNextObject(ClientState& client_state,
                                     const State& game_state,
                                     unsigned int start_index,
                                     bool look_for_polygon_initially)
{
    unsigned int current_index = start_index;
    bool looking_for_polygon = look_for_polygon_initially;
    const auto& map = game_state.map;
    unsigned int polygon_candidates_count = 0;
    unsigned int scenery_candidates_count = 0;
    while (polygon_candidates_count < map.GetPolygonsCount() ||
           scenery_candidates_count < map.GetSceneryInstances().size()) {
        if (looking_for_polygon) {
            const auto& polygon = map.GetPolygons().at(current_index);

            if (Map::PointInPoly(mouse_map_position_, polygon)) {
                client_state.map_editor_state.selected_polygon_ids.push_back(current_index);
                break;
            }
        } else {
            const auto& scenery = map.GetSceneryInstances().at(current_index);

            if (Map::PointInScenery(mouse_map_position_, scenery)) {
                client_state.map_editor_state.selected_scenery_ids.push_back(current_index);
                break;
            }
        }

        ++current_index;
        if (looking_for_polygon) {
            ++polygon_candidates_count;
            if (current_index == map.GetPolygonsCount()) {
                current_index = 0;
                looking_for_polygon = false;
            }
        } else {
            ++scenery_candidates_count;
            if (current_index == map.GetSceneryInstances().size()) {
                current_index = 0;
                looking_for_polygon = true;
            }
        }
    }
}
} // namespace Soldank
