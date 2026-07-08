module;

export module MapEditor.EditorViewportController;

import Extern.Glm;

import ClientState;

export namespace Soldank
{
class EditorViewportController
{
public:
    void BeginCameraDrag(const ClientState& client_state)
    {
        is_dragging_camera_ = true;
        mouse_screen_position_on_start_dragging_ = current_mouse_screen_position_;
        camera_position_on_start_dragging_ = client_state.camera.position;
    }

    void EndCameraDrag() { is_dragging_camera_ = false; }

    void ZoomInAtMouse(ClientState& client_state) const
    {
        ZoomAtMouse(client_state, ZoomDirection::In);
    }

    void ZoomOutAtMouse(ClientState& client_state) const
    {
        ZoomAtMouse(client_state, ZoomDirection::Out);
    }

    void OnMouseScreenPositionChange(ClientState& client_state, glm::vec2 new_mouse_position)
    {
        new_mouse_position.y = -new_mouse_position.y;
        current_mouse_screen_position_ = new_mouse_position;

        if (!is_dragging_camera_) {
            return;
        }

        glm::vec2 mouse_position_difference =
          mouse_screen_position_on_start_dragging_ - new_mouse_position;

        mouse_position_difference.x /=
          client_state.input.window_width / client_state.camera.view.GetWidth();
        mouse_position_difference.y /=
          client_state.input.window_height / client_state.camera.view.GetHeight();

        client_state.camera.position =
          camera_position_on_start_dragging_ + mouse_position_difference;
    }

private:
    enum class ZoomDirection
    {
        In,
        Out
    };

    static void ZoomAtMouse(ClientState& client_state, ZoomDirection zoom_direction)
    {
        glm::vec2 old_camera_dimensions = { client_state.camera.view.GetWidth(),
                                            client_state.camera.view.GetHeight() };
        glm::vec2 camera_position = client_state.camera.position;
        camera_position.y = -camera_position.y;
        glm::vec2 mouse_camera_diff = client_state.input.mouse_map_position - camera_position;
        glm::vec2 mouse_on_view_position = mouse_camera_diff + (old_camera_dimensions / 2.0F);
        glm::vec2 side_ratio =
          (mouse_on_view_position - (old_camera_dimensions / 2.0F)) / old_camera_dimensions;

        if (zoom_direction == ZoomDirection::In) {
            client_state.camera.view.ZoomIn();
        } else {
            client_state.camera.view.ZoomOut();
        }

        glm::vec2 new_camera_dimensions = { client_state.camera.view.GetWidth(),
                                            client_state.camera.view.GetHeight() };
        glm::vec2 pixels_difference = old_camera_dimensions - new_camera_dimensions;
        side_ratio.y = -side_ratio.y;

        client_state.camera.position += pixels_difference * side_ratio;
    }

    glm::vec2 current_mouse_screen_position_;
    glm::vec2 camera_position_on_start_dragging_;
    glm::vec2 mouse_screen_position_on_start_dragging_;
    bool is_dragging_camera_ = false;
};
} // namespace Soldank
