module;

#include <memory>

export module Networking.EventHandlers.KillCommandNetworkEventHandler;

import Networking.IGameServer;
import Runtime.ServerCommandQueues;

import Shared.Core.Simulation.SimulationCommands;

import Shared.Networking.NetworkEventDispatcher;
import Shared.Networking.NetworkEvent;

export namespace Soldank
{
class KillCommandNetworkEventHandler : public NetworkEventHandlerBase<>
{
public:
    KillCommandNetworkEventHandler(const std::shared_ptr<IGameServer>& game_server,
                                   ServerCommandQueues& command_queues)
        : game_server_(game_server)
        , command_queues_(command_queues)
    {
    }

private:
    NetworkEvent GetTargetNetworkEvent() const override { return NetworkEvent::KillCommand; }

    NetworkEventHandlerResult HandleNetworkMessageImpl(unsigned int sender_connection_id) override
    {
        unsigned int soldier_id = game_server_->GetSoldierIdFromConnectionId(sender_connection_id);
        game_server_->SendNetworkMessageToAll({ NetworkEvent::KillSoldier, soldier_id });
        command_queues_.EnqueueSimulationCommand(KillSoldierCommand{
          .soldier_id = static_cast<std::uint8_t>(soldier_id),
        });
        return NetworkEventHandlerResult::Success;
    }

    std::shared_ptr<IGameServer> game_server_;
    ServerCommandQueues& command_queues_;
};
} // namespace Soldank
