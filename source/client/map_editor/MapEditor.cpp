module;

#include <GLFW/glfw3.h>

#include <memory>
#include <string>
#include <utility>
#include <functional>
#include <filesystem>
#include <optional>
#include <vector>

export module MapEditor;

import Extern.Glm;

import Tool;
import MapEditorAction;
import MapEditor.EditorCommandHistory;
import MapEditor.Config;
import MapEditor.EditorDocument;
import MapEditor.EditorEventRouter;
import MapEditor.EditorMapProperties;
import MapEditor.EditorShortcutController;
import MapEditor.EditorToolController;
import MapEditor.EditorViewportController;
import ClientState;
import MapEditorState;
import Application.Input.Shortcut;
import AddObjectsMapEditorAction;
import RemoveSelectionMapEditorAction;
import TransformPolygonsMapEditorAction;
import TransformSceneriesMapEditorAction;
import TransformSpawnPointsMapEditorAction;

import Shared.Core.State.StateManager;
import Shared.Core.Map.PMSEnums;
import Shared.Core.Map.PMSStructs;

export namespace Soldank
{
class MapEditor
{
public:
    MapEditor(ClientState& client_state,
              StateManager& game_state_manager,
              std::filesystem::path config_file_path = "map_editor.toml");

    void Lock();
    void Unlock();

private:
    void OnSelectNewTool(ToolType tool_type,
                         ClientState& client_state,
                         const StateManager& game_state_manager);
    void OnSceneLeftMouseButtonClick(ClientState& client_state,
                                     const StateManager& game_state_manager);
    void OnSceneLeftMouseButtonRelease(ClientState& client_state,
                                       const StateManager& game_state_manager);
    void OnSceneRightMouseButtonClick(ClientState& client_state);
    void OnSceneRightMouseButtonRelease();
    void OnSceneMiddleMouseButtonClick(ClientState& client_state);
    void OnSceneMiddleMouseButtonRelease();
    void OnMouseScrollUp(ClientState& client_state) const;
    void OnMouseScrollDown(ClientState& client_state) const;
    void OnMouseScreenPositionChange(ClientState& client_state,
                                     glm::vec2 last_mouse_position,
                                     glm::vec2 new_mouse_position);
    void OnMouseMapPositionChange(ClientState& client_state,
                                  glm::vec2 last_mouse_position,
                                  glm::vec2 new_mouse_position,
                                  const StateManager& game_state_manager);

    void OnKeyPressed(int key,
                      int modifiers,
                      ClientState& client_state,
                      StateManager& game_state_manager);
    void OnKeyReleased(int key, ClientState& client_state);

    void ExecuteNewAction(ClientState& client_state,
                          StateManager& game_state_manager,
                          std::unique_ptr<MapEditorAction> new_action);
    void UndoLastAction(ClientState& client_state, StateManager& game_state_manager);
    void RedoUndoneAction(ClientState& client_state, StateManager& game_state_manager);
    void RemoveCurrentSelection(ClientState& client_state, StateManager& game_state_manager);
    void CopySelection(ClientState& client_state, const StateManager& game_state_manager);
    void PasteSelection(ClientState& client_state, StateManager& game_state_manager);

    void OnChangeSelectedSpawnPointsTypes(PMSSpawnPointType new_spawn_point_type,
                                          ClientState& client_state,
                                          StateManager& game_state_manager);
    void OnChangeSelectedSceneriesLevel(int new_level,
                                        ClientState& client_state,
                                        StateManager& game_state_manager);
    void OnTransformSelectedPolygons(
      const std::function<PMSPolygon(const PMSPolygon&)>& transform_function,
      ClientState& client_state,
      StateManager& game_state_manager);

    std::function<void(std::unique_ptr<MapEditorAction>)> add_new_map_editor_action_;
    std::function<void(MapEditorAction*)> execute_without_adding_map_editor_action_;
    EditorDocument document_;
    EditorCommandHistory command_history_;
    EditorEventRouter event_router_;
    EditorMapProperties map_properties_;
    std::unique_ptr<EditorToolController> tool_controller_;
    EditorShortcutController shortcut_controller_;
    EditorViewportController viewport_controller_;
    std::filesystem::path config_file_path_;

    bool locked_;

    std::vector<PMSPolygon> copied_polygons_;
    std::vector<std::pair<unsigned int, std::pair<PMSScenery, std::string>>> copied_sceneries_;
    std::vector<std::pair<unsigned int, PMSSpawnPoint>> copied_spawn_points_;
};
} // namespace Soldank

namespace Soldank
{
MapEditor::MapEditor(ClientState& client_state,
                     StateManager& game_state_manager,
                     std::filesystem::path config_file_path)
    : document_(client_state, game_state_manager)
    , map_properties_(client_state.map_editor_state, game_state_manager)
    , config_file_path_(std::move(config_file_path))
    , locked_(false)
{
    add_new_map_editor_action_ =
      [this, &client_state, &game_state_manager](std::unique_ptr<MapEditorAction> new_action) {
          ExecuteNewAction(client_state, game_state_manager, std::move(new_action));
      };
    execute_without_adding_map_editor_action_ = [&client_state,
                                                 &game_state_manager](MapEditorAction* new_action) {
        new_action->Execute(client_state, game_state_manager);
        return new_action;
    };
    tool_controller_ = std::make_unique<EditorToolController>(
      add_new_map_editor_action_, execute_without_adding_map_editor_action_);

    client_state.event_left_mouse_button_clicked.AddObserver(
      [this, &client_state, &game_state_manager]() {
          if (!client_state.map_editor_state.is_mouse_hovering_over_ui &&
              !client_state.map_editor_state.is_modal_or_popup_open) {
              OnSceneLeftMouseButtonClick(client_state, game_state_manager);
          }
      });

    client_state.event_left_mouse_button_released.AddObserver(
      [this, &client_state, &game_state_manager]() {
          if (!client_state.map_editor_state.is_mouse_hovering_over_ui &&
              !client_state.map_editor_state.is_modal_or_popup_open) {
              OnSceneLeftMouseButtonRelease(client_state, game_state_manager);
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

    client_state.event_mouse_map_position_changed.AddObserver(
      [this, &client_state, &game_state_manager](glm::vec2 last_mouse_position,
                                                 glm::vec2 new_mouse_position) {
          OnMouseMapPositionChange(
            client_state, last_mouse_position, new_mouse_position, game_state_manager);
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

    client_state.event_key_pressed.AddObserver(
      [this, &client_state, &game_state_manager](int key, int modifiers) {
          shortcut_controller_.OnKeyPressed(key);
          const int tool_capture_index = client_state.map_editor_state.tool_shortcut_capture_index;
          if (tool_capture_index >= 0) {
              if (IsShortcutModifierKey(key)) {
                  client_state.map_editor_state.shortcut_capture_modifiers = modifiers;
                  return;
              }
              client_state.map_editor_state.GetToolShortcut(
                static_cast<std::size_t>(tool_capture_index)) =
                key == GLFW_KEY_ESCAPE ? GLFW_KEY_UNKNOWN : EncodeShortcut(key, modifiers);
              client_state.map_editor_state.tool_shortcut_capture_index = -1;
              client_state.map_editor_state.shortcut_capture_modifiers = 0;
              client_state.map_editor_state.event_shortcuts_changed.Notify();
              return;
          }
          if (client_state.map_editor_state.is_play_mode_shortcut_capture_active) {
              if (IsShortcutModifierKey(key)) {
                  client_state.map_editor_state.shortcut_capture_modifiers = modifiers;
                  return;
              }
              client_state.map_editor_state.GetPlayModeShortcut() =
                key == GLFW_KEY_ESCAPE ? GLFW_KEY_UNKNOWN : EncodeShortcut(key, modifiers);
              client_state.map_editor_state.is_play_mode_shortcut_capture_active = false;
              client_state.map_editor_state.shortcut_capture_modifiers = 0;
              client_state.map_editor_state.event_shortcuts_changed.Notify();
              return;
          }

          if (!client_state.map_editor_state.is_modal_or_popup_open) {
              OnKeyPressed(key, modifiers, client_state, game_state_manager);
          }
      });
    client_state.event_key_released.AddObserver([this, &client_state](int key, int modifiers) {
        if (client_state.map_editor_state.tool_shortcut_capture_index >= 0 ||
            client_state.map_editor_state.is_play_mode_shortcut_capture_active) {
            client_state.map_editor_state.shortcut_capture_modifiers = modifiers;
        }
        shortcut_controller_.OnKeyReleased(key);
        if (!client_state.map_editor_state.is_modal_or_popup_open) {
            OnKeyReleased(key, client_state);
        }
    });

    client_state.map_editor_state.event_selected_new_tool.AddObserver(
      [this, &client_state, &game_state_manager](ToolType tool_type) {
          OnSelectNewTool(tool_type, client_state, game_state_manager);
      });

    client_state.map_editor_state.event_pressed_undo.AddObserver(
      [this, &client_state, &game_state_manager]() {
          UndoLastAction(client_state, game_state_manager);
      });

    client_state.map_editor_state.event_pressed_redo.AddObserver(
      [this, &client_state, &game_state_manager]() {
          RedoUndoneAction(client_state, game_state_manager);
      });
    client_state.map_editor_state.event_pressed_copy.AddObserver(
      [this, &client_state, &game_state_manager]() {
          CopySelection(client_state, game_state_manager);
      });
    client_state.map_editor_state.event_pressed_paste.AddObserver(
      [this, &client_state, &game_state_manager]() {
          PasteSelection(client_state, game_state_manager);
      });

    for (int i = 0; auto& saved_color : client_state.map_editor_state.palette_saved_colors) {
        int row = i / 12;
        if ((row + (i % 2)) % 2 == 0) {
            saved_color = { 1.0F, 1.0F, 1.0F, 1.0F };
        } else {
            saved_color = { 0.683F, 0.98F, 1.0F, 1.0F };
        }
        ++i;
    }

    MapEditorConfig::LoadSettings(config_file_path_,
                                  client_state.map_editor_state.palette_saved_colors,
                                  client_state.map_editor_state.GetPlayModeShortcut(),
                                  client_state.map_editor_state.ui_scale,
                                  client_state.map_editor_state.GetToolShortcuts());
    client_state.map_editor_state.pending_ui_scale = client_state.map_editor_state.ui_scale;
    client_state.map_editor_state.event_palette_saved_colors_changed.AddObserver(
      [this, &client_state]() {
          MapEditorConfig::SaveSettings(config_file_path_,
                                        client_state.map_editor_state.palette_saved_colors,
                                        client_state.map_editor_state.GetPlayModeShortcut(),
                                        client_state.map_editor_state.ui_scale,
                                        client_state.map_editor_state.GetToolShortcuts());
      });
    client_state.map_editor_state.event_shortcuts_changed.AddObserver([this, &client_state]() {
        MapEditorConfig::SaveSettings(config_file_path_,
                                      client_state.map_editor_state.palette_saved_colors,
                                      client_state.map_editor_state.GetPlayModeShortcut(),
                                      client_state.map_editor_state.ui_scale,
                                      client_state.map_editor_state.GetToolShortcuts());
    });
    client_state.map_editor_state.event_ui_scale_changed.AddObserver([this, &client_state]() {
        MapEditorConfig::SaveSettings(config_file_path_,
                                      client_state.map_editor_state.palette_saved_colors,
                                      client_state.map_editor_state.GetPlayModeShortcut(),
                                      client_state.map_editor_state.ui_scale,
                                      client_state.map_editor_state.GetToolShortcuts());
    });

    client_state.map_editor_state.event_selected_spawn_points_type_changed.AddObserver(
      [this, &client_state, &game_state_manager](PMSSpawnPointType new_spawn_point_type) {
          OnChangeSelectedSpawnPointsTypes(new_spawn_point_type, client_state, game_state_manager);
      });
    client_state.map_editor_state.event_selected_sceneries_level_changed.AddObserver(
      [this, &client_state, &game_state_manager](int new_level) {
          OnChangeSelectedSceneriesLevel(new_level, client_state, game_state_manager);
      });
    client_state.map_editor_state.event_selected_polygons_bounciness_changed.AddObserver(
      [this, &client_state, &game_state_manager](float new_bounciness) {
          OnTransformSelectedPolygons(
            [new_bounciness](const PMSPolygon& old_polygon) {
                PMSPolygon new_polygon = old_polygon;
                new_polygon.bounciness = new_bounciness;
                return new_polygon;
            },
            client_state,
            game_state_manager);
      });
    client_state.map_editor_state.event_selected_polygons_type_changed.AddObserver(
      [this, &client_state, &game_state_manager](PMSPolygonType new_polygon_type) {
          OnTransformSelectedPolygons(
            [new_polygon_type](const PMSPolygon& old_polygon) {
                PMSPolygon new_polygon = old_polygon;
                new_polygon.polygon_type = new_polygon_type;
                return new_polygon;
            },
            client_state,
            game_state_manager);
      });

    OnSelectNewTool(client_state.map_editor_state.selected_tool, client_state, game_state_manager);
}

void MapEditor::Lock()
{
    locked_ = true;
    tool_controller_->Deactivate();
    event_router_.Emit(EditorToolsDeactivatedEvent{});
}

void MapEditor::Unlock()
{
    locked_ = false;
    tool_controller_->Activate();
    event_router_.Emit(EditorToolsActivatedEvent{});
}

void MapEditor::OnSelectNewTool(ToolType tool_type,
                                ClientState& client_state,
                                const StateManager& game_state_manager)
{
    if (locked_ || !tool_controller_->IsActive()) {
        return;
    }

    tool_controller_->Select(tool_type, client_state, game_state_manager);
    event_router_.Emit(EditorToolSelectedEvent{ tool_type });
}

void MapEditor::OnSceneLeftMouseButtonClick(ClientState& client_state,
                                            const StateManager& game_state_manager)
{
    if (locked_) {
        return;
    }

    tool_controller_->ActiveTool().OnSceneLeftMouseButtonClick(client_state, game_state_manager);
}

void MapEditor::OnSceneLeftMouseButtonRelease(ClientState& client_state,
                                              const StateManager& game_state_manager)
{
    if (locked_) {
        return;
    }

    tool_controller_->ActiveTool().OnSceneLeftMouseButtonRelease(client_state, game_state_manager);
}

void MapEditor::OnSceneRightMouseButtonClick(ClientState& client_state)
{
    if (locked_) {
        return;
    }

    tool_controller_->ActiveTool().OnSceneRightMouseButtonClick(client_state);
}

void MapEditor::OnSceneRightMouseButtonRelease()
{
    if (locked_) {
        return;
    }

    tool_controller_->ActiveTool().OnSceneRightMouseButtonRelease();
}

void MapEditor::OnSceneMiddleMouseButtonClick(ClientState& client_state)
{
    if (locked_) {
        return;
    }

    viewport_controller_.BeginCameraDrag(client_state);
}

void MapEditor::OnSceneMiddleMouseButtonRelease()
{
    if (locked_) {
        return;
    }

    viewport_controller_.EndCameraDrag();
}

void MapEditor::OnMouseScrollUp(ClientState& client_state) const
{
    if (locked_) {
        return;
    }

    viewport_controller_.ZoomInAtMouse(client_state);
}

void MapEditor::OnMouseScrollDown(ClientState& client_state) const
{
    if (locked_) {
        return;
    }

    viewport_controller_.ZoomOutAtMouse(client_state);
}

void MapEditor::OnMouseScreenPositionChange(ClientState& client_state,
                                            glm::vec2 last_mouse_position,
                                            glm::vec2 new_mouse_position)
{
    if (locked_) {
        return;
    }

    viewport_controller_.OnMouseScreenPositionChange(client_state, new_mouse_position);

    tool_controller_->ActiveTool().OnMouseScreenPositionChange(
      client_state, last_mouse_position, new_mouse_position);
}

void MapEditor::OnMouseMapPositionChange(ClientState& client_state,
                                         glm::vec2 last_mouse_position,
                                         glm::vec2 new_mouse_position,
                                         const StateManager& game_state_manager)
{
    if (locked_) {
        return;
    }

    tool_controller_->ActiveTool().OnMouseMapPositionChange(
      client_state, last_mouse_position, new_mouse_position, game_state_manager);
}

void MapEditor::OnKeyPressed(int key,
                             int modifiers,
                             ClientState& client_state,
                             StateManager& game_state_manager)
{
    if (locked_) {
        return;
    }

    if (shortcut_controller_.IsSaveShortcut(key)) {
        document_.SaveCurrentMapOrOpenSaveAs();

        return;
    }

    if (shortcut_controller_.IsRedoShortcut(key)) {
        RedoUndoneAction(client_state, game_state_manager);

        return;
    }

    if (shortcut_controller_.IsUndoShortcut(key)) {
        UndoLastAction(client_state, game_state_manager);

        return;
    }

    if (shortcut_controller_.IsMapSettingsShortcut(key)) {
        document_.OpenMapSettings();

        return;
    }

    if (std::optional<ToolType> tool_type = shortcut_controller_.GetToolForKey(
          key, modifiers, client_state.map_editor_state.GetToolShortcuts())) {
        if (client_state.map_editor_state.selected_tool != *tool_type) {
            client_state.map_editor_state.event_selected_new_tool.Notify(*tool_type);
        }
        client_state.map_editor_state.selected_tool = *tool_type;
        return;
    }

    bool is_anything_selected = false;

    is_anything_selected |= !client_state.map_editor_state.selected_polygon_vertices.empty();
    is_anything_selected |= !client_state.map_editor_state.selected_scenery_ids.empty();
    is_anything_selected |= !client_state.map_editor_state.selected_spawn_point_ids.empty();

    if (shortcut_controller_.IsCopyShortcut(key) && is_anything_selected) {
        CopySelection(client_state, game_state_manager);
        return;
    }

    if (shortcut_controller_.IsPasteShortcut(key)) {
        PasteSelection(client_state, game_state_manager);
        return;
    }

    if (key == GLFW_KEY_LEFT_SHIFT) {
        tool_controller_->ActiveTool().OnModifierKey1Pressed(client_state);
    }
    if (key == GLFW_KEY_LEFT_CONTROL) {
        tool_controller_->ActiveTool().OnModifierKey2Pressed(client_state);
    }
    if (key == GLFW_KEY_LEFT_ALT) {
        tool_controller_->ActiveTool().OnModifierKey3Pressed(client_state);
    }

    if (key == GLFW_KEY_DELETE) {
        RemoveCurrentSelection(client_state, game_state_manager);
    }
}

void MapEditor::CopySelection(ClientState& client_state, const StateManager& game_state_manager)
{
    // TODO: make use of system's clipboard
    copied_polygons_.clear();
    copied_sceneries_.clear();
    copied_spawn_points_.clear();

    for (const auto& [selected_polygon_id, selected_vertices] :
         client_state.map_editor_state.selected_polygon_vertices) {
        if (!selected_vertices.all()) {
            continue;
        }

        copied_polygons_.push_back(
          game_state_manager.GetConstMap().GetPolygons().at(selected_polygon_id));
    }

    for (const auto& selected_scenery_id : client_state.map_editor_state.selected_scenery_ids) {
        const auto& scenery =
          game_state_manager.GetConstMap().GetSceneryInstances().at(selected_scenery_id);
        copied_sceneries_.push_back(
          { selected_scenery_id,
            { scenery,
              game_state_manager.GetConstMap().GetSceneryTypes().at(scenery.style - 1).name } });
    }

    for (const auto& selected_spawn_point_id :
         client_state.map_editor_state.selected_spawn_point_ids) {
        copied_spawn_points_.emplace_back(
          selected_spawn_point_id,
          game_state_manager.GetConstMap().GetSpawnPoints().at(selected_spawn_point_id));
    }
}

void MapEditor::PasteSelection(ClientState& client_state, StateManager& game_state_manager)
{
    if (copied_polygons_.empty() && copied_sceneries_.empty() && copied_spawn_points_.empty()) {
        return;
    }

    std::unique_ptr<AddObjectsMapEditorAction> add_copied_objects_action =
      std::make_unique<AddObjectsMapEditorAction>(
        copied_polygons_, copied_sceneries_, copied_spawn_points_);
    ExecuteNewAction(client_state, game_state_manager, std::move(add_copied_objects_action));
}

void MapEditor::OnKeyReleased(int key, ClientState& client_state)
{
    shortcut_controller_.OnKeyReleased(key);

    if (key == GLFW_KEY_LEFT_SHIFT) {
        tool_controller_->ActiveTool().OnModifierKey1Released(client_state);
    }
    if (key == GLFW_KEY_LEFT_CONTROL) {
        tool_controller_->ActiveTool().OnModifierKey2Released(client_state);
    }
    if (key == GLFW_KEY_LEFT_ALT) {
        tool_controller_->ActiveTool().OnModifierKey3Released(client_state);
    }
}

void MapEditor::ExecuteNewAction(ClientState& client_state,
                                 StateManager& game_state_manager,
                                 std::unique_ptr<MapEditorAction> new_action)
{
    if (locked_) {
        return;
    }

    if (command_history_.Execute(client_state, game_state_manager, std::move(new_action))) {
        event_router_.Emit(EditorCommandExecutedEvent{});
    }
}

void MapEditor::UndoLastAction(ClientState& client_state, StateManager& game_state_manager)
{
    if (locked_) {
        return;
    }

    command_history_.Undo(client_state, game_state_manager);
    event_router_.Emit(EditorCommandUndoneEvent{});
}

void MapEditor::RedoUndoneAction(ClientState& client_state, StateManager& game_state_manager)
{
    if (locked_) {
        return;
    }

    command_history_.Redo(client_state, game_state_manager);
    event_router_.Emit(EditorCommandRedoneEvent{});
}

void MapEditor::RemoveCurrentSelection(ClientState& client_state, StateManager& game_state_manager)
{
    std::unique_ptr<RemoveSelectionMapEditorAction> remove_selection_action =
      std::make_unique<RemoveSelectionMapEditorAction>(client_state, game_state_manager);
    ExecuteNewAction(client_state, game_state_manager, std::move(remove_selection_action));
}

void MapEditor::OnChangeSelectedSpawnPointsTypes(PMSSpawnPointType new_spawn_point_type,
                                                 ClientState& client_state,
                                                 StateManager& game_state_manager)
{
    std::vector<std::pair<unsigned int, PMSSpawnPoint>> selected_spawn_points;
    selected_spawn_points.reserve(client_state.map_editor_state.selected_spawn_point_ids.size());
    for (unsigned int selected_spawn_point_id :
         client_state.map_editor_state.selected_spawn_point_ids) {
        selected_spawn_points.emplace_back(
          selected_spawn_point_id,
          game_state_manager.GetConstMap().GetSpawnPoints().at(selected_spawn_point_id));
    }
    std::unique_ptr<TransformSpawnPointsMapEditorAction> transform_spawn_points_action =
      std::make_unique<TransformSpawnPointsMapEditorAction>(
        selected_spawn_points, [new_spawn_point_type](const PMSSpawnPoint& old_spawn_point) {
            PMSSpawnPoint new_spawn_point = old_spawn_point;
            new_spawn_point.type = new_spawn_point_type;
            return new_spawn_point;
        });
    ExecuteNewAction(client_state, game_state_manager, std::move(transform_spawn_points_action));
}

void MapEditor::OnChangeSelectedSceneriesLevel(int new_level,
                                               ClientState& client_state,
                                               StateManager& game_state_manager)
{
    std::vector<std::pair<unsigned int, PMSScenery>> selected_sceneries;
    selected_sceneries.reserve(client_state.map_editor_state.selected_scenery_ids.size());
    for (unsigned int selected_scenery_id : client_state.map_editor_state.selected_scenery_ids) {
        selected_sceneries.emplace_back(
          selected_scenery_id,
          game_state_manager.GetConstMap().GetSceneryInstances().at(selected_scenery_id));
    }
    std::unique_ptr<TransformSceneriesMapEditorAction> transform_sceneries_action =
      std::make_unique<TransformSceneriesMapEditorAction>(
        selected_sceneries, [new_level](const PMSScenery& old_scenery) {
            PMSScenery new_scenery = old_scenery;
            new_scenery.level = new_level;
            return new_scenery;
        });
    ExecuteNewAction(client_state, game_state_manager, std::move(transform_sceneries_action));
}

void MapEditor::OnTransformSelectedPolygons(
  const std::function<PMSPolygon(const PMSPolygon&)>& transform_function,
  ClientState& client_state,
  StateManager& game_state_manager)
{
    std::vector<std::pair<unsigned int, PMSPolygon>> selected_polygons;
    selected_polygons.reserve(client_state.map_editor_state.selected_polygon_vertices.size());
    for (const auto& selected_polygon_vertices :
         client_state.map_editor_state.selected_polygon_vertices) {
        selected_polygons.emplace_back(
          selected_polygon_vertices.first,
          game_state_manager.GetConstMap().GetPolygons().at(selected_polygon_vertices.first));
    }
    std::unique_ptr<TransformPolygonsMapEditorAction> transform_polygons_action =
      std::make_unique<TransformPolygonsMapEditorAction>(selected_polygons, transform_function);
    ExecuteNewAction(client_state, game_state_manager, std::move(transform_polygons_action));
}
} // namespace Soldank
