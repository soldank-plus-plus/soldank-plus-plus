module;

#include <cstdint>
#include <string>
#include <variant>

export module Events.ServerEvent;

export namespace Soldank
{
struct PlayerConnectedEvent
{
    std::uint32_t connection_id;
};

struct PlayerAuthenticatedEvent
{
    std::uint32_t connection_id;
    std::string nick;
};

struct PlayerDisconnectedEvent
{
    std::uint32_t connection_id;
    std::uint8_t soldier_id;
};

struct LobbyRegistrationEvent
{
    bool successful;
};

struct ScriptLifecycleEvent
{
    std::string script_name;
    bool loaded;
};

using ServerEvent = std::variant<PlayerConnectedEvent,
                                 PlayerAuthenticatedEvent,
                                 PlayerDisconnectedEvent,
                                 LobbyRegistrationEvent,
                                 ScriptLifecycleEvent>;
} // namespace Soldank
