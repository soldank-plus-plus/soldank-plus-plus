module;

#include <array>
#include <cstdint>
#include <string>

export module Application.ServerState;

import Shared.Core.Config.Config;

export namespace Soldank
{
struct ServerState
{
    std::array<unsigned int, Config::MAX_PLAYERS> last_processed_input_id;

    std::string server_name;
    std::uint16_t server_port;
};
} // namespace Soldank
