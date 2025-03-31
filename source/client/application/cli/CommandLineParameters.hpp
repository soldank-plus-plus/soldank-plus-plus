#ifndef __COMMAND_LINE_PARAMETERS_HPP__
#define __COMMAND_LINE_PARAMETERS_HPP__

#include "application/config/ClientConfig.hpp"
#include <optional>
#include <string>
#include <vector>
#include <cstdint>

namespace Soldank
{
enum class WindowSizeMode : std::uint8_t;
}

namespace Soldank::CommandLineParameters
{
enum class ApplicationMode
{
    Default = 0,
    Local,
    Online,
    MapEditor,
};

struct ParsedValues
{
    bool is_parsing_successful = false;
    ApplicationMode application_mode = ApplicationMode::Default;
    std::string join_server_ip;
    std::uint16_t join_server_port;
    std::optional<std::string> map;
    WindowSizeMode window_size_mode;
    int fps_limit = ClientConfig::FPS_LIMIT;
    bool is_debug_ui_enabled = false;
};

ParsedValues Parse(const std::vector<const char*>& cli_parameters);
} // namespace Soldank::CommandLineParameters

#endif
