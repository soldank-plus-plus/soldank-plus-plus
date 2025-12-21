module;

#include <GLFW/glfw3.h>

#include <array>
#include <vector>
#include <functional>

export module Application.Input.Keyboard;

export namespace Soldank
{
class Keyboard
{
public:
    static void KeyCallback(GLFWwindow* /*window*/,
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

    static bool Key(const int key) { return keys_.at(key); }

    static bool KeyChanged(int key)
    {
        const bool ret = keys_changed_.at(key);
        keys_changed_.at(key) = false;
        return ret;
    }

    static bool KeyWentUp(const int key) { return !keys_.at(key) && KeyChanged(key); }

    static bool KeyWentDown(const int key) { return keys_.at(key) && KeyChanged(key); }

    static void SubscribeKeyObserver(const std::function<void(int, int)>& observer)
    {
        key_observers_.push_back(observer);
    }

private:
    static std::array<bool, 348> keys_;
    static std::array<bool, 348> keys_changed_;
    static std::vector<std::function<void(int, int)>> key_observers_;
};
} // namespace Soldank

namespace Soldank
{
std::array<bool, 348> Keyboard::keys_ = {};
std::array<bool, 348> Keyboard::keys_changed_ = {};
std::vector<std::function<void(int, int)>> Keyboard::key_observers_{};
} // namespace Soldank
