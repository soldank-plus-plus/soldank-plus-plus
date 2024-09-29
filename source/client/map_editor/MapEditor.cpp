#include "map_editor/MapEditor.hpp"

#include "map_editor/tools/PolygonTool.hpp"
#include "map_editor/tools/DummyTool.hpp"

#include "spdlog/spdlog.h"

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
{
    add_new_map_editor_action_ = [this, &game_state](std::unique_ptr<MapEditorAction> new_action) {
        ExecuteNewAction(game_state.map, std::move(new_action));
    };

    client_state.event_left_mouse_button_clicked.AddObserver([this, &client_state]() {
        if (!client_state.map_editor_state.is_mouse_hovering_over_ui) {
            OnSceneLeftMouseButtonClick(client_state);
        }
    });

    client_state.event_left_mouse_button_released.AddObserver([this, &client_state]() {
        if (!client_state.map_editor_state.is_mouse_hovering_over_ui) {
            OnSceneLeftMouseButtonRelease();
        }
    });

    client_state.event_right_mouse_button_clicked.AddObserver([this, &client_state]() {
        if (!client_state.map_editor_state.is_mouse_hovering_over_ui) {
            OnSceneRightMouseButtonClick();
        }
    });

    client_state.event_right_mouse_button_released.AddObserver([this, &client_state]() {
        if (!client_state.map_editor_state.is_mouse_hovering_over_ui) {
            OnSceneRightMouseButtonRelease();
        }
    });

    client_state.event_middle_mouse_button_clicked.AddObserver([this, &client_state]() {
        if (!client_state.map_editor_state.is_mouse_hovering_over_ui) {
            OnSceneMiddleMouseButtonClick(client_state);
        }
    });

    client_state.event_middle_mouse_button_released.AddObserver([this, &client_state]() {
        if (!client_state.map_editor_state.is_mouse_hovering_over_ui) {
            OnSceneMiddleMouseButtonRelease();
        }
    });

    client_state.event_mouse_screen_position_changed.AddObserver(
      [this, &client_state](glm::vec2 last_mouse_position, glm::vec2 new_mouse_position) {
          OnMouseScreenPositionChange(client_state, last_mouse_position, new_mouse_position);
      });

    client_state.event_mouse_map_position_changed.AddObserver(
      [this, &client_state](glm::vec2 last_mouse_position, glm::vec2 new_mouse_position) {
          OnMouseMapPositionChange(client_state, last_mouse_position, new_mouse_position);
      });

    client_state.event_mouse_wheel_scrolled_up.AddObserver(
      [this, &client_state]() { OnMouseScrollUp(client_state); });
    client_state.event_mouse_wheel_scrolled_down.AddObserver(
      [this, &client_state]() { OnMouseScrollDown(client_state); });

    client_state.map_editor_state.event_selected_new_tool.AddObserver(
      [this](ToolType tool_type) { OnSelectNewTool(tool_type); });

    client_state.map_editor_state.event_pressed_undo.AddObserver(
      [this, &game_state]() { UndoLastAction(game_state.map); });

    client_state.map_editor_state.event_pressed_redo.AddObserver(
      [this, &game_state]() { RedoUndoneAction(game_state.map); });

    tools_.emplace_back(std::make_unique<DummyTool>());
    tools_.emplace_back(std::make_unique<PolygonTool>(add_new_map_editor_action_));
    tools_.emplace_back(std::make_unique<DummyTool>());
    tools_.emplace_back(std::make_unique<DummyTool>());
    tools_.emplace_back(std::make_unique<DummyTool>());
    tools_.emplace_back(std::make_unique<DummyTool>());
    tools_.emplace_back(std::make_unique<DummyTool>());
    tools_.emplace_back(std::make_unique<DummyTool>());
    tools_.emplace_back(std::make_unique<DummyTool>());
    tools_.emplace_back(std::make_unique<DummyTool>());
    tools_.emplace_back(std::make_unique<DummyTool>());
}

void MapEditor::Lock()
{
    locked_ = true;
}

void MapEditor::Unlock()
{
    locked_ = false;
}

void MapEditor::OnSelectNewTool(ToolType tool_type)
{
    if (locked_) {
        return;
    }

    tools_.at(std::to_underlying(selected_tool_))->OnUnselect();
    selected_tool_ = tool_type;
    tools_.at(std::to_underlying(selected_tool_))->OnSelect();
}

void MapEditor::OnSceneLeftMouseButtonClick(ClientState& client_state)
{
    if (locked_) {
        return;
    }

    tools_.at(std::to_underlying(selected_tool_))->OnSceneLeftMouseButtonClick(client_state);
}

void MapEditor::OnSceneLeftMouseButtonRelease()
{
    if (locked_) {
        return;
    }

    tools_.at(std::to_underlying(selected_tool_))->OnSceneLeftMouseButtonRelease();
}

void MapEditor::OnSceneRightMouseButtonClick()
{
    if (locked_) {
        return;
    }

    tools_.at(std::to_underlying(selected_tool_))->OnSceneRightMouseButtonClick();
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
                                         glm::vec2 new_mouse_position)
{
    if (locked_) {
        return;
    }

    tools_.at(std::to_underlying(selected_tool_))
      ->OnMouseMapPositionChange(client_state, last_mouse_position, new_mouse_position);
}

void MapEditor::ExecuteNewAction(Map& map, std::unique_ptr<MapEditorAction> new_action)
{
    if (locked_) {
        return;
    }

    map_editor_undone_actions_.clear();
    new_action->Execute(map);
    map_editor_executed_actions_.push_back(std::move(new_action));
}

void MapEditor::UndoLastAction(Map& map)
{
    if (locked_) {
        return;
    }

    if (!map_editor_executed_actions_.empty()) {
        map_editor_executed_actions_.back()->Undo(map);
        map_editor_undone_actions_.push_back(std::move(map_editor_executed_actions_.back()));
        map_editor_executed_actions_.pop_back();
    }
}

void MapEditor::RedoUndoneAction(Map& map)
{
    if (locked_) {
        return;
    }

    if (!map_editor_undone_actions_.empty()) {
        map_editor_executed_actions_.push_back(std::move(map_editor_undone_actions_.back()));
        map_editor_undone_actions_.pop_back();
        map_editor_executed_actions_.back()->Execute(map);
    }
}
} // namespace Soldank
