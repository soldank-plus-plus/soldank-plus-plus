module;

#include "application/config/Config.hpp"

#include <charconv>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

export module Application.LaunchParameters;

import Application.Window;

export namespace Soldank
{
enum class ApplicationMode
{
    Local,
    Online,
    MapEditor,
};

struct ServerEndpoint
{
    std::string ip;
    std::uint16_t port;
};

struct ParsedLaunchParameters
{
    bool is_parsing_successful = false;
    ApplicationMode application_mode = ApplicationMode::Local;
    std::optional<ServerEndpoint> server_endpoint;
    std::optional<std::string> map;
    WindowSizeMode window_size_mode = WindowBackendAdapter::GetDefaultWindowSizeMode();
    int fps_limit = Config::FPS_LIMIT;
    bool is_debug_ui_enabled = false;
};

using ParameterValues = std::unordered_map<std::string, std::string>;

class ILaunchParameters
{
public:
    virtual ~ILaunchParameters() = default;

    virtual ParsedLaunchParameters Parse() const = 0;
};

ParsedLaunchParameters ParseLaunchParameters(const ParameterValues& parameter_values)
{
    ParsedLaunchParameters parsed_values;

    const auto has = [&parameter_values](std::string_view name) {
        return parameter_values.contains(std::string{ name });
    };
    const auto get = [&parameter_values](std::string_view name) -> std::optional<std::string_view> {
        const auto it = parameter_values.find(std::string{ name });
        if (it == parameter_values.end()) {
            return std::nullopt;
        }
        return it->second;
    };
    const auto parse_int = [](std::string_view value, int& result) {
        const auto [end, error] =
          std::from_chars(value.data(), value.data() + value.size(), result);
        return error == std::errc{} && end == value.data() + value.size();
    };

    const int selected_mode_count = static_cast<int>(has("local")) +
                                    static_cast<int>(has("online")) +
                                    static_cast<int>(has("map-editor"));
    if (selected_mode_count > 1) {
        return parsed_values;
    }

    if (has("online")) {
        parsed_values.application_mode = ApplicationMode::Online;
    } else if (has("map-editor")) {
        parsed_values.application_mode = ApplicationMode::MapEditor;
    }

    if (const auto map = get("map")) {
        if (map->empty()) {
            return parsed_values;
        }
        parsed_values.map = std::string{ *map };
    }

    if (has("local") && !parsed_values.map) {
        return parsed_values;
    }

    if (parsed_values.application_mode == ApplicationMode::Online) {
        const auto ip = get("ip");
        const auto port_text = get("port");
        int port = 0;
        if (!ip || ip->empty() || !port_text || !parse_int(*port_text, port) || port <= 0 ||
            port > std::numeric_limits<std::uint16_t>::max()) {
            return parsed_values;
        }
        parsed_values.server_endpoint =
          ServerEndpoint{ .ip = std::string{ *ip }, .port = static_cast<std::uint16_t>(port) };
    }

    if (has("fullscreen")) {
        parsed_values.window_size_mode = WindowSizeMode::Fullscreen;
    } else if (has("borderless")) {
        parsed_values.window_size_mode = WindowSizeMode::BorderlessFullscreen;
    } else if (has("windowed")) {
        parsed_values.window_size_mode = WindowSizeMode::Windowed;
    }

    if (const auto max_fps = get("max-fps")) {
        if (!parse_int(*max_fps, parsed_values.fps_limit)) {
            return parsed_values;
        }
    }

    parsed_values.is_debug_ui_enabled = has("debug-ui");
    parsed_values.is_parsing_successful = true;
    return parsed_values;
}
} // namespace Soldank
