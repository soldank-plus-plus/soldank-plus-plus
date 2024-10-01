#include "Keyboard.hpp"

#include <GLFW/glfw3.h>

namespace Soldank
{
std::array<bool, 348> Keyboard::keys_ = {};
std::array<bool, 348> Keyboard::keys_changed_ = {};
std::vector<std::function<void(int, int)>> Keyboard::key_observers_{};

void Keyboard::KeyCallback(GLFWwindow* /*window*/,
                           const int key,
                           const int /*scancode*/,
                           const int action,
                           const int /*mods*/)
{
    for (const auto& key_observer : key_observers_) {
        key_observer(key, action);
    }

    if (GLFW_RELEASE != action) {
        if (!keys_.at(key)) {
            keys_.at(key) = true;
        }
    } else {
        keys_.at(key) = false;
    }
    keys_changed_.at(key) = action != GLFW_RELEASE;
}

bool Keyboard::Key(const int key)
{
    return keys_.at(key);
}

bool Keyboard::KeyChanged(int key)
{
    const bool ret = keys_changed_.at(key);
    keys_changed_.at(key) = false;
    return ret;
}

bool Keyboard::KeyWentUp(const int key)
{
    return !keys_.at(key) && KeyChanged(key);
}

bool Keyboard::KeyWentDown(const int key)
{
    return keys_.at(key) && KeyChanged(key);
}

void Keyboard::SubscribeKeyObserver(const std::function<void(int, int)>& observer)
{
    key_observers_.push_back(observer);
}
} // namespace Soldank