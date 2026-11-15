module;

#include <limits>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <cstdint>
#include <sstream>

export module Application.ServerConfigLoader;

import Application.ServerConfig;

import Extern.SimpleIni;
import Extern.Spdlog;

export namespace Soldank
{
class ServerConfigLoader
{
public:
    static ServerConfig Load(const char* file_path = "soldat.ini")
    {
        SimpleIni::CSimpleIniA ini_config;
        const SimpleIni::SI_Error rc = ini_config.LoadFile(file_path);
        if (rc < 0) {
            Spdlog::critical("Error: INI File could not be loaded: {}", file_path);
            throw std::runtime_error("INI file could not be loaded");
        }

        ServerConfig config;
        config.server_port = static_cast<std::uint16_t>(ini_config.GetLongValue("NETWORK", "Port"));
        if (config.server_port == 0) {
            Spdlog::critical("Error: Port can't be 0");
            throw std::runtime_error("server port can't be 0");
        }

        const char* server_name_cstr = ini_config.GetValue("NETWORK", "Server_Name");
        if (server_name_cstr == nullptr) {
            Spdlog::critical("Error: Server_Name is missing");
            throw std::runtime_error("server name is missing");
        }
        config.server_name = server_name_cstr;

        const long fps_limit = ini_config.GetLongValue("NETWORK", "FPS_Limit", config.fps_limit);
        if (fps_limit <= 0 || fps_limit > std::numeric_limits<int>::max()) {
            Spdlog::warn("Invalid FPS_Limit: {}. Using default: {}", fps_limit, config.fps_limit);
        } else {
            config.fps_limit = static_cast<int>(fps_limit);
        }

        config.map_path = ini_config.GetValue("GAME", "Map", "maps/ctf_Ash.pms");
        std::stringstream rotation(ini_config.GetValue("GAME", "Map_Rotation", ""));
        std::string map;
        while (std::getline(rotation, map, ',')) {
            const auto first = map.find_first_not_of(' ');
            if (first != std::string::npos) {
                config.map_rotation.push_back(
                  map.substr(first, map.find_last_not_of(' ') - first + 1));
            }
        }
        if (!config.map_rotation.empty()) {
            config.map_path = config.map_rotation.front();
        }
        config.game_mode = ini_config.GetValue("GAME", "Game_Mode", "capture_the_flag");
        config.survival = ini_config.GetBoolValue("GAME", "Survival_Mode", false);
        config.clear_weapons = ini_config.GetBoolValue("GAME", "Survival_Clear_Weapons", false);
        config.friendly_fire = ini_config.GetBoolValue("GAME", "Friendly_Fire", false);
        const auto score_limit = ini_config.GetLongValue("GAME", "Score_Limit", 0);
        const auto time_limit = ini_config.GetLongValue("GAME", "Time_Limit_Ticks", 36000);
        if (score_limit < 0 || score_limit > 1000000 || time_limit < 0 || time_limit > 1000000000) {
            throw std::invalid_argument("Invalid game mode limit");
        }
        config.score_limit = static_cast<int>(score_limit);
        config.time_limit = static_cast<unsigned int>(time_limit);
        config.realistic = ini_config.GetBoolValue("GAME", "Realistic_Mode", false);
        config.max_grenades = static_cast<std::uint8_t>(
          std::clamp<long>(ini_config.GetLongValue("GAME", "Max_Grenades", 5), 0, 255));
        return config;
    }
};
} // namespace Soldank
