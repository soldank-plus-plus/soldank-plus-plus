module;

#include <memory>

export module Networking.EventHandlers.PingCheckNetworkEventHandler;

import Networking.IGameServer;

import Shared.Networking.NetworkEventDispatcher;
import Shared.Networking.NetworkEvent;

export namespace Soldank
{
class PingCheckNetworkEventHandler : public NetworkEventHandlerBase<>
{
public:
    PingCheckNetworkEventHandler(const std::shared_ptr<IGameServer>& game_server)
        : game_server_(game_server)
    {
    }

private:
    NetworkEvent GetTargetNetworkEvent() const override { return NetworkEvent::PingCheck; }

    NetworkEventHandlerResult HandleNetworkMessageImpl(unsigned int sender_connection_id) override
    {
        game_server_->SendNetworkMessage(sender_connection_id, { NetworkEvent::PingCheck });
        return NetworkEventHandlerResult::Success;
    }

    std::shared_ptr<IGameServer> game_server_;
};
} // namespace Soldank
