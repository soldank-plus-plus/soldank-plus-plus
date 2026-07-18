#include <GLFW/glfw3.h>
#include <gtest/gtest.h>

#include <cstddef>

import Application.Input.Shortcut;

namespace
{
using namespace Soldank;

TEST(ShortcutTest, DefinitionsMatchTheirBindingSlotsAndDefaults)
{
    const auto& definitions = GetShortcutDefinitions();
    const ShortcutBindings defaults = GetDefaultShortcutBindings();

    EXPECT_EQ(definitions.size(), SHORTCUT_COUNT);
    for (std::size_t index = 0; index < definitions.size(); ++index) {
        const ShortcutDefinition& definition = definitions[index];
        EXPECT_EQ(GetShortcutIndex(definition.id), index);
        EXPECT_FALSE(definition.category.empty());
        EXPECT_FALSE(definition.name.empty());
        EXPECT_FALSE(definition.config_key.empty());
        EXPECT_EQ(GetShortcut(defaults, definition.id), definition.default_shortcut);
    }

    EXPECT_EQ(GetShortcut(defaults, ShortcutId::MapEditorSave),
              EncodeShortcut(GLFW_KEY_S, GLFW_MOD_CONTROL));
    EXPECT_EQ(GetShortcut(defaults, ShortcutId::MapEditorUndo),
              EncodeShortcut(GLFW_KEY_Z, GLFW_MOD_CONTROL));
}

TEST(ShortcutTest, EncodesAndMatchesKeyModifierCombinations)
{
    const int shortcut = EncodeShortcut(GLFW_KEY_F5, GLFW_MOD_CONTROL | GLFW_MOD_SHIFT);

    EXPECT_EQ(GetShortcutKey(shortcut), GLFW_KEY_F5);
    EXPECT_EQ(GetShortcutModifiers(shortcut), GLFW_MOD_CONTROL | GLFW_MOD_SHIFT);
    EXPECT_TRUE(MatchesShortcut(GLFW_KEY_F5, GLFW_MOD_CONTROL | GLFW_MOD_SHIFT, shortcut));
    EXPECT_FALSE(MatchesShortcut(GLFW_KEY_F5, GLFW_MOD_CONTROL, shortcut));
    EXPECT_FALSE(MatchesShortcut(GLFW_KEY_F4, GLFW_MOD_CONTROL | GLFW_MOD_SHIFT, shortcut));
}

TEST(ShortcutTest, FormatsShortcutsAndModifierCaptureText)
{
    EXPECT_EQ(GetShortcutDisplayName(EncodeShortcut(GLFW_KEY_C, GLFW_MOD_CONTROL | GLFW_MOD_SHIFT)),
              "Ctrl + Shift + C");
    EXPECT_EQ(GetShortcutDisplayName(GLFW_KEY_UNKNOWN), "");
    EXPECT_EQ(GetShortcutDisplayName(GLFW_KEY_LEFT), "Left");
    EXPECT_EQ(GetShortcutModifierPrefix(GLFW_MOD_ALT | GLFW_MOD_SUPER), "Alt + Windows + ");
}

TEST(ShortcutTest, RecognizesAllModifierKeys)
{
    EXPECT_TRUE(IsShortcutModifierKey(GLFW_KEY_LEFT_CONTROL));
    EXPECT_TRUE(IsShortcutModifierKey(GLFW_KEY_RIGHT_SHIFT));
    EXPECT_TRUE(IsShortcutModifierKey(GLFW_KEY_LEFT_ALT));
    EXPECT_TRUE(IsShortcutModifierKey(GLFW_KEY_RIGHT_SUPER));
    EXPECT_FALSE(IsShortcutModifierKey(GLFW_KEY_F5));
}
} // namespace
