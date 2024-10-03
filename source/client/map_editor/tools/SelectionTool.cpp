#include "map_editor/tools/SelectionTool.hpp"

namespace Soldank
{
void SelectionTool::OnSelect() {}

void SelectionTool::OnUnselect() {}

void SelectionTool::OnSceneLeftMouseButtonClick(ClientState& client_state, const State& game_state)
{
    const auto& map = game_state.map;

    unsigned int i = 0;
    unsigned int polygon_candidates_count = 0;
    if (client_state.map_editor_state.selected_polygon_ids.size() == 1) {
        unsigned int selected_polygon_id = client_state.map_editor_state.selected_polygon_ids.at(0);
        const auto& polygon = map.GetPolygons().at(selected_polygon_id);

        if (Map::PointInPoly(mouse_map_position_, polygon)) {
            // If we have only one polygon selected  and we still click inside of it then we want to
            // "rotate" through objects
            i = selected_polygon_id + 1;
            i %= map.GetPolygonsCount();
        }
    }

    client_state.map_editor_state.selected_polygon_ids.clear();

    while (polygon_candidates_count < map.GetPolygonsCount()) {
        const auto& polygon = map.GetPolygons().at(i);

        if (Map::PointInPoly(mouse_map_position_, polygon)) {
            client_state.map_editor_state.selected_polygon_ids.push_back(i);
            break;
        }

        ++i;
        ++polygon_candidates_count;
        i %= map.GetPolygonsCount();
    }
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
} // namespace Soldank
