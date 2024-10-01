#include "map_editor/tools/SelectionTool.hpp"

namespace Soldank
{
void SelectionTool::OnSelect() {}

void SelectionTool::OnUnselect() {}

void SelectionTool::OnSceneLeftMouseButtonClick(ClientState& client_state, const State& game_state)
{
    client_state.map_editor_state.selected_polygon_ids.clear();
    const auto& map = game_state.map;
    auto pos = mouse_map_position_;

    unsigned int i = start_polygon_lookup_from_id_;
    // doing do while on purpose to rotate through polygons if mouse didn't move from one spot
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-do-while)
    do {
        const auto& polygon = map.GetPolygons().at(i);

        if (Soldank::Map::PointInPoly(pos, polygon)) {
            client_state.map_editor_state.selected_polygon_ids.push_back(i);
            start_polygon_lookup_from_id_ = (i + 1) % map.GetPolygonsCount();
            break;
        }

        i++;
        i %= map.GetPolygonsCount();
    } while (i != start_polygon_lookup_from_id_);
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
    start_polygon_lookup_from_id_ = 0;
}
} // namespace Soldank
