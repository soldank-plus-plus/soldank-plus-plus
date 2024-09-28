#include "map_editor/MapEditor.hpp"

#include "map_editor/tools/PolygonTool.hpp"
#include "map_editor/tools/DummyTool.hpp"

#include "spdlog/spdlog.h"

#include <utility>

namespace Soldank
{
MapEditor::MapEditor(ClientState& client_state)
    : selected_tool_(client_state.map_editor_state.selected_tool)
    , is_dragging_camera_(false)
{
    client_state.event_left_mouse_button_clicked.AddObserver([this, &client_state]() {
        if (!client_state.map_editor_state.is_mouse_hovering_over_ui) {
            OnSceneLeftMouseButtonClick();
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

    tools_.emplace_back(std::make_unique<DummyTool>());
    tools_.emplace_back(std::make_unique<PolygonTool>());
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

void MapEditor::OnSelectNewTool(ToolType tool_type)
{
    tools_.at(std::to_underlying(selected_tool_))->OnUnselect();
    selected_tool_ = tool_type;
    tools_.at(std::to_underlying(selected_tool_))->OnSelect();
}

void MapEditor::OnSceneLeftMouseButtonClick()
{
    tools_.at(std::to_underlying(selected_tool_))->OnSceneLeftMouseButtonClick();
}

void MapEditor::OnSceneLeftMouseButtonRelease() {}

void MapEditor::OnSceneRightMouseButtonClick() {}

void MapEditor::OnSceneRightMouseButtonRelease() {}

void MapEditor::OnSceneMiddleMouseButtonClick(ClientState& client_state)
{
    is_dragging_camera_ = true;
    mouse_screen_position_on_start_dragging_ = current_mouse_screen_position_;
    camera_position_on_start_dragging_ = client_state.camera;
}

void MapEditor::OnSceneMiddleMouseButtonRelease()
{
    is_dragging_camera_ = false;
}

void MapEditor::OnMouseScrollUp(ClientState& client_state)
{
    client_state.camera_component.ZoomIn();
}

void MapEditor::OnMouseScrollDown(ClientState& client_state)
{
    client_state.camera_component.ZoomOut();
}

void MapEditor::OnMouseScreenPositionChange(ClientState& client_state,
                                            glm::vec2 last_mouse_position,
                                            glm::vec2 new_mouse_position)
{
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

    tools_.at(std::to_underlying(selected_tool_))->OnMouseMove(new_mouse_position);
}

void MapEditor::OnMouseMapPositionChange(ClientState& client_state,
                                         glm::vec2 last_mouse_position,
                                         glm::vec2 new_mouse_position)
{
}
} // namespace Soldank
