module;

#include <cstdint>
#include <string>
#include <vector>

export module Application.ServerConfig;

export namespace Soldank
{
struct ServerConfig
{
    std::string server_name;
    std::uint16_t server_port = 0;
    std::string map_path = "maps/ctf_Ash.pms";
    int fps_limit = 60;
    std::vector<std::string> map_rotation;
    std::string game_mode{ "capture_the_flag" };
    bool survival{};
    bool clear_weapons{};
    bool friendly_fire{};
    int score_limit{};
    unsigned int time_limit{ 36000 };
    bool realistic{};
    std::uint8_t max_grenades{ 5 };
};
} // namespace Soldank
