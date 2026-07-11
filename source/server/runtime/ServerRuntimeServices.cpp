module;

#include <cstdint>
#include <string>

export module Runtime.ServerRuntimeServices;

import Shared.Networking.NetworkMessage;

export namespace Soldank
{
class IServerNetworkHost
{
public:
    virtual ~IServerNetworkHost() = default;

    virtual void Update() = 0;
    virtual void SendNetworkMessage(unsigned int connection_id,
                                    const NetworkMessage& network_message) = 0;
    virtual void SendNetworkMessageToAll(const NetworkMessage& network_message) = 0;
    virtual unsigned int GetSoldierIdFromConnectionId(unsigned int connection_id) = 0;
};

class ILobbyRegistrationClient
{
public:
    virtual ~ILobbyRegistrationClient() = default;

    virtual void Register(const std::string& server_name, std::uint16_t server_port) = 0;
};
} // namespace Soldank
