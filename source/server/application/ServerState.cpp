module;

#include <cstdint>
#include <string>

export module Application.ServerState;

export namespace Soldank
{
struct ServerState
{
    std::string server_name;
    std::uint16_t server_port;
};
} // namespace Soldank
