module;

#include <cstdint>
#include <string>

export module Application.ServerConfig;

export namespace Soldank
{
struct ServerConfig
{
    std::string server_name;
    std::uint16_t server_port = 0;
    std::string map_path = "maps/ctf_Ash.pms";
    int fps_limit = 240;
};
} // namespace Soldank
