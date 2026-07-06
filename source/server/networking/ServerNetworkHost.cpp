module;

#include <memory>

export module Networking.ServerNetworkHost;

import Networking.IGameServer;

import Shared.Networking.NetworkMessage;

export namespace Soldank
{
class ServerNetworkHost
{
public:
    explicit ServerNetworkHost(const std::shared_ptr<IGameServer>& game_server)
        : game_server_(game_server)
    {
    }

    void Update() { game_server_->Update(); }

    void SendNetworkMessage(unsigned int connection_id, const NetworkMessage& network_message)
    {
        game_server_->SendNetworkMessage(connection_id, network_message);
    }

    void SendNetworkMessageToAll(const NetworkMessage& network_message)
    {
        game_server_->SendNetworkMessageToAll(network_message);
    }

    unsigned int GetSoldierIdFromConnectionId(unsigned int connection_id)
    {
        return game_server_->GetSoldierIdFromConnectionId(connection_id);
    }

private:
    std::shared_ptr<IGameServer> game_server_;
};
} // namespace Soldank
