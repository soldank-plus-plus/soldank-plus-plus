module;

#include "communication/NetworkEventDispatcher.hpp"
#include "communication/NetworkEvent.hpp"

#include "core/IWorld.hpp"

#include <memory>

export module Networking.EventHandlers.KillCommandNetworkEventHandler;

import Networking.IGameServer;

export namespace Soldank
{
class KillCommandNetworkEventHandler : public NetworkEventHandlerBase<>
{
public:
    KillCommandNetworkEventHandler(const std::shared_ptr<IWorld>& world,
                                   const std::shared_ptr<IGameServer>& game_server)
        : world_(world)
        , game_server_(game_server)
    {
    }

private:
    NetworkEvent GetTargetNetworkEvent() const override { return NetworkEvent::KillCommand; }

    NetworkEventHandlerResult HandleNetworkMessageImpl(unsigned int sender_connection_id) override
    {
        unsigned int soldier_id = game_server_->GetSoldierIdFromConnectionId(sender_connection_id);
        game_server_->SendNetworkMessageToAll({ NetworkEvent::KillSoldier, soldier_id });
        world_->KillSoldier(soldier_id);
        return NetworkEventHandlerResult::Success;
    }

    std::shared_ptr<IWorld> world_;
    std::shared_ptr<IGameServer> game_server_;
};
} // namespace Soldank
