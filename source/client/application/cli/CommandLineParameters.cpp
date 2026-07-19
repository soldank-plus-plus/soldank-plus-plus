module;

#include <cxxopts.hpp>

#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

export module Application.CLI.CommandLineParameters;

import Application.LaunchParameters;

import Extern.Spdlog;

export namespace Soldank
{
class CommandLineParameters final : public ILaunchParameters
{
public:
    explicit CommandLineParameters(std::vector<const char*> cli_parameters)
        : cli_parameters_(std::move(cli_parameters))
    {
    }

    ParsedLaunchParameters Parse() const override;

private:
    std::vector<const char*> cli_parameters_;
};

ParsedLaunchParameters CommandLineParameters::Parse() const
{
    ParsedLaunchParameters parsed_values;

    try {
        std::unique_ptr<cxxopts::Options> allocated(
          new cxxopts::Options(cli_parameters_[0], "Soldank++ client"));
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
            ("port", "Port of the server to join", cxxopts::value<std::string>())
            ("m,map", "Choose a map to load at the start of the game. Only in local", cxxopts::value<std::string>())
            ("fullscreen", "Start the game in fullscreen")
            ("borderless", "Start the game in borderless (windowed) fullscreen")
            ("windowed", "Start the game in window of a fixed size")
            ("max-fps", "Set FPS limit. Set max-fps to 0 for no limit", cxxopts::value<std::string>())
            ("debug-ui", "Enable Debug UI");
        // clang-format on

        auto result = options.parse((int)cli_parameters_.size(), cli_parameters_.data());

        if (result.count("help") != 0) {
            std::cout << options.help({ "", "Group" }) << std::endl;
            return parsed_values;
        }

        ParameterValues parameter_values;
        const auto add_flag = [&result, &parameter_values](const char* name) {
            if (result.count(name) != 0) {
                parameter_values.emplace(name, "");
            }
        };
        const auto add_value = [&result, &parameter_values](const char* name) {
            if (result.count(name) != 0) {
                parameter_values.emplace(name, result[name].as<std::string>());
            }
        };
        add_flag("local");
        add_flag("online");
        add_flag("map-editor");
        add_flag("fullscreen");
        add_flag("borderless");
        add_flag("windowed");
        add_flag("debug-ui");
        add_value("ip");
        add_value("port");
        add_value("map");
        add_value("max-fps");
        return ParseLaunchParameters(parameter_values);
    } catch (const cxxopts::exceptions::exception& e) {
        Spdlog::critical("error parsing options {}", e.what());
        return parsed_values;
    }
    return parsed_values;
}
} // namespace Soldank
