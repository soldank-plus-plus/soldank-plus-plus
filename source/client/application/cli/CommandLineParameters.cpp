#include "application/cli/CommandLineParameters.hpp"

#include "spdlog/spdlog.h"

#include <cxxopts.hpp>

#include <iostream>

namespace Soldank::CommandLineParameters
{
// NOLINTNEXTLINE modernize-avoid-c-arrays
ParsedValues Parse(int argc, const char* argv[])
{
    ParsedValues parsed_values;

    try {
        std::unique_ptr<cxxopts::Options> allocated(
          // NOLINTNEXTLINE cppcoreguidelines-pro-bounds-pointer-arithmetic
          new cxxopts::Options(argv[0], "Soldank++ client"));
        auto& options = *allocated;
        options.positional_help("[optional args]").show_positional_help();

        // clang-format off
        options
            .set_width(80)
            .add_options()
            ("help", "Print help")
            ("l,local", "Start local game")
            ("o,online",
                "Start online game. You need to provide IP and Port with --ip and --port options")
            ("e,map-editor", "Opens the application in map editting mode")
            ("ip", "IP of the server to join", cxxopts::value<std::string>())
            ("port", "Port of the server to join", cxxopts::value<std::uint16_t>())
            ("m,map", "Choose a map to load at the start of the game. Only in local", cxxopts::value<std::string>());
        // clang-format on

        auto result = options.parse(argc, argv);

        if (result.count("help") != 0) {
            std::cout << options.help({ "", "Group" }) << std::endl;
            return parsed_values;
        }

        if (result.count("local") + result.count("online") + result.count("mapeditor") > 1) {
            std::cout << "Options --local and --online can't be used at the same time" << std::endl;
        }

        if (result.count("local") != 0) {
            parsed_values.application_mode = ApplicationMode::Local;

            if (result.count("map") == 0) {
                std::cout << "Map is missing" << std::endl;
                return parsed_values;
            }
        }

        if (result.count("online") != 0) {
            parsed_values.application_mode = ApplicationMode::Online;

            if (result.count("ip") == 0) {
                std::cout << "IP is missing" << std::endl;
                return parsed_values;
            }

            if (result.count("port") == 0) {
                std::cout << "Port is missing" << std::endl;
                return parsed_values;
            }

            parsed_values.join_server_ip = result["ip"].as<std::string>();
            parsed_values.join_server_port = result["port"].as<std::uint16_t>();
        }

        if (result.count("map-editor") != 0) {
            parsed_values.application_mode = ApplicationMode::MapEditor;
        }

        if (result.count("map") != 0) {
            parsed_values.map = result["map"].as<std::string>();
        }

        parsed_values.is_parsing_successful = true;
        return parsed_values;
    } catch (const cxxopts::exceptions::exception& e) {
        spdlog::critical("error parsing options {}", e.what());
        return parsed_values;
    }
    return parsed_values;
}
} // namespace Soldank::CommandLineParameters
