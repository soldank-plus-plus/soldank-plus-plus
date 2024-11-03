#ifndef __COMMAND_LINE_PARAMETERS_HPP__
#define __COMMAND_LINE_PARAMETERS_HPP__

#include "application/config/Config.hpp"
#include <optional>
#include <string>
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
    int fps_limit = Config::FPS_LIMIT;
};

// NOLINTNEXTLINE modernize-avoid-c-arrays
ParsedValues Parse(int argc, const char* argv[]);
} // namespace Soldank::CommandLineParameters

#endif
