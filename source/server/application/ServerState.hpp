#ifndef __SERVER_STATE_HPP__
#define __SERVER_STATE_HPP__

#include "core/config/Config.hpp"

#include <array>
#include <string>

namespace Soldank
{
struct ServerState
{
    std::array<unsigned int, Config::MAX_PLAYERS> last_processed_input_id;
    std::array<std::int32_t, Config::MAX_PLAYERS> bullet_creation_count_per_tick_per_player;

    std::string server_name;
    std::uint16_t server_port;
};
} // namespace Soldank

#endif
