#include "map_editor/tools/SelectionTool.hpp"

namespace Soldank
{
void SelectionTool::OnSelect(ClientState& client_state, const State& game_state)
{
    for (auto& selected_vertices : client_state.map_editor_state.selected_polygon_vertices) {
        if (!selected_vertices.second.all()) {
            selected_vertices.second = { 0b111 };
            client_state.map_editor_state.event_polygon_selected.Notify(
              game_state.map.GetPolygons().at(selected_vertices.first), selected_vertices.second);
        }
    }
}

void SelectionTool::OnUnselect(ClientState& client_state) {}

void SelectionTool::OnSceneLeftMouseButtonClick(ClientState& client_state, const State& game_state)
{
    switch (current_selection_mode_) {
        case SelectionMode::SingleSelection: {
            SelectNextSingleObject(client_state, game_state);
            break;
        }
        case SelectionMode::AddToSelection: {
            AddFirstFoundObjectToSelection(client_state, game_state);
            break;
        }
        case SelectionMode::RemoveFromSelection: {
            RemoveLastFoundObjectFromSelection(client_state, game_state);
            break;
        }
    }
}

void SelectionTool::OnSceneLeftMouseButtonRelease(ClientState& client_state,
                                                  const State& game_state)
{
}

void SelectionTool::OnSceneRightMouseButtonClick(ClientState& client_state) {}

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

void SelectionTool::SelectNextSingleObject(ClientState& client_state, const State& game_state)
{
    const auto& map = game_state.map;

    unsigned int start_index = 0;
    unsigned int selected_polygons_count =
      client_state.map_editor_state.selected_polygon_vertices.size();
    unsigned int selected_sceneries_count =
      client_state.map_editor_state.selected_scenery_ids.size();
    unsigned int selected_objects_count = selected_polygons_count + selected_sceneries_count;
    bool look_for_polygon_initially = true;

    if (selected_objects_count == 1) {
        if (selected_polygons_count == 1) {
            unsigned int selected_polygon_id =
              client_state.map_editor_state.selected_polygon_vertices.at(0).first;
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

    client_state.map_editor_state.selected_polygon_vertices.clear();
    client_state.map_editor_state.selected_scenery_ids.clear();
    for (const auto& polygon : map.GetPolygons()) {
        // TODO: can be optimized
        client_state.map_editor_state.event_polygon_selected.Notify(polygon, { 0b000 });
    }

    if (look_for_polygon_initially && start_index >= map.GetPolygonsCount()) {
        start_index = 0;
        look_for_polygon_initially = false;
    }

    if (!look_for_polygon_initially && start_index >= map.GetSceneryInstances().size()) {
        start_index = 0;
        look_for_polygon_initially = true;
    }

    SelectNextObject(client_state, game_state, start_index, look_for_polygon_initially);
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
                client_state.map_editor_state.selected_polygon_vertices.push_back(
                  { current_index, { 0b111 } });
                client_state.map_editor_state.event_polygon_selected.Notify(polygon, { 0b111 });
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

void SelectionTool::AddFirstFoundObjectToSelection(ClientState& client_state,
                                                   const State& game_state)
{
    if (AddFirstFoundPolygonToSelection(client_state, game_state)) {
        return;
    }
    AddFirstFoundSceneryToSelection(client_state, game_state);
}

bool SelectionTool::AddFirstFoundPolygonToSelection(ClientState& client_state,
                                                    const State& game_state)
{
    const auto& map = game_state.map;

    for (const auto& polygon : map.GetPolygons()) {
        if (Map::PointInPoly(mouse_map_position_, polygon)) {
            bool already_selected = false;
            for (const auto& selected_polygon_vertices :
                 client_state.map_editor_state.selected_polygon_vertices) {
                if (polygon.id == selected_polygon_vertices.first) {
                    already_selected = true;
                    break;
                }
            }

            if (!already_selected) {
                client_state.map_editor_state.selected_polygon_vertices.push_back(
                  { polygon.id, { 0b111 } });
                client_state.map_editor_state.event_polygon_selected.Notify(polygon, { 0b111 });
                return true;
            }
        }
    }

    return false;
}

bool SelectionTool::AddFirstFoundSceneryToSelection(ClientState& client_state,
                                                    const State& game_state)
{
    const auto& map = game_state.map;

    for (unsigned int scenery_id = 0; scenery_id < map.GetSceneryInstances().size(); ++scenery_id) {
        const auto& scenery = map.GetSceneryInstances().at(scenery_id);
        if (Map::PointInScenery(mouse_map_position_, scenery)) {
            bool already_selected = false;
            for (const auto& selected_scenery_id :
                 client_state.map_editor_state.selected_scenery_ids) {
                if (scenery_id == selected_scenery_id) {
                    already_selected = true;
                    break;
                }
            }

            if (!already_selected) {
                client_state.map_editor_state.selected_scenery_ids.push_back(scenery_id);
                return true;
            }
        }
    }

    return false;
}

void SelectionTool::RemoveLastFoundObjectFromSelection(ClientState& client_state,
                                                       const State& game_state)
{
    const auto& map = game_state.map;
    for (int i = (int)client_state.map_editor_state.selected_scenery_ids.size() - 1; i >= 0; --i) {
        const auto& selected_scenery_id = client_state.map_editor_state.selected_scenery_ids.at(i);
        const auto& scenery = map.GetSceneryInstances().at(selected_scenery_id);

        if (Map::PointInScenery(mouse_map_position_, scenery)) {
            client_state.map_editor_state.selected_scenery_ids.erase(
              client_state.map_editor_state.selected_scenery_ids.begin() + i);
            return;
        }
    }

    for (int i = (int)client_state.map_editor_state.selected_polygon_vertices.size() - 1; i >= 0;
         --i) {
        const auto& selected_polygon_vertices =
          client_state.map_editor_state.selected_polygon_vertices.at(i);
        const auto& polygon = map.GetPolygons().at(selected_polygon_vertices.first);

        if (Map::PointInPoly(mouse_map_position_, polygon)) {
            client_state.map_editor_state.selected_polygon_vertices.erase(
              client_state.map_editor_state.selected_polygon_vertices.begin() + i);
            client_state.map_editor_state.event_polygon_selected.Notify(polygon, { 0b000 });
            return;
        }
    }
}

void SelectionTool::OnModifierKey1Pressed()
{
    current_selection_mode_ = SelectionMode::AddToSelection;
}

void SelectionTool::OnModifierKey1Released()
{
    current_selection_mode_ = SelectionMode::SingleSelection;
}

void SelectionTool::OnModifierKey2Pressed() {}

void SelectionTool::OnModifierKey2Released() {}

void SelectionTool::OnModifierKey3Pressed()
{
    current_selection_mode_ = SelectionMode::RemoveFromSelection;
}

void SelectionTool::OnModifierKey3Released()
{
    current_selection_mode_ = SelectionMode::SingleSelection;
}
} // namespace Soldank
