#ifndef __KEYBOARD_HPP__
#define __KEYBOARD_HPP__

#include <array>
#include <vector>
#include <functional>

struct GLFWwindow;

namespace Soldank
{
class Keyboard
{
public:
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static bool Key(int key);
    static bool KeyChanged(int key);
    static bool KeyWentUp(int key);
    static bool KeyWentDown(int key);

    static void SubscribeKeyObserver(const std::function<void(int, int)>& observer);

private:
    static std::array<bool, 348> keys_;
    static std::array<bool, 348> keys_changed_;
    static std::vector<std::function<void(int, int)>> key_observers_;
};
} // namespace Soldank

#endif
