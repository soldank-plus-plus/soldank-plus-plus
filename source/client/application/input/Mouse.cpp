#include "Mouse.hpp"

#include <GLFW/glfw3.h>

namespace Soldank
{
bool Mouse::first_mouse_ = true;

// initialized in the middle of the screen
double Mouse::x_ = 320.0f;
double Mouse::y_ = 240.0f;

double Mouse::dx_ = 0;
double Mouse::dy_ = 0;

double Mouse::lastx_ = 0;
double Mouse::lasty_ = 0;

double Mouse::scroll_dx_ = 0;
double Mouse::scroll_dy_ = 0;

bool Mouse::buttons_[GLFW_MOUSE_BUTTON_LAST] = { 0 };
bool Mouse::buttons_changed_[GLFW_MOUSE_BUTTON_LAST] = { 0 };
std::vector<std::function<void(int, int)>> Mouse::button_observers_{};
std::vector<std::function<void(double, double)>> Mouse::mouse_scroll_observers_{};
std::vector<std::function<void(double, double)>> Mouse::mouse_movement_observers_{};

void Mouse::CursorPosCallback(GLFWwindow* window, const double x, const double y)
{
    int window_width = 0;
    int window_height = 0;
    glfwGetWindowSize(window, &window_width, &window_height);

    for (const auto& mouse_movement_observer : mouse_movement_observers_) {
        mouse_movement_observer(x, y);
    }

    if (first_mouse_) {
        lastx_ = x;
        lasty_ = y;
        first_mouse_ = false;
    }

    dx_ = x - lastx_;
    dy_ = lasty_ - y; // y coord are inverted
    lastx_ = x;
    lasty_ = y;

    x_ += dx_;
    if (x_ > window_width) {
        x_ = window_width;
    }
    if (x_ < 0.0) {
        x_ = 0.0;
    }
    y_ += dy_;
    if (y_ > window_height) {
        y_ = window_height;
    }
    if (y_ < 0.0) {
        y_ = 0.0;
    }
}

void Mouse::MouseButtonCallback(GLFWwindow* window, const int button, const int action, int mods)
{
    for (const auto& button_observer : button_observers_) {
        button_observer(button, action);
    }

    if (action == GLFW_PRESS) {
        if (!buttons_[button]) {
            buttons_[button] = true;
        }
    } else {
        buttons_[button] = false;
    }
    buttons_changed_[button] = action != GLFW_REPEAT;
}

void Mouse::MouseWheelCallback(GLFWwindow* window, const double dx, const double dy)
{
    for (const auto& mouse_scroll_observer : mouse_scroll_observers_) {
        mouse_scroll_observer(dx, dy);
    }

    Mouse::scroll_dx_ = 0;
    Mouse::scroll_dy_ = 0;
}

double Mouse::GetX()
{
    return x_;
}

double Mouse::GetY()
{
    return y_;
}

double Mouse::GetDx()
{
    const auto temp = dx_;
    dx_ = 0;
    return temp;
}

double Mouse::GetDy()
{
    const auto temp = dy_;
    dy_ = 0;
    return temp;
}

bool Mouse::Button(int button)
{
    return buttons_[button];
}

void Mouse::SubscribeButtonObserver(const std::function<void(int, int)>& observer)
{
    button_observers_.push_back(observer);
}

void Mouse::SubscribeMouseScrollObserver(const std::function<void(double, double)>& observer)
{
    mouse_scroll_observers_.push_back(observer);
}

void Mouse::SubscribeMouseMovementObserver(const std::function<void(double, double)>& observer)
{
    mouse_movement_observers_.push_back(observer);
}

} // namespace Soldank
