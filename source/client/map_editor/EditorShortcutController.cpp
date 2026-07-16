module;

#include <GLFW/glfw3.h>

#include <optional>
#include <span>
#include <utility>
#include <vector>

export module MapEditor.EditorShortcutController;

import MapEditorState;
import Application.Input.Shortcut;

export namespace Soldank
{
class EditorShortcutController
{
public:
    bool IsSaveShortcut(int key) const { return is_holding_left_ctrl_ && key == GLFW_KEY_S; }

    bool IsRedoShortcut(int key) const
    {
        return is_holding_left_ctrl_ && is_holding_left_shift_ && key == GLFW_KEY_Z;
    }

    bool IsUndoShortcut(int key) const { return is_holding_left_ctrl_ && key == GLFW_KEY_Z; }

    bool IsMapSettingsShortcut(int key) const { return is_holding_left_ctrl_ && key == GLFW_KEY_M; }

    bool IsCopyShortcut(int key) const { return is_holding_left_ctrl_ && key == GLFW_KEY_C; }

    bool IsPasteShortcut(int key) const { return is_holding_left_ctrl_ && key == GLFW_KEY_V; }

    std::optional<ToolType> GetToolForKey(int key,
                                          int modifiers,
                                          std::span<const int> tool_shortcut_keys) const
    {
        for (std::size_t tool_index = 0; tool_index < tool_shortcut_keys.size(); ++tool_index) {
            if (EncodeShortcut(key, modifiers) == tool_shortcut_keys[tool_index]) {
                return static_cast<ToolType>(tool_index);
            }
        }
        return std::nullopt;
    }

    void OnKeyPressed(int key)
    {
        if (key == GLFW_KEY_LEFT_SHIFT) {
            is_holding_left_shift_ = true;
        }
        if (key == GLFW_KEY_LEFT_CONTROL) {
            is_holding_left_ctrl_ = true;
        }
    }

    void OnKeyReleased(int key)
    {
        if (key == GLFW_KEY_LEFT_SHIFT) {
            is_holding_left_shift_ = false;
        }
        if (key == GLFW_KEY_LEFT_CONTROL) {
            is_holding_left_ctrl_ = false;
        }
    }

private:
    bool is_holding_left_ctrl_ = false;
    bool is_holding_left_shift_ = false;
};
} // namespace Soldank
