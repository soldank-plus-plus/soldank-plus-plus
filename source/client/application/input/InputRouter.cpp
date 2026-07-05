module;

#include "core/math/Glm.hpp"

#include <GLFW/glfw3.h>

#include <functional>

export module Application.Input.InputRouter;

import Application.Input.InputSnapshot;

export namespace Soldank
{
class InputRouter
{
public:
    using KeyHandler = std::function<void(int, int)>;
    using MouseButtonHandler = std::function<void(int, int)>;
    using MouseMoveHandler = std::function<void(glm::vec2, glm::vec2)>;
    using MouseScrollHandler = std::function<void(float)>;

    void SetActiveContext(InputContext context) { active_context_ = context; }

    InputContext GetActiveContext() const { return active_context_; }

    void SetKeyHandler(KeyHandler handler) { key_handler_ = std::move(handler); }
    void SetMouseButtonHandler(MouseButtonHandler handler)
    {
        mouse_button_handler_ = std::move(handler);
    }
    void SetMouseScreenMoveHandler(MouseMoveHandler handler)
    {
        mouse_screen_move_handler_ = std::move(handler);
    }
    void SetMouseMapMoveHandler(MouseMoveHandler handler)
    {
        mouse_map_move_handler_ = std::move(handler);
    }
    void SetMouseScrollHandler(MouseScrollHandler handler)
    {
        mouse_scroll_handler_ = std::move(handler);
    }

    void Route(const InputSnapshot& input)
    {
        const bool ui_captured = active_context_ == InputContext::UiCaptured;

        if (!ui_captured) {
            if (mouse_screen_move_handler_) {
                mouse_screen_move_handler_(last_mouse_screen_position_,
                                           input.mouse_screen_position);
            }
            if (mouse_map_move_handler_) {
                mouse_map_move_handler_(last_mouse_map_position_, input.mouse_map_position);
            }
            if (mouse_scroll_handler_ && input.mouse_wheel_delta != 0.0F) {
                mouse_scroll_handler_(input.mouse_wheel_delta);
            }
            if (mouse_button_handler_) {
                for (const auto& mouse_button_change : input.mouse_button_changes) {
                    mouse_button_handler_(mouse_button_change.button, mouse_button_change.action);
                }
            }
        }

        if (key_handler_) {
            for (const auto& key_change : input.key_changes) {
                if (!ui_captured || IsGlobalKey(key_change.key)) {
                    key_handler_(key_change.key, key_change.action);
                }
            }
        }

        last_mouse_screen_position_ = input.mouse_screen_position;
        last_mouse_map_position_ = input.mouse_map_position;
    }

    void ResetMousePositions(glm::vec2 mouse_screen_position, glm::vec2 mouse_map_position)
    {
        last_mouse_screen_position_ = mouse_screen_position;
        last_mouse_map_position_ = mouse_map_position;
    }

private:
    static bool IsGlobalKey(int key) { return key == GLFW_KEY_F5; }

    InputContext active_context_ = InputContext::Gameplay;
    KeyHandler key_handler_;
    MouseButtonHandler mouse_button_handler_;
    MouseMoveHandler mouse_screen_move_handler_;
    MouseMoveHandler mouse_map_move_handler_;
    MouseScrollHandler mouse_scroll_handler_;
    glm::vec2 last_mouse_screen_position_{};
    glm::vec2 last_mouse_map_position_{};
};
} // namespace Soldank
