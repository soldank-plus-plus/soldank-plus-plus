module;

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <GLFW/glfw3.h>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

export module MapEditor.Config;

import Extern.Glm;
import Extern.Spdlog;
import Extern.TomlPlusPlus;

import MapEditorState;
import Application.Input.Shortcut;

export namespace Soldank
{
class MapEditorConfig
{
public:
    static bool LoadPalette(const std::filesystem::path& file_path,
                            std::span<glm::vec4> palette_colors);
    static bool SavePalette(const std::filesystem::path& file_path,
                            std::span<const glm::vec4> palette_colors);
    static bool LoadSettings(const std::filesystem::path& file_path,
                             std::span<glm::vec4> palette_colors,
                             int& play_mode_shortcut_key,
                             float& ui_scale,
                             std::span<int> tool_shortcut_keys,
                             std::span<int> menu_shortcut_keys = {});
    static bool SaveSettings(const std::filesystem::path& file_path,
                             std::span<const glm::vec4> palette_colors,
                             int play_mode_shortcut_key,
                             float ui_scale,
                             std::span<const int> tool_shortcut_keys,
                             std::span<const int> menu_shortcut_keys = {});
};
} // namespace Soldank

namespace Soldank
{
namespace
{
constexpr std::int64_t MIN_COLOR_CHANNEL = 0;
constexpr std::int64_t MAX_COLOR_CHANNEL = 255;
constexpr float MIN_UI_SCALE = 0.5F;
constexpr float MAX_UI_SCALE = 2.0F;

bool IsShortcutKey(int key)
{
    return key >= GLFW_KEY_SPACE && key <= GLFW_KEY_LAST;
}

bool IsShortcutKeyOrUnassigned(int key)
{
    if (key == GLFW_KEY_UNKNOWN) {
        return true;
    }

    const int shortcut_key = GetShortcutKey(key);
    const int shortcut_modifiers = GetShortcutModifiers(key);
    return IsShortcutKey(shortcut_key) && !IsShortcutModifierKey(shortcut_key) &&
           key == EncodeShortcut(shortcut_key, shortcut_modifiers);
}

bool IsUiScaleValid(float ui_scale)
{
    return ui_scale >= MIN_UI_SCALE && ui_scale <= MAX_UI_SCALE;
}

std::int64_t ToColorChannel(float value)
{
    return static_cast<std::int64_t>(
      std::lround(std::clamp(value, 0.0F, 1.0F) * static_cast<float>(MAX_COLOR_CHANNEL)));
}
} // namespace

bool MapEditorConfig::LoadPalette(const std::filesystem::path& file_path,
                                  std::span<glm::vec4> palette_colors)
{
    if (!std::filesystem::exists(file_path)) {
        return false;
    }

    try {
        Toml::Table config = Toml::ParseFile(file_path.string());
        const Toml::Array* colors = config["palette"]["colors"].as_array();
        if (colors == nullptr || colors->size() != palette_colors.size()) {
            Spdlog::warn("Map editor config contains an invalid palette: {}", file_path.string());
            return false;
        }

        std::vector<glm::vec4> loaded_colors(palette_colors.size());

        for (std::size_t color_index = 0; color_index < colors->size(); ++color_index) {
            const Toml::Array* color = colors->get(color_index)->as_array();
            if (color == nullptr || color->size() != 4) {
                Spdlog::warn("Map editor config contains an invalid palette color: {}",
                             file_path.string());
                return false;
            }

            glm::vec4 loaded_color;
            for (std::size_t channel_index = 0; channel_index < 4; ++channel_index) {
                auto channel = color->get(channel_index)->value<std::int64_t>();
                if (!channel || *channel < MIN_COLOR_CHANNEL || *channel > MAX_COLOR_CHANNEL) {
                    Spdlog::warn("Map editor config contains an invalid color channel: {}",
                                 file_path.string());
                    return false;
                }
                loaded_color[channel_index] =
                  static_cast<float>(*channel) / static_cast<float>(MAX_COLOR_CHANNEL);
            }
            loaded_colors.at(color_index) = loaded_color;
        }

        std::ranges::copy(loaded_colors, palette_colors.begin());
        return true;
    } catch (const std::exception& exception) {
        Spdlog::warn(
          "Could not load map editor config '{}': {}", file_path.string(), exception.what());
        return false;
    }
}

bool MapEditorConfig::SavePalette(const std::filesystem::path& file_path,
                                  std::span<const glm::vec4> palette_colors)
{
    Toml::Array colors;
    for (const glm::vec4& palette_color : palette_colors) {
        Toml::Array color;
        for (std::size_t channel_index = 0; channel_index < 4; ++channel_index) {
            color.push_back(ToColorChannel(palette_color[channel_index]));
        }
        colors.push_back(std::move(color));
    }

    Toml::Table palette;
    palette.insert("colors", std::move(colors));
    Toml::Table config;
    config.insert("version", std::int64_t{ 1 });
    config.insert("palette", std::move(palette));

    std::ofstream config_file(file_path, std::ios::out | std::ios::trunc);
    if (!config_file) {
        Spdlog::warn("Could not save map editor config: {}", file_path.string());
        return false;
    }
    config_file << Toml::Format(config);
    return config_file.good();
}

bool MapEditorConfig::LoadSettings(const std::filesystem::path& file_path,
                                   std::span<glm::vec4> palette_colors,
                                   int& play_mode_shortcut_key,
                                   float& ui_scale,
                                   std::span<int> tool_shortcut_keys,
                                   std::span<int> menu_shortcut_keys)
{
    const bool palette_loaded = LoadPalette(file_path, palette_colors);
    if (!std::filesystem::exists(file_path)) {
        return palette_loaded;
    }

    try {
        const Toml::Table config = Toml::ParseFile(file_path.string());
        const auto shortcut_key = config["shortcuts"]["play_mode_key"].value<std::int64_t>();
        if (shortcut_key) {
            const int loaded_shortcut_key = static_cast<int>(*shortcut_key);
            if (!IsShortcutKeyOrUnassigned(loaded_shortcut_key)) {
                Spdlog::warn("Map editor config contains an invalid play mode shortcut: {}",
                             file_path.string());
            } else {
                play_mode_shortcut_key = loaded_shortcut_key;
            }
        }

        const auto loaded_ui_scale = config["general"]["ui_scale"].value<double>();
        if (loaded_ui_scale) {
            const float loaded_ui_scale_float = static_cast<float>(*loaded_ui_scale);
            if (!IsUiScaleValid(loaded_ui_scale_float)) {
                Spdlog::warn("Map editor config contains an invalid UI scale: {}",
                             file_path.string());
            } else {
                ui_scale = loaded_ui_scale_float;
            }
        }

        for (std::size_t tool_index = 0; tool_index < tool_shortcut_keys.size(); ++tool_index) {
            const auto tool_shortcut =
              config["shortcuts"]["tools"][GetShortcutDefinitions()[tool_index + 1].config_key]
                .value<std::int64_t>();
            if (!tool_shortcut) {
                continue;
            }

            const int loaded_tool_shortcut = static_cast<int>(*tool_shortcut);
            if (!IsShortcutKeyOrUnassigned(loaded_tool_shortcut)) {
                Spdlog::warn("Map editor config contains an invalid tool shortcut: {}",
                             file_path.string());
                continue;
            }
            tool_shortcut_keys[tool_index] = loaded_tool_shortcut;
        }
        for (std::size_t menu_index = 0; menu_index < menu_shortcut_keys.size(); ++menu_index) {
            const auto menu_shortcut =
              config["shortcuts"]["menu"][GetShortcutDefinitions()[menu_index + 12].config_key]
                .value<std::int64_t>();
            if (menu_shortcut && IsShortcutKeyOrUnassigned(static_cast<int>(*menu_shortcut))) {
                menu_shortcut_keys[menu_index] = static_cast<int>(*menu_shortcut);
            }
        }
    } catch (const std::exception& exception) {
        Spdlog::warn(
          "Could not load map editor settings '{}': {}", file_path.string(), exception.what());
    }

    return palette_loaded;
}

bool MapEditorConfig::SaveSettings(const std::filesystem::path& file_path,
                                   std::span<const glm::vec4> palette_colors,
                                   int play_mode_shortcut_key,
                                   float ui_scale,
                                   std::span<const int> tool_shortcut_keys,
                                   std::span<const int> menu_shortcut_keys)
{
    if (!IsShortcutKeyOrUnassigned(play_mode_shortcut_key)) {
        Spdlog::warn("Could not save an invalid play mode shortcut: {}", play_mode_shortcut_key);
        return false;
    }
    if (!IsUiScaleValid(ui_scale)) {
        Spdlog::warn("Could not save an invalid UI scale: {}", ui_scale);
        return false;
    }
    if (!std::ranges::all_of(tool_shortcut_keys, IsShortcutKeyOrUnassigned)) {
        Spdlog::warn("Could not save invalid tool shortcuts");
        return false;
    }

    Toml::Array colors;
    for (const glm::vec4& palette_color : palette_colors) {
        Toml::Array color;
        for (std::size_t channel_index = 0; channel_index < 4; ++channel_index) {
            color.push_back(ToColorChannel(palette_color[channel_index]));
        }
        colors.push_back(std::move(color));
    }

    Toml::Table palette;
    palette.insert("colors", std::move(colors));
    Toml::Table shortcuts;
    shortcuts.insert("play_mode_key", static_cast<std::int64_t>(play_mode_shortcut_key));
    Toml::Table tool_shortcuts;
    for (std::size_t tool_index = 0; tool_index < tool_shortcut_keys.size(); ++tool_index) {
        tool_shortcuts.insert(GetShortcutDefinitions()[tool_index + 1].config_key,
                              static_cast<std::int64_t>(tool_shortcut_keys[tool_index]));
    }
    shortcuts.insert("tools", std::move(tool_shortcuts));
    Toml::Table menu_shortcuts;
    for (std::size_t menu_index = 0; menu_index < menu_shortcut_keys.size(); ++menu_index) {
        menu_shortcuts.insert(GetShortcutDefinitions()[menu_index + 12].config_key,
                              static_cast<std::int64_t>(menu_shortcut_keys[menu_index]));
    }
    shortcuts.insert("menu", std::move(menu_shortcuts));
    Toml::Table general;
    general.insert("ui_scale", static_cast<double>(ui_scale));
    Toml::Table config;
    config.insert("version", std::int64_t{ 1 });
    config.insert("palette", std::move(palette));
    config.insert("shortcuts", std::move(shortcuts));
    config.insert("general", std::move(general));

    std::ofstream config_file(file_path, std::ios::out | std::ios::trunc);
    if (!config_file) {
        Spdlog::warn("Could not save map editor config: {}", file_path.string());
        return false;
    }
    config_file << Toml::Format(config);
    return config_file.good();
}
} // namespace Soldank
