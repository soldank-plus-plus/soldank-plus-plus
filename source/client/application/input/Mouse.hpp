#ifndef __MOUSE_HPP__
#define __MOUSE_HPP__

#include <functional>
#include <vector>

struct GLFWwindow;

namespace Soldank
{
class Mouse
{
public:
    static void CursorPosCallback(GLFWwindow* window, double x, double y);
    static void MouseButtonCallback(GLFWwindow* window,
                                    const int button,
                                    const int action,
                                    int mods);
    static void MouseWheelCallback(GLFWwindow* window, double dx, double dy);

    static double GetDx();
    static double GetDy();

    static double GetX();
    static double GetY();

    static bool Button(int button);

    static void SubscribeButtonObserver(const std::function<void(int, int)>& observer);
    static void SubscribeMouseMovementObserver(const std::function<void(double, double)>& observer);

private:
    static double x_;
    static double y_;

    static double dx_;
    static double dy_;

    static double scroll_dx_;
    static double scroll_dy_;

    static double lastx_;
    static double lasty_;

    static bool first_mouse_;
    static bool buttons_[]; // button state array (true for down, false for up)
    static bool buttons_changed_[];

    static std::vector<std::function<void(int, int)>> button_observers_;
    static std::vector<std::function<void(double, double)>> mouse_movement_observers_;
};
} // namespace Soldank
#endif