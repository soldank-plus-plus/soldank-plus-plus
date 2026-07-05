module;

#include "core/math/Glm.hpp"

#include <span>

export module Application.Input.InputSnapshot;

export namespace Soldank
{
struct KeyStateChange
{
    int key;
    int action;
};

struct MouseButtonStateChange
{
    int button;
    int action;
};

struct InputSnapshot
{
    glm::vec2 mouse_screen_position;
    glm::vec2 mouse_map_position;
    bool left_mouse_down = false;
    bool right_mouse_down = false;
    bool middle_mouse_down = false;
    std::span<const KeyStateChange> key_changes;
    std::span<const MouseButtonStateChange> mouse_button_changes;
    float mouse_wheel_delta = 0.0F;
};

enum class InputContext
{
    Gameplay,
    Editor,
    EditorPlayTest,
    UiCaptured
};
} // namespace Soldank
