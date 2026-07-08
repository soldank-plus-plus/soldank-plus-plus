module;

#include <GLFW/glfw3.h>

#include <optional>
#include <utility>
#include <vector>

export module MapEditor.EditorShortcutController;

import MapEditorState;

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

    std::optional<ToolType> GetToolForKey(int key) const
    {
        static const std::vector<std::pair<int, ToolType>> key_to_tool_type_map = {
            { GLFW_KEY_A, ToolType::Transform },       { GLFW_KEY_Q, ToolType::Polygon },
            { GLFW_KEY_S, ToolType::VertexSelection }, { GLFW_KEY_W, ToolType::Selection },
            { GLFW_KEY_D, ToolType::VertexColor },     { GLFW_KEY_E, ToolType::Color },
            { GLFW_KEY_F, ToolType::Texture },         { GLFW_KEY_R, ToolType::Scenery },
            { GLFW_KEY_G, ToolType::Waypoint },        { GLFW_KEY_T, ToolType::Spawnpoint },
            { GLFW_KEY_H, ToolType::ColorPicker },
        };

        for (const auto& [tool_key, tool_type] : key_to_tool_type_map) {
            if (key == tool_key) {
                return tool_type;
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
