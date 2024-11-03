#include "map_editor/tools/TransformTool.hpp"
#include "core/map/PMSStructs.hpp"
#include "map_editor/actions/MoveSelectionMapEditorAction.hpp"
#include "map_editor/actions/RotateSelectionMapEditorAction.hpp"
#include "map_editor/actions/ScaleSelectionMapEditorAction.hpp"

#include "core/math/Calc.hpp"

#include <limits>

namespace Soldank
{
TransformTool::TransformTool(
  const std::function<void(std::unique_ptr<MapEditorAction>)>& add_new_map_editor_action,
  const std::function<void(MapEditorAction*)>& execute_without_adding_map_editor_action)
    : add_new_map_editor_action_(add_new_map_editor_action)
    , execute_without_adding_map_editor_action_(execute_without_adding_map_editor_action)
    , mouse_map_position_on_last_click_()
    , transform_mode_(TransformMode::Move)
{
}

void TransformTool::OnSelect(ClientState& client_state, const State& game_state)
{
    SetupSelectionBox(client_state, game_state);
    SetTransformMode(TransformMode::Move, client_state);
}

void TransformTool::OnUnselect(ClientState& client_state)
{
    client_state.map_editor_state.vertex_selection_box = std::nullopt;
}

void TransformTool::OnSceneLeftMouseButtonClick(ClientState& client_state, const State& game_state)
{
    switch (transform_mode_) {
        case TransformMode::Move: {
            std::vector<std::pair<std::pair<unsigned int, unsigned int>, glm::vec2>>
              polygon_vertices_with_position;
            for (const auto& [selected_polygon_id, selected_vertices] :
                 client_state.map_editor_state.selected_polygon_vertices) {
                for (unsigned int i = 0; i < 3; ++i) {
                    if (selected_vertices[i]) {
                        const auto& polygon = game_state.map.GetPolygons().at(selected_polygon_id);
                        polygon_vertices_with_position.push_back(
                          { { selected_polygon_id, i },
                            { polygon.vertices.at(i).x, polygon.vertices.at(i).y } });
                    }
                }
            }

            std::vector<std::pair<unsigned int, glm::vec2>> scenery_ids_with_position;
            for (const auto& selected_scenery_id :
                 client_state.map_editor_state.selected_scenery_ids) {
                glm::vec2 position = {
                    game_state.map.GetSceneryInstances().at(selected_scenery_id).x,
                    game_state.map.GetSceneryInstances().at(selected_scenery_id).y
                };
                scenery_ids_with_position.emplace_back(selected_scenery_id, position);
            }

            std::vector<std::pair<unsigned int, glm::ivec2>> spawn_point_ids_with_position;
            for (const auto& selected_spawn_point_id :
                 client_state.map_editor_state.selected_spawn_point_ids) {
                glm::ivec2 position = {
                    game_state.map.GetSpawnPoints().at(selected_spawn_point_id).x,
                    game_state.map.GetSpawnPoints().at(selected_spawn_point_id).y
                };
                spawn_point_ids_with_position.emplace_back(selected_spawn_point_id, position);
            }

            std::vector<std::pair<unsigned int, glm::vec2>> soldier_positions;
            soldier_positions.reserve(client_state.map_editor_state.selected_soldier_ids.size());
            for (const auto& selected_soldier_id :
                 client_state.map_editor_state.selected_soldier_ids) {
                for (const auto& soldier : game_state.soldiers) {
                    if (soldier.id != selected_soldier_id) {
                        continue;
                    }

                    soldier_positions.emplace_back(selected_soldier_id, soldier.particle.position);
                }
            }

            maybe_move_selection_action_ =
              std::make_unique<MoveSelectionMapEditorAction>(polygon_vertices_with_position,
                                                             scenery_ids_with_position,
                                                             spawn_point_ids_with_position,
                                                             soldier_positions);
            break;
        }
        case TransformMode::Scale: {
            if (!client_state.map_editor_state.vertex_selection_box) {
                break;
            }

            float max_distance = 5.0F * client_state.camera_component.GetZoom();

            if (Calc::SquareDistance(client_state.map_editor_state.vertex_selection_box->second,
                                     client_state.mouse_map_position) >
                max_distance * max_distance) {
                break;
            }

            std::vector<std::pair<std::pair<unsigned int, std::bitset<3>>, PMSPolygon>> polygons;
            polygons.reserve(client_state.map_editor_state.selected_polygon_vertices.size());
            for (const auto& selected_polygon_vertices :
                 client_state.map_editor_state.selected_polygon_vertices) {
                polygons.emplace_back(
                  selected_polygon_vertices,
                  game_state.map.GetPolygons().at(selected_polygon_vertices.first));
            }

            std::vector<std::pair<unsigned int, PMSScenery>> sceneries;
            sceneries.reserve(client_state.map_editor_state.selected_scenery_ids.size());
            for (const auto& selected_scenery_id :
                 client_state.map_editor_state.selected_scenery_ids) {
                sceneries.emplace_back(
                  selected_scenery_id,
                  game_state.map.GetSceneryInstances().at(selected_scenery_id));
            }

            std::vector<std::pair<unsigned int, PMSSpawnPoint>> spawn_points;
            spawn_points.reserve(client_state.map_editor_state.selected_spawn_point_ids.size());
            for (const auto& selected_spawn_point_id :
                 client_state.map_editor_state.selected_spawn_point_ids) {
                spawn_points.emplace_back(
                  selected_spawn_point_id,
                  game_state.map.GetSpawnPoints().at(selected_spawn_point_id));
            }

            std::vector<std::pair<unsigned int, glm::vec2>> soldier_positions;
            soldier_positions.reserve(client_state.map_editor_state.selected_soldier_ids.size());
            for (const auto& selected_soldier_id :
                 client_state.map_editor_state.selected_soldier_ids) {
                for (const auto& soldier : game_state.soldiers) {
                    if (soldier.id != selected_soldier_id) {
                        continue;
                    }

                    soldier_positions.emplace_back(selected_soldier_id, soldier.particle.position);
                }
            }

            glm::vec2 origin = client_state.map_editor_state.vertex_selection_box->first;

            maybe_scale_selection_action_ =
              std::make_unique<ScaleSelectionMapEditorAction>(polygons,
                                                              sceneries,
                                                              spawn_points,
                                                              soldier_positions,
                                                              origin,
                                                              client_state.mouse_map_position);

            break;
        }
        case TransformMode::Rotate: {
            if (!client_state.map_editor_state.vertex_selection_box) {
                break;
            }

            float max_distance = 5.0F * client_state.camera_component.GetZoom();

            if (Calc::SquareDistance(client_state.map_editor_state.vertex_selection_box->second,
                                     client_state.mouse_map_position) >
                max_distance * max_distance) {
                break;
            }

            std::vector<std::pair<std::pair<unsigned int, std::bitset<3>>, PMSPolygon>> polygons;
            polygons.reserve(client_state.map_editor_state.selected_polygon_vertices.size());
            for (const auto& selected_polygon_vertices :
                 client_state.map_editor_state.selected_polygon_vertices) {
                polygons.emplace_back(
                  selected_polygon_vertices,
                  game_state.map.GetPolygons().at(selected_polygon_vertices.first));
            }

            std::vector<std::pair<unsigned int, PMSScenery>> sceneries;
            sceneries.reserve(client_state.map_editor_state.selected_scenery_ids.size());
            for (const auto& selected_scenery_id :
                 client_state.map_editor_state.selected_scenery_ids) {
                sceneries.emplace_back(
                  selected_scenery_id,
                  game_state.map.GetSceneryInstances().at(selected_scenery_id));
            }

            std::vector<std::pair<unsigned int, PMSSpawnPoint>> spawn_points;
            spawn_points.reserve(client_state.map_editor_state.selected_spawn_point_ids.size());
            for (const auto& selected_spawn_point_id :
                 client_state.map_editor_state.selected_spawn_point_ids) {
                spawn_points.emplace_back(
                  selected_spawn_point_id,
                  game_state.map.GetSpawnPoints().at(selected_spawn_point_id));
            }

            std::vector<std::pair<unsigned int, glm::vec2>> soldier_positions;
            soldier_positions.reserve(client_state.map_editor_state.selected_soldier_ids.size());
            for (const auto& selected_soldier_id :
                 client_state.map_editor_state.selected_soldier_ids) {
                for (const auto& soldier : game_state.soldiers) {
                    if (soldier.id != selected_soldier_id) {
                        continue;
                    }

                    soldier_positions.emplace_back(selected_soldier_id, soldier.particle.position);
                }
            }

            glm::vec2 origin = client_state.map_editor_state.vertex_selection_box->first;

            maybe_rotate_selection_action_ =
              std::make_unique<RotateSelectionMapEditorAction>(polygons,
                                                               sceneries,
                                                               spawn_points,
                                                               soldier_positions,
                                                               origin,
                                                               client_state.mouse_map_position);

            break;
        }
    }

    mouse_map_position_on_last_click_ = client_state.mouse_map_position;
}

void TransformTool::OnSceneLeftMouseButtonRelease(ClientState& /*client_state*/,
                                                  const State& /*game_state*/)
{
    if (maybe_move_selection_action_) {
        add_new_map_editor_action_(std::move(*maybe_move_selection_action_));
        maybe_move_selection_action_ = std::nullopt;
    }

    if (maybe_scale_selection_action_) {
        add_new_map_editor_action_(std::move(*maybe_scale_selection_action_));
        maybe_scale_selection_action_ = std::nullopt;
    }

    if (maybe_rotate_selection_action_) {
        add_new_map_editor_action_(std::move(*maybe_rotate_selection_action_));
        maybe_rotate_selection_action_ = std::nullopt;
    }
}

void TransformTool::OnSceneRightMouseButtonClick(ClientState& client_state) {}

void TransformTool::OnSceneRightMouseButtonRelease() {}

void TransformTool::OnMouseScreenPositionChange(ClientState& client_state,
                                                glm::vec2 last_mouse_position,
                                                glm::vec2 new_mouse_position)
{
}

void TransformTool::OnMouseMapPositionChange(ClientState& client_state,
                                             glm::vec2 /*last_mouse_position*/,
                                             glm::vec2 new_mouse_position,
                                             const State& game_state)
{
    if (maybe_move_selection_action_) {
        glm::vec2 move_offset = new_mouse_position - mouse_map_position_on_last_click_;
        (*maybe_move_selection_action_)->SetMoveOffset(move_offset);
        execute_without_adding_map_editor_action_(maybe_move_selection_action_->get());
        SetupSelectionBox(client_state, game_state);
    }

    if (maybe_scale_selection_action_) {
        (*maybe_scale_selection_action_)->SetCurrentMousePosition(new_mouse_position);
        execute_without_adding_map_editor_action_(maybe_scale_selection_action_->get());
        SetupSelectionBox(client_state, game_state);
    }

    if (maybe_rotate_selection_action_) {
        (*maybe_rotate_selection_action_)->SetCurrentMousePosition(new_mouse_position);
        execute_without_adding_map_editor_action_(maybe_rotate_selection_action_->get());
        SetupSelectionBox(client_state, game_state);
    }
}

void TransformTool::OnModifierKey1Pressed(ClientState& client_state) {}

void TransformTool::OnModifierKey1Released(ClientState& client_state) {}

void TransformTool::OnModifierKey2Pressed(ClientState& client_state)
{
    SetTransformMode(TransformMode::Scale, client_state);
}

void TransformTool::OnModifierKey2Released(ClientState& client_state)
{
    SetTransformMode(TransformMode::Move, client_state);
}

void TransformTool::OnModifierKey3Pressed(ClientState& client_state)
{
    SetTransformMode(TransformMode::Rotate, client_state);
}

void TransformTool::OnModifierKey3Released(ClientState& client_state)
{
    SetTransformMode(TransformMode::Move, client_state);
}

void TransformTool::SetupSelectionBox(ClientState& client_state, const State& game_state)
{
    float left = std::numeric_limits<float>::infinity();
    float right = -std::numeric_limits<float>::infinity();
    float top = std::numeric_limits<float>::infinity();
    float bottom = -std::numeric_limits<float>::infinity();

    bool is_anything_selected = false;
    is_anything_selected |= !client_state.map_editor_state.selected_polygon_vertices.empty();
    is_anything_selected |= !client_state.map_editor_state.selected_scenery_ids.empty();
    is_anything_selected |= !client_state.map_editor_state.selected_spawn_point_ids.empty();

    if (is_anything_selected) {
        for (const auto& [polygon_id, vertices] :
             client_state.map_editor_state.selected_polygon_vertices) {

            const auto& polygon = game_state.map.GetPolygons().at(polygon_id);
            for (unsigned int i = 0; i < 3; ++i) {
                if (!vertices[i]) {
                    continue;
                }

                const auto& vertex = polygon.vertices.at(i);
                left = std::min(left, vertex.x);
                right = std::max(right, vertex.x);
                top = std::min(top, vertex.y);
                bottom = std::max(bottom, vertex.y);
            }
        }

        for (const auto& scenery_id : client_state.map_editor_state.selected_scenery_ids) {
            const auto& scenery = game_state.map.GetSceneryInstances().at(scenery_id);
            auto scenery_vertices = Map::GetSceneryVertexPositions(scenery);
            for (const auto& scenery_vertex : scenery_vertices) {
                left = std::min(left, scenery_vertex.x);
                right = std::max(right, scenery_vertex.x);
                top = std::min(top, scenery_vertex.y);
                bottom = std::max(bottom, scenery_vertex.y);
            }
        }

        for (const auto& spawn_point_id : client_state.map_editor_state.selected_spawn_point_ids) {
            const auto& spawn_point = game_state.map.GetSpawnPoints().at(spawn_point_id);
            float zoom = client_state.camera_component.GetZoom();
            glm::vec2 top_left_corner = { (float)spawn_point.x - 8.0F * zoom,
                                          (float)spawn_point.y - 8.0F * zoom };
            glm::vec2 bottom_right_corner = { (float)spawn_point.x + 8.0F * zoom,
                                              (float)spawn_point.y + 8.0F * zoom };

            left = std::min(left, top_left_corner.x);
            right = std::max(right, bottom_right_corner.x);
            top = std::min(top, top_left_corner.y);
            bottom = std::max(bottom, bottom_right_corner.y);
        }

        for (const auto& selected_soldier_id : client_state.map_editor_state.selected_soldier_ids) {
            for (const auto& soldier : game_state.soldiers) {
                if (soldier.id != selected_soldier_id) {
                    continue;
                }

                float soldier_left = soldier.particle.position.x - 9.0F;
                float soldier_right = soldier.particle.position.x + 9.0F;
                float soldier_top = soldier.particle.position.y - 25.0F;
                float soldier_bottom = soldier.particle.position.y + 5.0F;

                left = std::min(left, soldier_left);
                right = std::max(right, soldier_right);
                top = std::min(top, soldier_top);
                bottom = std::max(bottom, soldier_bottom);
            }
        }

        client_state.map_editor_state.vertex_selection_box = { { left, top }, { right, bottom } };
    } else {
        client_state.map_editor_state.vertex_selection_box = std::nullopt;
    }
}

void TransformTool::SetTransformMode(TransformMode new_transform_mode, ClientState& client_state)
{
    transform_mode_ = new_transform_mode;

    switch (new_transform_mode) {
        case TransformMode::Move: {
            client_state.map_editor_state.current_tool_action_description = "Move Selection";
            break;
        }
        case TransformMode::Scale: {
            client_state.map_editor_state.current_tool_action_description = "Scale Selection";
            break;
        }
        case TransformMode::Rotate: {
            client_state.map_editor_state.current_tool_action_description = "Rotate Selection";
            break;
        }
    }
}
} // namespace Soldank
