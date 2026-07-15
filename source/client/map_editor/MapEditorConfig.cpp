module;

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <span>
#include <utility>
#include <vector>

export module MapEditor.Config;

import Extern.Glm;
import Extern.Spdlog;
import Extern.TomlPlusPlus;

export namespace Soldank
{
class MapEditorConfig
{
public:
    static bool LoadPalette(const std::filesystem::path& file_path,
                            std::span<glm::vec4> palette_colors);
    static bool SavePalette(const std::filesystem::path& file_path,
                            std::span<const glm::vec4> palette_colors);
};
} // namespace Soldank

namespace Soldank
{
namespace
{
constexpr std::int64_t MIN_COLOR_CHANNEL = 0;
constexpr std::int64_t MAX_COLOR_CHANNEL = 255;

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
    config.insert("version", 1);
    config.insert("palette", std::move(palette));

    std::ofstream config_file(file_path, std::ios::out | std::ios::trunc);
    if (!config_file) {
        Spdlog::warn("Could not save map editor config: {}", file_path.string());
        return false;
    }
    config_file << Toml::Format(config);
    return config_file.good();
}
} // namespace Soldank
