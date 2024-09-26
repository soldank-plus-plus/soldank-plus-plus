#ifndef __COMMAND_LINE_PARAMETERS_HPP__
#define __COMMAND_LINE_PARAMETERS_HPP__

#include <optional>
#include <string>
#include <cstdint>

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
};

// NOLINTNEXTLINE modernize-avoid-c-arrays
ParsedValues Parse(int argc, const char* argv[]);
} // namespace Soldank::CommandLineParameters

#endif
