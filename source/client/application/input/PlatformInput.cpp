module;

#include "core/math/Glm.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <vector>

export module Application.Input.PlatformInput;

import Application.Input.InputSnapshot;

export namespace Soldank
{
class PlatformInput
{
public:
    void OnKey(int key, int action)
    {
        if (!IsValidKey(key)) {
            return;
        }

        key_changes_.push_back({ .key = key, .action = action });
        if (action == GLFW_RELEASE) {
            keys_.at(key) = false;
            keys_released_.at(key) = true;
        } else if (action != GLFW_REPEAT) {
            keys_.at(key) = true;
            keys_pressed_.at(key) = true;
        }
    }

    void OnMouseButton(int button, int action)
    {
        if (!IsValidMouseButton(button)) {
            return;
        }

        mouse_button_changes_.push_back({ .button = button, .action = action });
        if (action == GLFW_RELEASE) {
            mouse_buttons_.at(button) = false;
        } else if (action != GLFW_REPEAT) {
            mouse_buttons_.at(button) = true;
        }
    }

    void OnCursorPos(GLFWwindow* window, double x, double y)
    {
        int window_width = 0;
        int window_height = 0;
        glfwGetWindowSize(window, &window_width, &window_height);

        if (first_mouse_) {
            last_x_ = x;
            last_y_ = y;
            first_mouse_ = false;
        }

        dx_ = x - last_x_;
        dy_ = last_y_ - y;
        last_x_ = x;
        last_y_ = y;

        if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
            x_ += dx_;
            y_ += dy_;
        } else {
            x_ = x;
            y_ = static_cast<double>(window_height) - y;
        }

        x_ = std::clamp(x_, 0.0, static_cast<double>(window_width));
        y_ = std::clamp(y_, 0.0, static_cast<double>(window_height));
    }

    void OnScroll(double /*dx*/, double dy) { mouse_wheel_delta_ += static_cast<float>(dy); }

    bool Key(int key) const
    {
        if (!IsValidKey(key)) {
            return false;
        }
        return keys_.at(key);
    }

    bool KeyWentDown(int key) const
    {
        if (!IsValidKey(key)) {
            return false;
        }
        return keys_pressed_.at(key);
    }

    bool KeyWentUp(int key) const
    {
        if (!IsValidKey(key)) {
            return false;
        }
        return keys_released_.at(key);
    }

    bool Button(int button) const
    {
        if (!IsValidMouseButton(button)) {
            return false;
        }
        return mouse_buttons_.at(button);
    }

    double GetX() const { return x_; }
    double GetY() const { return y_; }
    double GetDx() const { return dx_; }
    double GetDy() const { return dy_; }

    InputSnapshot CreateSnapshot(glm::vec2 mouse_screen_position,
                                 glm::vec2 mouse_map_position) const
    {
        return InputSnapshot{
            .mouse_screen_position = mouse_screen_position,
            .mouse_map_position = mouse_map_position,
            .left_mouse_down = Button(GLFW_MOUSE_BUTTON_LEFT),
            .right_mouse_down = Button(GLFW_MOUSE_BUTTON_RIGHT),
            .middle_mouse_down = Button(GLFW_MOUSE_BUTTON_MIDDLE),
            .key_changes = key_changes_,
            .mouse_button_changes = mouse_button_changes_,
            .mouse_wheel_delta = mouse_wheel_delta_,
        };
    }

    void ResetFrame()
    {
        key_changes_.clear();
        mouse_button_changes_.clear();
        mouse_wheel_delta_ = 0.0F;
        dx_ = 0.0;
        dy_ = 0.0;
        keys_pressed_.fill(false);
        keys_released_.fill(false);
    }

private:
    static bool IsValidKey(int key) { return key >= 0 && key <= GLFW_KEY_LAST; }
    static bool IsValidMouseButton(int button)
    {
        return button >= 0 && button <= GLFW_MOUSE_BUTTON_LAST;
    }

    std::array<bool, GLFW_KEY_LAST + 1> keys_{};
    std::array<bool, GLFW_KEY_LAST + 1> keys_pressed_{};
    std::array<bool, GLFW_KEY_LAST + 1> keys_released_{};
    std::array<bool, GLFW_MOUSE_BUTTON_LAST + 1> mouse_buttons_{};
    std::vector<KeyStateChange> key_changes_;
    std::vector<MouseButtonStateChange> mouse_button_changes_;

    double x_ = 320.0;
    double y_ = 240.0;
    double dx_ = 0.0;
    double dy_ = 0.0;
    double last_x_ = 0.0;
    double last_y_ = 0.0;
    float mouse_wheel_delta_ = 0.0F;
    bool first_mouse_ = true;
};
} // namespace Soldank
