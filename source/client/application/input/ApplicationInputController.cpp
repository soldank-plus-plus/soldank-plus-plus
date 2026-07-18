module;

#include <GLFW/glfw3.h>

#include <cmath>
#include <cstdint>

export module Application.Input.ApplicationInputController;

import Extern.Glm;

import Application.ClientModes;
import Application.Input.InputRouter;
import Application.Input.InputSnapshot;
import Application.Input.PlatformInput;
import Application.Window;
import ClientState;
import DebugUI;
import Gameplay.GameSession;
import Runtime.ClientRuntime;

import Shared.Core.IWorld;

export namespace Soldank
{
class ApplicationInputController
{
public:
    ApplicationInputController(Window& window,
                               IWorld& world,
                               ClientState& client_state,
                               ClientRuntime& client_runtime,
                               const GameSession& game_session)
        : window_(window)
        , world_(world)
        , client_state_(client_state)
        , client_runtime_(client_runtime)
        , game_session_(game_session)
    {
    }

    void ConfigureHandlers()
    {
        input_router_.SetMouseButtonHandler([this](int button, int action) {
            if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
                client_state_.event_left_mouse_button_clicked.Notify();
            }
            if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
                client_state_.event_right_mouse_button_clicked.Notify();
            }
            if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS) {
                client_state_.event_middle_mouse_button_clicked.Notify();
            }
            if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
                client_state_.event_left_mouse_button_released.Notify();
            }
            if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
                client_state_.event_right_mouse_button_released.Notify();
            }
            if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE) {
                client_state_.event_middle_mouse_button_released.Notify();
            }
        });

        input_router_.SetKeyHandler([this](int key, int action, int modifiers) {
            if (action == GLFW_PRESS) {
                client_state_.event_key_pressed.Notify(key, modifiers);
                if (IsGameplayActive() && client_state_.client_soldier_id.has_value()) {
                    if (key == GLFW_KEY_J) {
                        MoveKeyboardAimCursor(*client_state_.client_soldier_id, -1.0F);
                    } else if (key == GLFW_KEY_L) {
                        MoveKeyboardAimCursor(*client_state_.client_soldier_id, 1.0F);
                    }
                }
            }
            if (action == GLFW_RELEASE) {
                client_state_.event_key_released.Notify(key, modifiers);
            }
        });
        input_router_.SetGlobalKeyPredicate([this](int key) {
            return client_state_.map_editor_state.is_play_mode_shortcut_capture_active ||
                   client_state_.map_editor_state.tool_shortcut_capture_index >= 0 ||
                   client_state_.map_editor_state.shortcut_capture_binding_index >= 0 ||
                   (key == GLFW_KEY_ESCAPE &&
                    client_state_.map_editor_state.is_play_test_escape_menu_open);
        });

        input_router_.SetMouseScreenMoveHandler(
          [this](glm::vec2 last_mouse_position, glm::vec2 new_mouse_position) {
              if (std::abs(last_mouse_position.x - new_mouse_position.x) > 0.001F ||
                  std::abs(last_mouse_position.y - new_mouse_position.y) > 0.001F) {
                  client_state_.event_mouse_screen_position_changed.Notify(last_mouse_position,
                                                                           new_mouse_position);
              }
          });
        input_router_.SetMouseMapMoveHandler(
          [this](glm::vec2 last_mouse_position, glm::vec2 new_mouse_position) {
              if (std::abs(last_mouse_position.x - new_mouse_position.x) > 0.0000001F ||
                  std::abs(last_mouse_position.y - new_mouse_position.y) > 0.0000001F) {
                  client_state_.event_mouse_map_position_changed.Notify(last_mouse_position,
                                                                        new_mouse_position);
                  client_state_.input.mouse_map_position = GetMouseMapPosition();
              }
          });
        input_router_.SetMouseScrollHandler([this](float wheel_delta) {
            if (wheel_delta < 0.0F) {
                client_state_.event_mouse_wheel_scrolled_down.Notify();
            } else if (wheel_delta > 0.0F) {
                client_state_.event_mouse_wheel_scrolled_up.Notify();
            }

            glm::vec2 mouse_map_position = GetMouseMapPosition();
            client_state_.event_mouse_map_position_changed.Notify(
              client_state_.input.mouse_map_position, mouse_map_position);
            client_state_.input.mouse_map_position = mouse_map_position;
        });
    }

    void ResetMousePositions()
    {
        input_router_.ResetMousePositions(GetMouseScreenPosition(), GetMouseMapPosition());
    }

    void Route()
    {
        const InputSnapshot input_snapshot = window_.GetPlatformInput().CreateSnapshot(
          GetMouseScreenPosition(), GetMouseMapPosition());
        input_router_.Route(input_snapshot);
    }

    void UpdateContext()
    {
        bool is_ui_captured = DebugUI::GetWantCaptureMouse() ||
                              client_state_.map_editor_state.is_mouse_hovering_over_ui ||
                              client_state_.map_editor_state.is_modal_or_popup_open;
        input_router_.SetActiveContext(client_runtime_.GetInputContext(is_ui_captured));
        input_router_.SetKeyboardCaptured(DebugUI::GetWantTextInput() ||
                                          client_state_.map_editor_state.is_modal_or_popup_open);
    }

    void UpdateWindowSize()
    {
        glm::ivec2 window_size = window_.GetWindowSize();
        client_state_.input.window_width = static_cast<float>(window_size.x);
        client_state_.input.window_height = static_cast<float>(window_size.y);
    }

    bool IsUiCaptured() const
    {
        return input_router_.GetActiveContext() == InputContext::UiCaptured;
    }

    glm::vec2 GetMouseScreenPosition() const { return window_.GetCursorScreenPosition(); }

    glm::vec2 GetMouseMapPosition() const
    {
        glm::vec2 window_size = window_.GetWindowSize();
        glm::vec2 mouse_map_position;
        if (window_.GetCursorMode() == CursorMode::Locked) {
            mouse_map_position = { window_.GetPlatformInput().GetX(),
                                   window_size.y - window_.GetPlatformInput().GetY() };
        } else {
            mouse_map_position = window_.GetCursorScreenPosition();
        }

        float ratio_x = window_size.x / client_state_.camera.view.GetWidth();
        float ratio_y = window_size.y / client_state_.camera.view.GetHeight();
        mouse_map_position.x /= ratio_x;
        mouse_map_position.y /= ratio_y;
        window_size.x /= ratio_x;
        window_size.y /= ratio_y;
        mouse_map_position.x =
          mouse_map_position.x - window_size.x / 2.0F + client_state_.camera.position.x;
        mouse_map_position.y =
          mouse_map_position.y - window_size.y / 2.0F - client_state_.camera.position.y;
        return mouse_map_position;
    }

private:
    bool IsGameplayActive() const
    {
        return game_session_.IsGameplayActive(client_runtime_.GetClientMode(),
                                              client_runtime_.GetEditorMode());
    }

    void MoveKeyboardAimCursor(std::uint8_t soldier_id, float direction)
    {
        constexpr float KEYBOARD_CURSOR_DISTANCE = 150.0F;
        const glm::vec2& soldier_position = world_.GetSoldier(soldier_id).particle.position;
        glm::vec2 window_size = window_.GetWindowSize();
        float ratio_x = window_size.x / client_state_.camera.view.GetWidth();
        float ratio_y = window_size.y / client_state_.camera.view.GetHeight();
        glm::vec2 soldier_screen_position = {
            (soldier_position.x - client_state_.camera.position.x +
             client_state_.camera.view.GetWidth() / 2.0F) *
              ratio_x,
            (soldier_position.y + client_state_.camera.position.y +
             client_state_.camera.view.GetHeight() / 2.0F) *
              ratio_y
        };
        soldier_screen_position.y = window_size.y - soldier_screen_position.y;
        soldier_screen_position.x += direction * KEYBOARD_CURSOR_DISTANCE;
        window_.SetCursorScreenPosition(soldier_screen_position);
    }

    Window& window_;
    IWorld& world_;
    ClientState& client_state_;
    ClientRuntime& client_runtime_;
    const GameSession& game_session_;
    InputRouter input_router_;
};
} // namespace Soldank
