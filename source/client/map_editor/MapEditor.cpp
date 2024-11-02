#include "map_editor/MapEditor.hpp"

#include "core/map/PMSEnums.hpp"
#include "core/map/PMSStructs.hpp"
#include "map_editor/actions/AddObjectsMapEditorAction.hpp"
#include "map_editor/actions/RemoveSelectionMapEditorAction.hpp"
#include "map_editor/actions/TransformPolygonsMapEditorAction.hpp"
#include "map_editor/actions/TransformSceneriesMapEditorAction.hpp"
#include "map_editor/actions/TransformSpawnPointsMapEditorAction.hpp"
#include "map_editor/tools/ColorPickerTool.hpp"
#include "map_editor/tools/ColorTool.hpp"
#include "map_editor/tools/PolygonTool.hpp"
#include "map_editor/tools/SceneryTool.hpp"
#include "map_editor/tools/SelectionTool.hpp"
#include "map_editor/tools/SpawnpointTool.hpp"
#include "map_editor/tools/TextureTool.hpp"
#include "map_editor/tools/TransformTool.hpp"
#include "map_editor/tools/VertexColorTool.hpp"
#include "map_editor/tools/VertexSelectionTool.hpp"
#include "map_editor/tools/WaypointTool.hpp"

#include <GLFW/glfw3.h>

#include <memory>
#include <utility>

namespace Soldank
{
MapEditor::MapEditor(ClientState& client_state, State& game_state)
    : selected_tool_(client_state.map_editor_state.selected_tool)
    , current_mouse_screen_position_()
    , camera_position_on_start_dragging_()
    , mouse_screen_position_on_start_dragging_()
    , is_dragging_camera_(false)
    , locked_(false)
    , is_holding_left_ctrl_(false)
{
    add_new_map_editor_action_ =
      [this, &client_state, &game_state](std::unique_ptr<MapEditorAction> new_action) {
          ExecuteNewAction(client_state, game_state.map, std::move(new_action));
      };
    execute_without_adding_map_editor_action_ = [&client_state,
                                                 &game_state](MapEditorAction* new_action) {
        new_action->Execute(client_state, game_state.map);
        return new_action;
    };

    client_state.event_left_mouse_button_clicked.AddObserver([this, &client_state, &game_state]() {
        if (!client_state.map_editor_state.is_mouse_hovering_over_ui &&
            !client_state.map_editor_state.is_modal_or_popup_open) {
            OnSceneLeftMouseButtonClick(client_state, game_state);
        }
    });

    client_state.event_left_mouse_button_released.AddObserver([this, &client_state, &game_state]() {
        if (!client_state.map_editor_state.is_mouse_hovering_over_ui &&
            !client_state.map_editor_state.is_modal_or_popup_open) {
            OnSceneLeftMouseButtonRelease(client_state, game_state);
        }
    });

    client_state.event_right_mouse_button_clicked.AddObserver([this, &client_state]() {
        if (!client_state.map_editor_state.is_mouse_hovering_over_ui &&
            !client_state.map_editor_state.is_modal_or_popup_open) {
            OnSceneRightMouseButtonClick(client_state);
        }
    });

    client_state.event_right_mouse_button_released.AddObserver([this, &client_state]() {
        if (!client_state.map_editor_state.is_mouse_hovering_over_ui &&
            !client_state.map_editor_state.is_modal_or_popup_open) {
            OnSceneRightMouseButtonRelease();
        }
    });

    client_state.event_middle_mouse_button_clicked.AddObserver([this, &client_state]() {
        if (!client_state.map_editor_state.is_mouse_hovering_over_ui &&
            !client_state.map_editor_state.is_modal_or_popup_open) {
            OnSceneMiddleMouseButtonClick(client_state);
        }
    });

    client_state.event_middle_mouse_button_released.AddObserver([this, &client_state]() {
        if (!client_state.map_editor_state.is_mouse_hovering_over_ui &&
            !client_state.map_editor_state.is_modal_or_popup_open) {
            OnSceneMiddleMouseButtonRelease();
        }
    });

    client_state.event_mouse_screen_position_changed.AddObserver(
      [this, &client_state](glm::vec2 last_mouse_position, glm::vec2 new_mouse_position) {
          OnMouseScreenPositionChange(client_state, last_mouse_position, new_mouse_position);
      });

    client_state.event_mouse_map_position_changed.AddObserver([this, &client_state, &game_state](
                                                                glm::vec2 last_mouse_position,
                                                                glm::vec2 new_mouse_position) {
        OnMouseMapPositionChange(client_state, last_mouse_position, new_mouse_position, game_state);
    });

    client_state.event_mouse_wheel_scrolled_up.AddObserver([this, &client_state]() {
        if (!client_state.map_editor_state.is_modal_or_popup_open) {
            OnMouseScrollUp(client_state);
        }
    });
    client_state.event_mouse_wheel_scrolled_down.AddObserver([this, &client_state]() {
        if (!client_state.map_editor_state.is_modal_or_popup_open) {
            OnMouseScrollDown(client_state);
        }
    });

    client_state.event_key_pressed.AddObserver([this, &client_state, &game_state](int key) {
        if (!client_state.map_editor_state.is_modal_or_popup_open) {
            OnKeyPressed(key, client_state, game_state.map);
        }
    });
    client_state.event_key_released.AddObserver([this, &client_state](int key) {
        if (!client_state.map_editor_state.is_modal_or_popup_open) {
            OnKeyReleased(key, client_state);
        }
    });

    client_state.map_editor_state.event_selected_new_tool.AddObserver(
      [this, &client_state, &game_state](ToolType tool_type) {
          OnSelectNewTool(tool_type, client_state, game_state);
      });

    client_state.map_editor_state.event_pressed_undo.AddObserver(
      [this, &client_state, &game_state]() { UndoLastAction(client_state, game_state.map); });

    client_state.map_editor_state.event_pressed_redo.AddObserver(
      [this, &client_state, &game_state]() { RedoUndoneAction(client_state, game_state.map); });

    tools_.emplace_back(std::make_unique<TransformTool>(add_new_map_editor_action_,
                                                        execute_without_adding_map_editor_action_));
    tools_.emplace_back(std::make_unique<PolygonTool>(add_new_map_editor_action_));
    tools_.emplace_back(std::make_unique<VertexSelectionTool>());
    tools_.emplace_back(std::make_unique<SelectionTool>());
    tools_.emplace_back(std::make_unique<VertexColorTool>());
    tools_.emplace_back(std::make_unique<ColorTool>(add_new_map_editor_action_));
    tools_.emplace_back(std::make_unique<TextureTool>());
    tools_.emplace_back(std::make_unique<SceneryTool>(add_new_map_editor_action_));
    tools_.emplace_back(std::make_unique<WaypointTool>());
    tools_.emplace_back(std::make_unique<SpawnpointTool>(add_new_map_editor_action_));
    tools_.emplace_back(std::make_unique<ColorPickerTool>());

    for (int i = 0; auto& saved_color : client_state.map_editor_state.palette_saved_colors) {
        int row = i / 12;
        if ((row + (i % 2)) % 2 == 0) {
            saved_color = { 1.0F, 1.0F, 1.0F, 1.0F };
        } else {
            saved_color = { 0.683F, 0.98F, 1.0F, 1.0F };
        }
        ++i;
    }

    client_state.map_editor_state.event_selected_spawn_points_type_changed.AddObserver(
      [this, &client_state, &game_state](PMSSpawnPointType new_spawn_point_type) {
          OnChangeSelectedSpawnPointsTypes(new_spawn_point_type, client_state, game_state.map);
      });
    client_state.map_editor_state.event_selected_sceneries_level_changed.AddObserver(
      [this, &client_state, &game_state](int new_level) {
          OnChangeSelectedSceneriesLevel(new_level, client_state, game_state.map);
      });
    client_state.map_editor_state.event_selected_polygons_bounciness_changed.AddObserver(
      [this, &client_state, &game_state](float new_bounciness) {
          OnTransformSelectedPolygons(
            [new_bounciness](const PMSPolygon& old_polygon) {
                PMSPolygon new_polygon = old_polygon;
                new_polygon.bounciness = new_bounciness;
                return new_polygon;
            },
            client_state,
            game_state.map);
      });
    client_state.map_editor_state.event_selected_polygons_type_changed.AddObserver(
      [this, &client_state, &game_state](PMSPolygonType new_polygon_type) {
          OnTransformSelectedPolygons(
            [new_polygon_type](const PMSPolygon& old_polygon) {
                PMSPolygon new_polygon = old_polygon;
                new_polygon.polygon_type = new_polygon_type;
                return new_polygon;
            },
            client_state,
            game_state.map);
      });
}

void MapEditor::Lock()
{
    locked_ = true;
}

void MapEditor::Unlock()
{
    locked_ = false;
}

void MapEditor::OnSelectNewTool(ToolType tool_type,
                                ClientState& client_state,
                                const State& game_state)
{
    if (locked_) {
        return;
    }

    tools_.at(std::to_underlying(selected_tool_))->OnUnselect(client_state);
    selected_tool_ = tool_type;
    tools_.at(std::to_underlying(selected_tool_))->OnSelect(client_state, game_state);
}

void MapEditor::OnSceneLeftMouseButtonClick(ClientState& client_state, const State& game_state)
{
    if (locked_) {
        return;
    }

    tools_.at(std::to_underlying(selected_tool_))
      ->OnSceneLeftMouseButtonClick(client_state, game_state);
}

void MapEditor::OnSceneLeftMouseButtonRelease(ClientState& client_state, const State& game_state)
{
    if (locked_) {
        return;
    }

    tools_.at(std::to_underlying(selected_tool_))
      ->OnSceneLeftMouseButtonRelease(client_state, game_state);
}

void MapEditor::OnSceneRightMouseButtonClick(ClientState& client_state)
{
    if (locked_) {
        return;
    }

    tools_.at(std::to_underlying(selected_tool_))->OnSceneRightMouseButtonClick(client_state);
}

void MapEditor::OnSceneRightMouseButtonRelease()
{
    if (locked_) {
        return;
    }

    tools_.at(std::to_underlying(selected_tool_))->OnSceneRightMouseButtonRelease();
}

void MapEditor::OnSceneMiddleMouseButtonClick(ClientState& client_state)
{
    if (locked_) {
        return;
    }

    is_dragging_camera_ = true;
    mouse_screen_position_on_start_dragging_ = current_mouse_screen_position_;
    camera_position_on_start_dragging_ = client_state.camera;
}

void MapEditor::OnSceneMiddleMouseButtonRelease()
{
    if (locked_) {
        return;
    }

    is_dragging_camera_ = false;
}

void MapEditor::OnMouseScrollUp(ClientState& client_state) const
{
    if (locked_) {
        return;
    }

    client_state.camera_component.ZoomIn();
}

void MapEditor::OnMouseScrollDown(ClientState& client_state) const
{
    if (locked_) {
        return;
    }

    client_state.camera_component.ZoomOut();
}

void MapEditor::OnMouseScreenPositionChange(ClientState& client_state,
                                            glm::vec2 last_mouse_position,
                                            glm::vec2 new_mouse_position)
{
    if (locked_) {
        return;
    }

    new_mouse_position.y = -new_mouse_position.y;
    current_mouse_screen_position_ = new_mouse_position;

    if (is_dragging_camera_) {
        glm::vec2 mouse_position_difference =
          mouse_screen_position_on_start_dragging_ - new_mouse_position;

        // We need to scale the difference because the window size is different than the camera size
        mouse_position_difference.x /=
          client_state.window_width / client_state.camera_component.GetWidth();
        mouse_position_difference.y /=
          client_state.window_height / client_state.camera_component.GetHeight();

        client_state.camera = camera_position_on_start_dragging_ + mouse_position_difference;
    }

    tools_.at(std::to_underlying(selected_tool_))
      ->OnMouseScreenPositionChange(client_state, last_mouse_position, new_mouse_position);
}

void MapEditor::OnMouseMapPositionChange(ClientState& client_state,
                                         glm::vec2 last_mouse_position,
                                         glm::vec2 new_mouse_position,
                                         const State& game_state)
{
    if (locked_) {
        return;
    }

    tools_.at(std::to_underlying(selected_tool_))
      ->OnMouseMapPositionChange(client_state, last_mouse_position, new_mouse_position, game_state);
}

void MapEditor::OnKeyPressed(int key, ClientState& client_state, Map& map)
{
    static std::vector<std::pair<int, ToolType>> key_to_tool_type_map = {
        { GLFW_KEY_A, ToolType::Transform },       { GLFW_KEY_Q, ToolType::Polygon },
        { GLFW_KEY_S, ToolType::VertexSelection }, { GLFW_KEY_W, ToolType::Selection },
        { GLFW_KEY_D, ToolType::VertexColor },     { GLFW_KEY_E, ToolType::Color },
        { GLFW_KEY_F, ToolType::Texture },         { GLFW_KEY_R, ToolType::Scenery },
        { GLFW_KEY_G, ToolType::Waypoint },        { GLFW_KEY_T, ToolType::Spawnpoint },
        { GLFW_KEY_H, ToolType::ColorPicker },
    };

    for (const auto& key_to_tool_type : key_to_tool_type_map) {
        if (key == key_to_tool_type.first) {
            if (client_state.map_editor_state.selected_tool != key_to_tool_type.second) {
                client_state.map_editor_state.event_selected_new_tool.Notify(
                  key_to_tool_type.second);
            }
            client_state.map_editor_state.selected_tool = key_to_tool_type.second;
            return;
        }
    }

    bool is_anything_selected = false;

    is_anything_selected |= !client_state.map_editor_state.selected_polygon_vertices.empty();
    is_anything_selected |= !client_state.map_editor_state.selected_scenery_ids.empty();
    is_anything_selected |= !client_state.map_editor_state.selected_spawn_point_ids.empty();

    if (is_holding_left_ctrl_ && is_anything_selected && key == GLFW_KEY_C) {
        // TODO: make use of system's clipboard
        copied_polygons_.clear();
        copied_sceneries_.clear();
        copied_spawn_points_.clear();

        for (const auto& [selected_polygon_id, selected_vertices] :
             client_state.map_editor_state.selected_polygon_vertices) {
            if (!selected_vertices.all()) {
                continue;
            }

            copied_polygons_.push_back(map.GetPolygons().at(selected_polygon_id));
        }

        for (const auto& selected_scenery_id : client_state.map_editor_state.selected_scenery_ids) {
            const auto& scenery = map.GetSceneryInstances().at(selected_scenery_id);
            copied_sceneries_.push_back(
              { selected_scenery_id,
                { scenery, map.GetSceneryTypes().at(scenery.style - 1).name } });
        }

        for (const auto& selected_spawn_point_id :
             client_state.map_editor_state.selected_spawn_point_ids) {
            copied_spawn_points_.emplace_back(selected_spawn_point_id,
                                              map.GetSpawnPoints().at(selected_spawn_point_id));
        }

        return;
    }

    if (is_holding_left_ctrl_ && key == GLFW_KEY_V) {
        if (!copied_polygons_.empty() || !copied_sceneries_.empty() ||
            !copied_spawn_points_.empty()) {

            std::unique_ptr<AddObjectsMapEditorAction> add_copied_objects_action =
              std::make_unique<AddObjectsMapEditorAction>(
                copied_polygons_, copied_sceneries_, copied_spawn_points_);
            ExecuteNewAction(client_state, map, std::move(add_copied_objects_action));

            return;
        }
    }

    if (key == GLFW_KEY_LEFT_SHIFT) {
        tools_.at(std::to_underlying(selected_tool_))->OnModifierKey1Pressed();
    }
    if (key == GLFW_KEY_LEFT_CONTROL) {
        is_holding_left_ctrl_ = true;
        tools_.at(std::to_underlying(selected_tool_))->OnModifierKey2Pressed();
    }
    if (key == GLFW_KEY_LEFT_ALT) {
        tools_.at(std::to_underlying(selected_tool_))->OnModifierKey3Pressed();
    }

    if (key == GLFW_KEY_DELETE) {
        RemoveCurrentSelection(client_state, map);
    }
}

void MapEditor::OnKeyReleased(int key, ClientState& /*client_state*/)
{
    if (key == GLFW_KEY_LEFT_SHIFT) {
        tools_.at(std::to_underlying(selected_tool_))->OnModifierKey1Released();
    }
    if (key == GLFW_KEY_LEFT_CONTROL) {
        is_holding_left_ctrl_ = false;
        tools_.at(std::to_underlying(selected_tool_))->OnModifierKey2Released();
    }
    if (key == GLFW_KEY_LEFT_ALT) {
        tools_.at(std::to_underlying(selected_tool_))->OnModifierKey3Released();
    }
}

void MapEditor::ExecuteNewAction(ClientState& client_state,
                                 Map& map,
                                 std::unique_ptr<MapEditorAction> new_action)
{
    if (locked_) {
        return;
    }

    map_editor_undone_actions_.clear();
    new_action->Execute(client_state, map);
    map_editor_executed_actions_.push_back(std::move(new_action));
    if (map_editor_executed_actions_.size() > ACTION_HISTORY_LIMIT) {
        map_editor_executed_actions_.pop_front();
    }

    UpdateUndoRedoButtons(client_state);
}

void MapEditor::UndoLastAction(ClientState& client_state, Map& map)
{
    if (locked_) {
        return;
    }

    if (!map_editor_executed_actions_.empty()) {
        map_editor_executed_actions_.back()->Undo(client_state, map);
        map_editor_undone_actions_.push_back(std::move(map_editor_executed_actions_.back()));
        map_editor_executed_actions_.pop_back();
    }

    UpdateUndoRedoButtons(client_state);
}

void MapEditor::RedoUndoneAction(ClientState& client_state, Map& map)
{
    if (locked_) {
        return;
    }

    if (!map_editor_undone_actions_.empty()) {
        map_editor_executed_actions_.push_back(std::move(map_editor_undone_actions_.back()));
        map_editor_undone_actions_.pop_back();
        map_editor_executed_actions_.back()->Execute(client_state, map);
    }

    UpdateUndoRedoButtons(client_state);
}

void MapEditor::UpdateUndoRedoButtons(ClientState& client_state)
{
    client_state.map_editor_state.is_undo_enabled = !map_editor_executed_actions_.empty();
    client_state.map_editor_state.is_redo_enabled = !map_editor_undone_actions_.empty();
}

void MapEditor::RemoveCurrentSelection(ClientState& client_state, Map& map)
{
    std::unique_ptr<RemoveSelectionMapEditorAction> remove_selection_action =
      std::make_unique<RemoveSelectionMapEditorAction>(client_state, map);
    ExecuteNewAction(client_state, map, std::move(remove_selection_action));
}

void MapEditor::OnChangeSelectedSpawnPointsTypes(PMSSpawnPointType new_spawn_point_type,
                                                 ClientState& client_state,
                                                 Map& map)
{
    std::vector<std::pair<unsigned int, PMSSpawnPoint>> selected_spawn_points;
    selected_spawn_points.reserve(client_state.map_editor_state.selected_spawn_point_ids.size());
    for (unsigned int selected_spawn_point_id :
         client_state.map_editor_state.selected_spawn_point_ids) {
        selected_spawn_points.emplace_back(selected_spawn_point_id,
                                           map.GetSpawnPoints().at(selected_spawn_point_id));
    }
    std::unique_ptr<TransformSpawnPointsMapEditorAction> transform_spawn_points_action =
      std::make_unique<TransformSpawnPointsMapEditorAction>(
        selected_spawn_points, [new_spawn_point_type](const PMSSpawnPoint& old_spawn_point) {
            PMSSpawnPoint new_spawn_point = old_spawn_point;
            new_spawn_point.type = new_spawn_point_type;
            return new_spawn_point;
        });
    ExecuteNewAction(client_state, map, std::move(transform_spawn_points_action));
}

void MapEditor::OnChangeSelectedSceneriesLevel(int new_level, ClientState& client_state, Map& map)
{
    std::vector<std::pair<unsigned int, PMSScenery>> selected_sceneries;
    selected_sceneries.reserve(client_state.map_editor_state.selected_scenery_ids.size());
    for (unsigned int selected_scenery_id : client_state.map_editor_state.selected_scenery_ids) {
        selected_sceneries.emplace_back(selected_scenery_id,
                                        map.GetSceneryInstances().at(selected_scenery_id));
    }
    std::unique_ptr<TransformSceneriesMapEditorAction> transform_sceneries_action =
      std::make_unique<TransformSceneriesMapEditorAction>(
        selected_sceneries, [new_level](const PMSScenery& old_scenery) {
            PMSScenery new_scenery = old_scenery;
            new_scenery.level = new_level;
            return new_scenery;
        });
    ExecuteNewAction(client_state, map, std::move(transform_sceneries_action));
}

void MapEditor::OnTransformSelectedPolygons(
  const std::function<PMSPolygon(const PMSPolygon&)>& transform_function,
  ClientState& client_state,
  Map& map)
{
    std::vector<std::pair<unsigned int, PMSPolygon>> selected_polygons;
    selected_polygons.reserve(client_state.map_editor_state.selected_polygon_vertices.size());
    for (const auto& selected_polygon_vertices :
         client_state.map_editor_state.selected_polygon_vertices) {
        selected_polygons.emplace_back(selected_polygon_vertices.first,
                                       map.GetPolygons().at(selected_polygon_vertices.first));
    }
    std::unique_ptr<TransformPolygonsMapEditorAction> transform_polygons_action =
      std::make_unique<TransformPolygonsMapEditorAction>(selected_polygons, transform_function);
    ExecuteNewAction(client_state, map, std::move(transform_polygons_action));
}
} // namespace Soldank
