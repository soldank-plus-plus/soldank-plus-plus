#include "map_editor/tools/SelectionTool.hpp"

#include "core/math/Calc.hpp"

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
    SetSelectionMode(SelectionMode::SingleSelection, client_state);
}

void SelectionTool::OnUnselect(ClientState& /*client_state*/) {}

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

void SelectionTool::OnSceneLeftMouseButtonRelease(ClientState& /*client_state*/,
                                                  const State& /*game_state*/)
{
}

void SelectionTool::OnSceneRightMouseButtonClick(ClientState& /*client_state*/) {}

void SelectionTool::OnSceneRightMouseButtonRelease() {}

void SelectionTool::OnMouseScreenPositionChange(ClientState& /*client_state*/,
                                                glm::vec2 /*last_mouse_position*/,
                                                glm::vec2 /*new_mouse_position*/)
{
}

void SelectionTool::OnMouseMapPositionChange(ClientState& /*client_state*/,
                                             glm::vec2 /*last_mouse_position*/,
                                             glm::vec2 new_mouse_position,
                                             const State& /*game_state*/)
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
    unsigned int selected_spawn_points_count =
      client_state.map_editor_state.selected_spawn_point_ids.size();
    unsigned int selected_soldiers_count =
      client_state.map_editor_state.selected_soldier_ids.size();
    unsigned int selected_objects_count = selected_polygons_count + selected_sceneries_count +
                                          selected_spawn_points_count + selected_soldiers_count;
    NextObjectTypeToSelect next_object_type_to_select = NextObjectTypeToSelect::Polygon;

    if (selected_objects_count == 1) {
        if (selected_polygons_count == 1) {
            unsigned int selected_polygon_id =
              client_state.map_editor_state.selected_polygon_vertices.at(0).first;
            const auto& polygon = map.GetPolygons().at(selected_polygon_id);

            if (Map::PointInPoly(mouse_map_position_, polygon)) {
                // If we have only one polygon selected and we still click inside of it then we
                // want to "rotate" through objects
                start_index = selected_polygon_id + 1;
                next_object_type_to_select = NextObjectTypeToSelect::Polygon;
            }
        } else if (selected_sceneries_count == 1) {
            unsigned int selected_scenery_id =
              client_state.map_editor_state.selected_scenery_ids.at(0);
            const auto& scenery = map.GetSceneryInstances().at(selected_scenery_id);

            if (Map::PointInScenery(mouse_map_position_, scenery)) {
                // If we have only one scenery selected and we still click inside of it then we
                // want to "rotate" through objects
                start_index = selected_scenery_id + 1;
                next_object_type_to_select = NextObjectTypeToSelect::Scenery;
            }
        } else if (selected_spawn_points_count == 1) {
            unsigned int selected_spawn_point_id =
              client_state.map_editor_state.selected_spawn_point_ids.at(0);
            const auto& spawn_point = map.GetSpawnPoints().at(selected_spawn_point_id);

            if (IsMouseInSpawnPoint(client_state, { spawn_point.x, spawn_point.y })) {
                // If we have only one scenery selected and we still click inside of it then we
                // want to "rotate" through objects
                start_index = selected_spawn_point_id + 1;
                next_object_type_to_select = NextObjectTypeToSelect::SpawnPoint;
            }
        } else if (selected_soldiers_count == 1) {
            unsigned int selected_soldier_id =
              client_state.map_editor_state.selected_soldier_ids.at(0);
            for (const auto& soldier : game_state.soldiers) {
                if (soldier.id != selected_soldier_id) {
                    continue;
                }

                if (IsMouseInSoldier(soldier.particle.position)) {
                    // If we have only one scenery selected and we still click inside of it then we
                    // want to "rotate" through objects

                    // selected_soldier_id is soldier.id which starts from 1 instead of 0 that's why
                    // we don't do +1 here
                    start_index = selected_soldier_id;
                    next_object_type_to_select = NextObjectTypeToSelect::Soldier;
                }
            }
        }
    }

    client_state.map_editor_state.selected_polygon_vertices.clear();
    client_state.map_editor_state.selected_scenery_ids.clear();
    client_state.map_editor_state.selected_spawn_point_ids.clear();
    client_state.map_editor_state.selected_soldier_ids.clear();

    if (map.GetPolygonsCount() == 0 && map.GetSceneryInstances().empty() &&
        map.GetSpawnPoints().empty()) {
        return;
    }

    for (const auto& polygon : map.GetPolygons()) {
        // TODO: can be optimized
        client_state.map_editor_state.event_polygon_selected.Notify(polygon, { 0b000 });
    }

    if (next_object_type_to_select == NextObjectTypeToSelect::Polygon &&
        start_index >= map.GetPolygonsCount()) {

        start_index = 0;
        next_object_type_to_select =
          GetNextObjectTypeToSelect(next_object_type_to_select, game_state);
    }

    if (next_object_type_to_select == NextObjectTypeToSelect::Scenery &&
        start_index >= map.GetSceneryInstances().size()) {

        start_index = 0;
        next_object_type_to_select =
          GetNextObjectTypeToSelect(next_object_type_to_select, game_state);
    }

    if (next_object_type_to_select == NextObjectTypeToSelect::SpawnPoint &&
        start_index >= map.GetSpawnPoints().size()) {

        start_index = 0;
        next_object_type_to_select =
          GetNextObjectTypeToSelect(next_object_type_to_select, game_state);
    }

    if (next_object_type_to_select == NextObjectTypeToSelect::Soldier &&
        start_index >= game_state.soldiers.size()) {

        start_index = 0;
        next_object_type_to_select =
          GetNextObjectTypeToSelect(next_object_type_to_select, game_state);
    }

    SelectNextObject(client_state, game_state, start_index, next_object_type_to_select);
}

void SelectionTool::SelectNextObject(ClientState& client_state,
                                     const State& game_state,
                                     unsigned int start_index,
                                     NextObjectTypeToSelect next_object_type_to_select)
{
    unsigned int current_index = start_index;
    const auto& map = game_state.map;
    unsigned int polygon_candidates_count = 0;
    unsigned int scenery_candidates_count = 0;
    unsigned int spawn_point_candidates_count = 0;
    unsigned int soldier_candidates_count = 0;

    while (polygon_candidates_count < map.GetPolygonsCount() ||
           scenery_candidates_count < map.GetSceneryInstances().size() ||
           spawn_point_candidates_count < map.GetSpawnPoints().size() ||
           soldier_candidates_count < game_state.soldiers.size()) {

        switch (next_object_type_to_select) {
            case NextObjectTypeToSelect::Polygon: {
                const auto& polygon = map.GetPolygons().at(current_index);

                if (Map::PointInPoly(mouse_map_position_, polygon)) {
                    client_state.map_editor_state.selected_polygon_vertices.push_back(
                      { current_index, { 0b111 } });
                    client_state.map_editor_state.event_polygon_selected.Notify(polygon, { 0b111 });
                    return;
                }
                break;
            }
            case NextObjectTypeToSelect::Scenery: {
                const auto& scenery = map.GetSceneryInstances().at(current_index);

                if (Map::PointInScenery(mouse_map_position_, scenery)) {
                    client_state.map_editor_state.selected_scenery_ids.push_back(current_index);
                    return;
                }
                break;
            }
            case NextObjectTypeToSelect::SpawnPoint: {
                const auto& spawn_point = map.GetSpawnPoints().at(current_index);

                if (IsMouseInSpawnPoint(client_state, { spawn_point.x, spawn_point.y })) {
                    client_state.map_editor_state.selected_spawn_point_ids.push_back(current_index);
                    return;
                }
                break;
            }
            case NextObjectTypeToSelect::Soldier: {
                for (const auto& soldier : game_state.soldiers) {
                    if (soldier.id != current_index + 1) {
                        continue;
                    }

                    if (IsMouseInSoldier(soldier.particle.position)) {
                        client_state.map_editor_state.selected_soldier_ids.push_back(soldier.id);
                        return;
                    }
                }
                break;
            }
        }

        ++current_index;

        switch (next_object_type_to_select) {
            case NextObjectTypeToSelect::Polygon: {
                ++polygon_candidates_count;
                if (current_index == map.GetPolygonsCount()) {
                    current_index = 0;
                    next_object_type_to_select =
                      GetNextObjectTypeToSelect(next_object_type_to_select, game_state);
                }
                break;
            }
            case NextObjectTypeToSelect::Scenery: {
                ++scenery_candidates_count;
                if (current_index == map.GetSceneryInstances().size()) {
                    current_index = 0;
                    next_object_type_to_select =
                      GetNextObjectTypeToSelect(next_object_type_to_select, game_state);
                }
                break;
            }
            case NextObjectTypeToSelect::SpawnPoint: {
                ++spawn_point_candidates_count;
                if (current_index == map.GetSpawnPoints().size()) {
                    current_index = 0;
                    next_object_type_to_select =
                      GetNextObjectTypeToSelect(next_object_type_to_select, game_state);
                }
                break;
            }
            case NextObjectTypeToSelect::Soldier: {
                ++soldier_candidates_count;
                if (current_index == game_state.soldiers.size()) {
                    current_index = 0;
                    next_object_type_to_select =
                      GetNextObjectTypeToSelect(next_object_type_to_select, game_state);
                }
                break;
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
    if (AddFirstFoundSceneryToSelection(client_state, game_state)) {
        return;
    }

    if (AddFirstFoundSpawnPointToSelection(client_state, game_state)) {
        return;
    }

    if (AddFirstFoundSoldierToSelection(client_state, game_state)) {
        return;
    }
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

bool SelectionTool::AddFirstFoundSpawnPointToSelection(ClientState& client_state,
                                                       const State& game_state)
{
    const auto& map = game_state.map;

    for (unsigned int spawn_point_id = 0; spawn_point_id < map.GetSpawnPoints().size();
         ++spawn_point_id) {

        const auto& spawn_point = map.GetSpawnPoints().at(spawn_point_id);

        if (IsMouseInSpawnPoint(client_state, { spawn_point.x, spawn_point.y })) {
            bool already_selected = false;
            for (const auto& selected_spawn_point_id :
                 client_state.map_editor_state.selected_spawn_point_ids) {
                if (spawn_point_id == selected_spawn_point_id) {
                    already_selected = true;
                    break;
                }
            }

            if (!already_selected) {
                client_state.map_editor_state.selected_spawn_point_ids.push_back(spawn_point_id);
                return true;
            }
        }
    }

    return false;
}

bool SelectionTool::AddFirstFoundSoldierToSelection(ClientState& client_state,
                                                    const State& game_state)
{
    for (const auto& soldier : game_state.soldiers) {
        if (IsMouseInSoldier(soldier.particle.position)) {
            bool already_selected = false;
            for (const auto& selected_soldier_id :
                 client_state.map_editor_state.selected_soldier_ids) {
                if (soldier.id == selected_soldier_id) {
                    already_selected = true;
                    break;
                }
            }

            if (!already_selected) {
                client_state.map_editor_state.selected_soldier_ids.push_back(soldier.id);
                return true;
            }
        }
    }

    return false;
}

void SelectionTool::RemoveLastFoundObjectFromSelection(ClientState& client_state,
                                                       const State& game_state)
{
    for (int i = (int)client_state.map_editor_state.selected_soldier_ids.size() - 1; i >= 0; --i) {

        const auto& selected_soldier_id = client_state.map_editor_state.selected_soldier_ids.at(i);
        for (const auto& soldier : game_state.soldiers) {
            if (soldier.id != selected_soldier_id) {
                continue;
            }

            if (IsMouseInSoldier(soldier.particle.position)) {
                client_state.map_editor_state.selected_soldier_ids.erase(
                  client_state.map_editor_state.selected_soldier_ids.begin() + i);
                return;
            }
        }
    }

    const auto& map = game_state.map;
    for (int i = (int)client_state.map_editor_state.selected_spawn_point_ids.size() - 1; i >= 0;
         --i) {

        const auto& selected_spawn_point_id =
          client_state.map_editor_state.selected_spawn_point_ids.at(i);
        const auto& spawn_point = map.GetSpawnPoints().at(selected_spawn_point_id);

        if (IsMouseInSpawnPoint(client_state, { spawn_point.x, spawn_point.y })) {
            client_state.map_editor_state.selected_spawn_point_ids.erase(
              client_state.map_editor_state.selected_spawn_point_ids.begin() + i);
            return;
        }
    }

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

void SelectionTool::OnModifierKey1Pressed(ClientState& client_state)
{
    SetSelectionMode(SelectionMode::AddToSelection, client_state);
}

void SelectionTool::OnModifierKey1Released(ClientState& client_state)
{
    SetSelectionMode(SelectionMode::SingleSelection, client_state);
}

void SelectionTool::OnModifierKey2Pressed(ClientState& client_state) {}

void SelectionTool::OnModifierKey2Released(ClientState& client_state) {}

void SelectionTool::OnModifierKey3Pressed(ClientState& client_state)
{
    SetSelectionMode(SelectionMode::RemoveFromSelection, client_state);
}

void SelectionTool::OnModifierKey3Released(ClientState& client_state)
{
    SetSelectionMode(SelectionMode::SingleSelection, client_state);
}

bool SelectionTool::IsMouseInSpawnPoint(const ClientState& client_state,
                                        const glm::vec2& spawn_point_position) const
{
    float current_camera_zoom = client_state.camera_component.GetZoom();
    return Calc::SquareDistance(spawn_point_position, mouse_map_position_) / current_camera_zoom <=
           64.0F * current_camera_zoom;
}

bool SelectionTool::IsMouseInSoldier(const glm::vec2& soldier_position) const
{
    float left = soldier_position.x - 9.0F;
    float right = soldier_position.x + 9.0F;
    float top = soldier_position.y - 25.0F;
    float bottom = soldier_position.y + 5.0F;
    return (mouse_map_position_.x >= left && mouse_map_position_.x <= right &&
            mouse_map_position_.y >= top && mouse_map_position_.y <= bottom);
}

void SelectionTool::SetSelectionMode(SelectionMode new_selection_mode, ClientState& client_state)
{
    current_selection_mode_ = new_selection_mode;

    switch (new_selection_mode) {
        case SelectionMode::SingleSelection: {
            client_state.map_editor_state.current_tool_action_description = "Select Objects";
            break;
        }
        case SelectionMode::AddToSelection: {
            client_state.map_editor_state.current_tool_action_description = "Add to Selection";
            break;
        }
        case SelectionMode::RemoveFromSelection: {
            client_state.map_editor_state.current_tool_action_description = "Remove from Selection";
            break;
        }
    }
}

SelectionTool::NextObjectTypeToSelect SelectionTool::GetNextObjectTypeToSelect(
  NextObjectTypeToSelect current_object_type_to_select,
  const State& game_state)
{
    const auto& map = game_state.map;
    NextObjectTypeToSelect new_object_type_to_select = current_object_type_to_select;
    switch (current_object_type_to_select) {
        case NextObjectTypeToSelect::Polygon:
            new_object_type_to_select = NextObjectTypeToSelect::Scenery;
            break;
        case NextObjectTypeToSelect::Scenery:
            new_object_type_to_select = NextObjectTypeToSelect::SpawnPoint;
            break;
        case NextObjectTypeToSelect::SpawnPoint:
            new_object_type_to_select = NextObjectTypeToSelect::Soldier;
            break;
        case NextObjectTypeToSelect::Soldier:
            new_object_type_to_select = NextObjectTypeToSelect::Polygon;
            break;
    }

    bool should_repeat = false;

    switch (new_object_type_to_select) {
        case NextObjectTypeToSelect::Polygon: {
            if (map.GetPolygonsCount() == 0) {
                should_repeat = true;
            }
            break;
        }
        case NextObjectTypeToSelect::Scenery: {
            if (map.GetSceneryInstances().empty()) {
                should_repeat = true;
            }
            break;
        }
        case NextObjectTypeToSelect::SpawnPoint: {
            if (map.GetSpawnPoints().empty()) {
                should_repeat = true;
            }
            break;
        }
        case NextObjectTypeToSelect::Soldier: {
            if (game_state.soldiers.empty()) {
                should_repeat = true;
            }
            break;
        }
    }

    if (should_repeat) {
        return GetNextObjectTypeToSelect(new_object_type_to_select, game_state);
    }

    return new_object_type_to_select;
}
} // namespace Soldank
