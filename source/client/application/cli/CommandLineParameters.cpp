#include "application/cli/CommandLineParameters.hpp"

#include "application/window/Window.hpp"

#include "spdlog/spdlog.h"

#include <cxxopts.hpp>

#include <iostream>

namespace Soldank::CommandLineParameters
{
ParsedValues Parse(const std::vector<const char*>& cli_parameters)
{
    ParsedValues parsed_values;

    try {
        std::unique_ptr<cxxopts::Options> allocated(
          new cxxopts::Options(cli_parameters[0], "Soldank++ client"));
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
            ("e,map-editor", "Opens the application in map editing mode")
            ("ip", "IP of the server to join", cxxopts::value<std::string>())
            ("port", "Port of the server to join", cxxopts::value<std::uint16_t>())
            ("m,map", "Choose a map to load at the start of the game. Only in local", cxxopts::value<std::string>())
            ("fullscreen", "Start the game in fullscreen")
            ("borderless", "Start the game in borderless (windowed) fullscreen")
            ("windowed", "Start the game in window of a fixed size")
            ("max-fps", "Set FPS limit. Set max-fps to 0 for no limit", cxxopts::value<int>())
            ("debug-ui", "Enable Debug UI");
        // clang-format on

        auto result = options.parse((int)cli_parameters.size(), cli_parameters.data());

        if (result.count("help") != 0) {
            std::cout << options.help({ "", "Group" }) << std::endl;
            return parsed_values;
        }

        if (result.count("local") + result.count("online") + result.count("map-editor") > 1) {
            std::cout << "Options --local, --online and --map-editor can't be used at the same time"
                      << std::endl;
            return parsed_values;
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

        if (result.count("fullscreen") != 0) {
            parsed_values.window_size_mode = WindowSizeMode::Fullscreen;
        } else if (result.count("borderless") != 0) {
            parsed_values.window_size_mode = WindowSizeMode::BorderlessFullscreen;
        } else if (result.count("windowed") != 0) {
            parsed_values.window_size_mode = WindowSizeMode::Windowed;
        } else {
            parsed_values.window_size_mode = WindowSizeMode::Fullscreen;
        }

        if (result.count("max-fps") != 0) {
            parsed_values.fps_limit = result["max-fps"].as<int>();
        }

        if (result.count("debug-ui") != 0) {
            parsed_values.is_debug_ui_enabled = true;
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
