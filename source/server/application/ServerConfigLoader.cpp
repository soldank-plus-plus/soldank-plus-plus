module;

#include <limits>
#include <stdexcept>
#include <string>

export module Application.ServerConfigLoader;

import Application.ServerConfig;

import Extern.SimpleIni;
import Extern.Spdlog;

export namespace Soldank
{
class ServerConfigLoader
{
public:
    ServerConfig Load(const char* file_path = "soldat.ini") const
    {
        SimpleIni::CSimpleIniA ini_config;
        SimpleIni::SI_Error rc = ini_config.LoadFile(file_path);
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

        const long fps_limit =
          ini_config.GetLongValue("NETWORK", "FPS_Limit", config.fps_limit);
        if (fps_limit <= 0 || fps_limit > std::numeric_limits<int>::max()) {
            Spdlog::warn("Invalid FPS_Limit: {}. Using default: {}", fps_limit, config.fps_limit);
        } else {
            config.fps_limit = static_cast<int>(fps_limit);
        }

        return config;
    }
};
} // namespace Soldank
