module;

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

export module SelectionTool;

import Extern.Glm;

import Tool;
import MapEditorAction;
import ClientState;

import Shared.Core.State.StateManager;
import Shared.Core.Utility.Observable;
import Shared.Core.Math.Calc;
import Shared.Core.Map.Map;

export namespace Soldank
{
class SelectionTool final : public Tool
{
private:
    enum class SelectionMode : std::uint8_t
    {
        SingleSelection = 0,
        AddToSelection,
        RemoveFromSelection,
    };

    enum class SelectableObjectType : std::uint8_t
    {
        Polygon = 0,
        Scenery,
        SpawnPoint,
        Soldier,
    };

    struct SelectableObject
    {
        SelectableObjectType type;
        unsigned int id;

        bool operator==(const SelectableObject&) const = default;
    };

public:
    ~SelectionTool() final = default;

    void OnSelect(ClientState& client_state, const StateManager& game_state_manager) final
    {
        for (auto& selected_vertices : client_state.map_editor_state.selected_polygon_vertices) {
            if (!selected_vertices.second.all()) {
                selected_vertices.second = { 0b111 };
                client_state.map_editor_state.event_polygon_selected.Notify(
                  game_state_manager.GetConstMap().GetPolygons().at(selected_vertices.first),
                  selected_vertices.second);
            }
        }
        SetSelectionMode(SelectionMode::SingleSelection, client_state);
    }

    void OnUnselect(ClientState& /*client_state*/) final {}

    void OnSceneLeftMouseButtonClick(ClientState& client_state,
                                     const StateManager& game_state_manager) final
    {
        switch (current_selection_mode_) {
            case SelectionMode::SingleSelection: {
                SelectNextSingleObject(client_state, game_state_manager);
                break;
            }
            case SelectionMode::AddToSelection: {
                AddFirstFoundObjectToSelection(client_state, game_state_manager);
                break;
            }
            case SelectionMode::RemoveFromSelection: {
                RemoveLastFoundObjectFromSelection(client_state, game_state_manager);
                break;
            }
        }
    }

    void OnSceneLeftMouseButtonRelease(ClientState& /*client_state*/,
                                       const StateManager& /*game_state_manager*/) final
    {
    }

    void OnSceneRightMouseButtonClick(ClientState& client_state) final
    {
        client_state.map_editor_state.should_open_selection_context_menu = true;
    }

    void OnSceneRightMouseButtonRelease() final {}

    void OnMouseScreenPositionChange(ClientState& /*client_state*/,
                                     glm::vec2 /*last_mouse_position*/,
                                     glm::vec2 /*new_mouse_position*/) final
    {
    }

    void OnMouseMapPositionChange(ClientState& /*client_state*/,
                                  glm::vec2 /*last_mouse_position*/,
                                  glm::vec2 new_mouse_position,
                                  const StateManager& /*game_state_manager*/) final
    {
        mouse_map_position_ = new_mouse_position;
    }

    void OnModifierKey1Pressed(ClientState& client_state) final
    {
        SetSelectionMode(SelectionMode::AddToSelection, client_state);
    }

    void OnModifierKey1Released(ClientState& client_state) final
    {
        SetSelectionMode(SelectionMode::SingleSelection, client_state);
    }

    void OnModifierKey2Pressed(ClientState& /* client_state */) final {}

    void OnModifierKey2Released(ClientState& /* client_state */) final {}

    void OnModifierKey3Pressed(ClientState& client_state) final
    {
        SetSelectionMode(SelectionMode::RemoveFromSelection, client_state);
    }

    void OnModifierKey3Released(ClientState& client_state) final
    {
        SetSelectionMode(SelectionMode::SingleSelection, client_state);
    }

    void SelectNextSingleObject(ClientState& client_state, const StateManager& game_state_manager)
    {
        const std::vector<SelectableObject> candidates =
          FindObjectsAtCursor(client_state, game_state_manager);
        const std::optional<SelectableObject> current_selection =
          GetSingleSelectedObject(client_state);

        std::size_t next_candidate_index = 0;
        if (current_selection) {
            const auto current_candidate = std::ranges::find(candidates, *current_selection);
            if (current_candidate != candidates.end() && !candidates.empty()) {
                next_candidate_index =
                  (static_cast<std::size_t>(std::distance(candidates.begin(), current_candidate)) +
                   1) %
                  candidates.size();
            }
        }

        ClearSelection(client_state, game_state_manager);
        if (!candidates.empty()) {
            SelectObject(candidates.at(next_candidate_index), client_state, game_state_manager);
        }
    }

    void AddFirstFoundObjectToSelection(ClientState& client_state,
                                        const StateManager& game_state_manager)
    {
        const std::vector<SelectableObject> candidates =
          FindObjectsAtCursor(client_state, game_state_manager);
        const auto candidate = std::ranges::find_if(
          candidates, [&](const auto& object) { return !IsSelected(object, client_state); });
        if (candidate != candidates.end()) {
            SelectObject(*candidate, client_state, game_state_manager);
        }
    }

    void RemoveLastFoundObjectFromSelection(ClientState& client_state,
                                            const StateManager& game_state_manager) const
    {
        const std::optional<SelectableObject> object =
          FindSelectedObjectAtCursorForRemoval(client_state, game_state_manager);
        if (object) {
            DeselectObject(*object, client_state, game_state_manager);
        }
    }

    std::vector<SelectableObject> FindObjectsAtCursor(const ClientState& client_state,
                                                      const StateManager& game_state_manager) const
    {
        std::vector<SelectableObject> objects;
        const auto& map = game_state_manager.GetConstMap();

        for (unsigned int id = 0; id < map.GetPolygonsCount(); ++id) {
            if (Map::PointInPoly(mouse_map_position_, map.GetPolygons().at(id))) {
                objects.push_back({ SelectableObjectType::Polygon, id });
            }
        }
        for (unsigned int id = 0; id < map.GetSceneryInstances().size(); ++id) {
            if (Map::PointInScenery(mouse_map_position_, map.GetSceneryInstances().at(id))) {
                objects.push_back({ SelectableObjectType::Scenery, id });
            }
        }
        for (unsigned int id = 0; id < map.GetSpawnPoints().size(); ++id) {
            const auto& spawn_point = map.GetSpawnPoints().at(id);
            if (IsMouseInSpawnPoint(client_state, { spawn_point.x, spawn_point.y })) {
                objects.push_back({ SelectableObjectType::SpawnPoint, id });
            }
        }
        game_state_manager.ForEachSoldier([&](const auto& soldier) {
            if (IsMouseInSoldier(soldier.particle.position)) {
                objects.push_back({ SelectableObjectType::Soldier, soldier.id });
            }
        });
        return objects;
    }

    static std::optional<SelectableObject> GetSingleSelectedObject(const ClientState& client_state)
    {
        const auto& editor_state = client_state.map_editor_state;
        const std::size_t selected_count =
          editor_state.selected_polygon_vertices.size() + editor_state.selected_scenery_ids.size() +
          editor_state.selected_spawn_point_ids.size() + editor_state.selected_soldier_ids.size();
        if (selected_count != 1) {
            return std::nullopt;
        }
        if (!editor_state.selected_polygon_vertices.empty()) {
            return SelectableObject{ SelectableObjectType::Polygon,
                                     editor_state.selected_polygon_vertices.front().first };
        }
        if (!editor_state.selected_scenery_ids.empty()) {
            return SelectableObject{ SelectableObjectType::Scenery,
                                     editor_state.selected_scenery_ids.front() };
        }
        if (!editor_state.selected_spawn_point_ids.empty()) {
            return SelectableObject{ SelectableObjectType::SpawnPoint,
                                     editor_state.selected_spawn_point_ids.front() };
        }
        return SelectableObject{ SelectableObjectType::Soldier,
                                 editor_state.selected_soldier_ids.front() };
    }

    static bool IsSelected(const SelectableObject& object, const ClientState& client_state)
    {
        const auto& editor_state = client_state.map_editor_state;
        switch (object.type) {
            case SelectableObjectType::Polygon:
                return std::ranges::any_of(
                  editor_state.selected_polygon_vertices,
                  [&](const auto& selection) { return selection.first == object.id; });
            case SelectableObjectType::Scenery:
                return std::ranges::contains(editor_state.selected_scenery_ids, object.id);
            case SelectableObjectType::SpawnPoint:
                return std::ranges::contains(editor_state.selected_spawn_point_ids, object.id);
            case SelectableObjectType::Soldier:
                return std::ranges::contains(editor_state.selected_soldier_ids, object.id);
        }
        return false;
    }

    static void SelectObject(const SelectableObject& object,
                             ClientState& client_state,
                             const StateManager& game_state_manager)
    {
        auto& editor_state = client_state.map_editor_state;
        switch (object.type) {
            case SelectableObjectType::Polygon: {
                editor_state.selected_polygon_vertices.push_back({ object.id, { 0b111 } });
                editor_state.event_polygon_selected.Notify(
                  game_state_manager.GetConstMap().GetPolygons().at(object.id), { 0b111 });
                break;
            }
            case SelectableObjectType::Scenery:
                editor_state.selected_scenery_ids.push_back(object.id);
                break;
            case SelectableObjectType::SpawnPoint:
                editor_state.selected_spawn_point_ids.push_back(object.id);
                break;
            case SelectableObjectType::Soldier:
                editor_state.selected_soldier_ids.push_back(object.id);
                break;
        }
    }

    static void DeselectObject(const SelectableObject& object,
                               ClientState& client_state,
                               const StateManager& game_state_manager)
    {
        auto& editor_state = client_state.map_editor_state;
        switch (object.type) {
            case SelectableObjectType::Polygon: {
                std::erase_if(editor_state.selected_polygon_vertices,
                              [&](const auto& selection) { return selection.first == object.id; });
                editor_state.event_polygon_selected.Notify(
                  game_state_manager.GetConstMap().GetPolygons().at(object.id), { 0b000 });
                break;
            }
            case SelectableObjectType::Scenery:
                std::erase(editor_state.selected_scenery_ids, object.id);
                break;
            case SelectableObjectType::SpawnPoint:
                std::erase(editor_state.selected_spawn_point_ids, object.id);
                break;
            case SelectableObjectType::Soldier:
                std::erase(editor_state.selected_soldier_ids, object.id);
                break;
        }
    }

    static void ClearSelection(ClientState& client_state, const StateManager& game_state_manager)
    {
        auto& editor_state = client_state.map_editor_state;
        for (const auto& polygon : game_state_manager.GetConstMap().GetPolygons()) {
            editor_state.event_polygon_selected.Notify(polygon, { 0b000 });
        }
        editor_state.selected_polygon_vertices.clear();
        editor_state.selected_scenery_ids.clear();
        editor_state.selected_spawn_point_ids.clear();
        editor_state.selected_soldier_ids.clear();
    }

    std::optional<SelectableObject> FindSelectedObjectAtCursorForRemoval(
      const ClientState& client_state,
      const StateManager& game_state_manager) const
    {
        const auto& editor_state = client_state.map_editor_state;
        const auto& map = game_state_manager.GetConstMap();
        for (auto iterator = editor_state.selected_soldier_ids.rbegin();
             iterator != editor_state.selected_soldier_ids.rend();
             ++iterator) {
            if (IsMouseInSoldier(game_state_manager.GetSoldier(*iterator).particle.position)) {
                return SelectableObject{ SelectableObjectType::Soldier, *iterator };
            }
        }
        for (auto iterator = editor_state.selected_spawn_point_ids.rbegin();
             iterator != editor_state.selected_spawn_point_ids.rend();
             ++iterator) {
            const auto& spawn_point = map.GetSpawnPoints().at(*iterator);
            if (IsMouseInSpawnPoint(client_state, { spawn_point.x, spawn_point.y })) {
                return SelectableObject{ SelectableObjectType::SpawnPoint, *iterator };
            }
        }
        for (auto iterator = editor_state.selected_scenery_ids.rbegin();
             iterator != editor_state.selected_scenery_ids.rend();
             ++iterator) {
            if (Map::PointInScenery(mouse_map_position_, map.GetSceneryInstances().at(*iterator))) {
                return SelectableObject{ SelectableObjectType::Scenery, *iterator };
            }
        }
        for (auto iterator = editor_state.selected_polygon_vertices.rbegin();
             iterator != editor_state.selected_polygon_vertices.rend();
             ++iterator) {
            if (Map::PointInPoly(mouse_map_position_, map.GetPolygons().at(iterator->first))) {
                return SelectableObject{ SelectableObjectType::Polygon, iterator->first };
            }
        }
        return std::nullopt;
    }

    bool IsMouseInSpawnPoint(const ClientState& client_state,
                             const glm::vec2& spawn_point_position) const
    {
        float current_camera_zoom = client_state.camera.view.GetZoom();
        return Calc::SquareDistance(spawn_point_position, mouse_map_position_) /
                 current_camera_zoom <=
               64.0F * current_camera_zoom;
    }

    bool IsMouseInSoldier(const glm::vec2& soldier_position) const
    {
        float left = soldier_position.x - 9.0F;
        float right = soldier_position.x + 9.0F;
        float top = soldier_position.y - 25.0F;
        float bottom = soldier_position.y + 5.0F;
        return (mouse_map_position_.x >= left && mouse_map_position_.x <= right &&
                mouse_map_position_.y >= top && mouse_map_position_.y <= bottom);
    }

    void SetSelectionMode(SelectionMode new_selection_mode, ClientState& client_state)
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
                client_state.map_editor_state.current_tool_action_description =
                  "Remove from Selection";
                break;
            }
        }
    }

    glm::vec2 mouse_map_position_;

    SelectionMode current_selection_mode_;
};
} // namespace Soldank
