module;

#include <GLFW/glfw3.h>

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

export module Application.Input.Shortcut;

export namespace Soldank
{
enum class ShortcutId : std::size_t
{
    MapEditorPlayMode,
    MapEditorToolTransform,
    MapEditorToolPolygon,
    MapEditorToolVertexSelection,
    MapEditorToolSelection,
    MapEditorToolVertexColor,
    MapEditorToolColor,
    MapEditorToolTexture,
    MapEditorToolScenery,
    MapEditorToolWaypoint,
    MapEditorToolSpawnpoint,
    MapEditorToolColorPicker,
    Count
};

struct ShortcutDefinition
{
    ShortcutId id;
    std::string_view category;
    std::string_view name;
    std::string_view config_key;
    int default_shortcut;
};

constexpr int SHORTCUT_MODIFIER_MASK =
  GLFW_MOD_CONTROL | GLFW_MOD_SHIFT | GLFW_MOD_ALT | GLFW_MOD_SUPER;
constexpr int SHORTCUT_MODIFIER_SHIFT = 16;
constexpr std::size_t SHORTCUT_COUNT = static_cast<std::size_t>(ShortcutId::Count);
using ShortcutBindings = std::array<int, SHORTCUT_COUNT>;

constexpr int EncodeShortcut(int key, int modifiers)
{
    return key == GLFW_KEY_UNKNOWN
             ? GLFW_KEY_UNKNOWN
             : key | ((modifiers & SHORTCUT_MODIFIER_MASK) << SHORTCUT_MODIFIER_SHIFT);
}
constexpr int GetShortcutKey(int shortcut)
{
    return shortcut == GLFW_KEY_UNKNOWN ? GLFW_KEY_UNKNOWN : shortcut & 0xFFFF;
}
constexpr int GetShortcutModifiers(int shortcut)
{
    return shortcut == GLFW_KEY_UNKNOWN
             ? 0
             : (shortcut >> SHORTCUT_MODIFIER_SHIFT) & SHORTCUT_MODIFIER_MASK;
}
constexpr bool IsShortcutModifierKey(int key)
{
    return key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL ||
           key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT || key == GLFW_KEY_LEFT_ALT ||
           key == GLFW_KEY_RIGHT_ALT || key == GLFW_KEY_LEFT_SUPER || key == GLFW_KEY_RIGHT_SUPER;
}
constexpr std::size_t GetShortcutIndex(ShortcutId id)
{
    return static_cast<std::size_t>(id);
}
constexpr ShortcutId GetToolShortcutId(std::size_t tool_index)
{
    return static_cast<ShortcutId>(GetShortcutIndex(ShortcutId::MapEditorToolTransform) +
                                   tool_index);
}
constexpr int GetShortcut(const ShortcutBindings& bindings, ShortcutId id)
{
    return bindings[GetShortcutIndex(id)];
}
constexpr void SetShortcut(ShortcutBindings& bindings, ShortcutId id, int shortcut)
{
    bindings[GetShortcutIndex(id)] = shortcut;
}
constexpr bool MatchesShortcut(int key, int modifiers, int shortcut)
{
    return EncodeShortcut(key, modifiers) == shortcut;
}
inline const std::array<ShortcutDefinition, SHORTCUT_COUNT>& GetShortcutDefinitions()
{
    static constexpr std::array<ShortcutDefinition, SHORTCUT_COUNT> DEFINITIONS{ {
      { ShortcutId::MapEditorPlayMode,
        "Play test",
        "Switch to play mode",
        "play_mode_key",
        GLFW_KEY_F5 },
      { ShortcutId::MapEditorToolTransform, "Tools", "Transform", "transform", GLFW_KEY_A },
      { ShortcutId::MapEditorToolPolygon, "Tools", "Polygon", "polygon", GLFW_KEY_Q },
      { ShortcutId::MapEditorToolVertexSelection,
        "Tools",
        "Vertex selection",
        "vertex_selection",
        GLFW_KEY_S },
      { ShortcutId::MapEditorToolSelection, "Tools", "Selection", "selection", GLFW_KEY_W },
      { ShortcutId::MapEditorToolVertexColor, "Tools", "Vertex color", "vertex_color", GLFW_KEY_D },
      { ShortcutId::MapEditorToolColor, "Tools", "Color", "color", GLFW_KEY_E },
      { ShortcutId::MapEditorToolTexture, "Tools", "Texture", "texture", GLFW_KEY_F },
      { ShortcutId::MapEditorToolScenery, "Tools", "Scenery", "scenery", GLFW_KEY_R },
      { ShortcutId::MapEditorToolWaypoint, "Tools", "Waypoint", "waypoint", GLFW_KEY_G },
      { ShortcutId::MapEditorToolSpawnpoint, "Tools", "Spawn point", "spawnpoint", GLFW_KEY_T },
      { ShortcutId::MapEditorToolColorPicker, "Tools", "Color picker", "color_picker", GLFW_KEY_H },
    } };
    return DEFINITIONS;
}
inline ShortcutBindings GetDefaultShortcutBindings()
{
    ShortcutBindings bindings{};
    for (const auto& definition : GetShortcutDefinitions()) {
        SetShortcut(bindings, definition.id, definition.default_shortcut);
    }
    return bindings;
}
inline std::string GetShortcutModifierPrefix(int modifiers)
{
    std::string result;
    if (modifiers & GLFW_MOD_CONTROL)
        result += "Ctrl + ";
    if (modifiers & GLFW_MOD_SHIFT)
        result += "Shift + ";
    if (modifiers & GLFW_MOD_ALT)
        result += "Alt + ";
    if (modifiers & GLFW_MOD_SUPER)
        result += "Windows + ";
    return result;
}
inline std::string GetShortcutDisplayName(int shortcut)
{
    if (shortcut == GLFW_KEY_UNKNOWN)
        return "";
    const int key = GetShortcutKey(shortcut);
    const std::string prefix = GetShortcutModifierPrefix(GetShortcutModifiers(shortcut));
    if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z)
        return prefix + char('A' + key - GLFW_KEY_A);
    if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9)
        return prefix + char('0' + key - GLFW_KEY_0);
    if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F12)
        return prefix + "F" + std::to_string(key - GLFW_KEY_F1 + 1);
    if (key == GLFW_KEY_SPACE)
        return prefix + "Space";
    if (key == GLFW_KEY_ENTER)
        return prefix + "Enter";
    if (key == GLFW_KEY_TAB)
        return prefix + "Tab";
    if (key == GLFW_KEY_ESCAPE)
        return prefix + "Escape";
    if (key == GLFW_KEY_BACKSPACE)
        return prefix + "Backspace";
    return prefix + "Key " + std::to_string(key);
}
} // namespace Soldank
