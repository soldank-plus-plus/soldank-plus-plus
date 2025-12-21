module;

#include "core/state/StateManager.hpp"
#include "core/map/PMSStructs.hpp"
#include "core/math/Glm.hpp"

#include <memory>

export module ColorTool;

import Tool;
import MapEditorAction;
import ColorObjectsMapEditorAction;
import ClientState;

export namespace Soldank
{
class ColorTool final : public Tool
{
public:
    ColorTool(
      const std::function<void(std::unique_ptr<MapEditorAction>)>& add_new_map_editor_action)
        : add_new_map_editor_action_(add_new_map_editor_action)
        , mouse_map_position_()
    {
    }

    ~ColorTool() final = default;

    void OnSelect(ClientState& client_state, const StateManager& /*game_state_manager*/) final
    {
        client_state.map_editor_state.current_tool_action_description = "Color objects";
    }

    void OnUnselect(ClientState& /* client_state */) final {}

    void OnSceneLeftMouseButtonClick(ClientState& client_state,
                                     const StateManager& game_state_manager) final
    {
        bool is_anything_selected = false;

        is_anything_selected |= !client_state.map_editor_state.selected_polygon_vertices.empty();
        is_anything_selected |= !client_state.map_editor_state.selected_scenery_ids.empty();

        std::vector<std::pair<std::pair<unsigned int, unsigned int>, PMSColor>>
          polygon_vertices_to_color;
        std::vector<std::pair<unsigned int, PMSColor>> scenery_ids_to_color;

        if (is_anything_selected) {
            for (const auto& polygon_vertices :
                 client_state.map_editor_state.selected_polygon_vertices) {
                for (unsigned int i = 0; i < 3; ++i) {
                    if (polygon_vertices.second[i]) {
                        polygon_vertices_to_color.push_back({ { polygon_vertices.first, i },
                                                              game_state_manager.GetConstMap()
                                                                .GetPolygons()
                                                                .at(polygon_vertices.first)
                                                                .vertices.at(i)
                                                                .color });
                    }
                }
            }
            for (const auto& scenery_id : client_state.map_editor_state.selected_scenery_ids) {
                scenery_ids_to_color.emplace_back(
                  scenery_id,
                  game_state_manager.GetConstMap().GetSceneryInstances().at(scenery_id).color);
            }
        } else {
            for (unsigned int polygon_id = 0;
                 polygon_id < game_state_manager.GetConstMap().GetPolygonsCount();
                 ++polygon_id) {
                if (Map::PointInPoly(
                      mouse_map_position_,
                      game_state_manager.GetConstMap().GetPolygons().at(polygon_id))) {
                    for (unsigned int vertex_id = 0; vertex_id < 3; ++vertex_id) {
                        polygon_vertices_to_color.push_back({ { polygon_id, vertex_id },
                                                              game_state_manager.GetConstMap()
                                                                .GetPolygons()
                                                                .at(polygon_id)
                                                                .vertices.at(vertex_id)
                                                                .color });
                    }
                }
            }
            for (unsigned int i = 0;
                 i < game_state_manager.GetConstMap().GetSceneryInstances().size();
                 ++i) {
                if (Map::PointInScenery(
                      mouse_map_position_,
                      game_state_manager.GetConstMap().GetSceneryInstances().at(i))) {
                    scenery_ids_to_color.emplace_back(
                      i, game_state_manager.GetConstMap().GetSceneryInstances().at(i).color);
                }
            }
        }

        if (scenery_ids_to_color.empty() && polygon_vertices_to_color.empty()) {
            return;
        }

        PMSColor palette_color(
          (unsigned char)(client_state.map_editor_state.palette_current_color.at(0) * 255.0F),
          (unsigned char)(client_state.map_editor_state.palette_current_color.at(1) * 255.0F),
          (unsigned char)(client_state.map_editor_state.palette_current_color.at(2) * 255.0F),
          (unsigned char)(client_state.map_editor_state.palette_current_color.at(3) * 255.0F));

        std::unique_ptr<ColorObjectsMapEditorAction> color_objects_action =
          std::make_unique<ColorObjectsMapEditorAction>(
            palette_color, polygon_vertices_to_color, scenery_ids_to_color);
        add_new_map_editor_action_(std::move(color_objects_action));
    }

    void OnSceneLeftMouseButtonRelease(ClientState& /* client_state */,
                                       const StateManager& /* game_state_manager */) final
    {
    }

    void OnSceneRightMouseButtonClick(ClientState& /* client_state */) final {}

    void OnSceneRightMouseButtonRelease() final {}

    void OnMouseScreenPositionChange(ClientState& /* client_state */,
                                     glm::vec2 /* last_mouse_position*/,
                                     glm::vec2 /* new_mouse_position */) final
    {
    }

    void OnMouseMapPositionChange(ClientState& /*client_state*/,
                                  glm::vec2 /*last_mouse_position*/,
                                  glm::vec2 new_mouse_position,
                                  const StateManager& /*game_state_manager*/) final
    {
        mouse_map_position_ = new_mouse_position;
    }

    void OnModifierKey1Pressed(ClientState& /* client_state */) final {}

    void OnModifierKey1Released(ClientState& /* client_state */) final {}

    void OnModifierKey2Pressed(ClientState& /* client_state */) final {}

    void OnModifierKey2Released(ClientState& /* client_state */) final {}

    void OnModifierKey3Pressed(ClientState& /* client_state */) final {}

    void OnModifierKey3Released(ClientState& /* client_state */) final {}

private:
    std::function<void(std::unique_ptr<MapEditorAction>)> add_new_map_editor_action_;
    glm::vec2 mouse_map_position_;
};
} // namespace Soldank
