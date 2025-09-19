module;

#include "communication/NetworkMessage.hpp"

export module Networking.IGameServer;

export namespace Soldank
{
class IGameServer
{
public:
    virtual ~IGameServer() = default;
    virtual void Update() = 0;

    virtual void SendNetworkMessage(unsigned int connection_id,
                                    const NetworkMessage& network_message) = 0;
    virtual void SendNetworkMessageToAll(const NetworkMessage& network_message) = 0;

    virtual unsigned int GetSoldierIdFromConnectionId(unsigned int connection_id) = 0;
};
} // namespace Soldank
