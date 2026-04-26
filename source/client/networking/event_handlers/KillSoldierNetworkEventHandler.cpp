module;

#include <chrono>

export module Networking.KillSoldierNetworkEventHandler;

import Shared.IWorld;

import Shared.Networking.NetworkEventDispatcher;
import Shared.Networking.NetworkEvent;

export namespace Soldank
{
class KillSoldierNetworkEventHandler : public NetworkEventHandlerBase<std::uint8_t>
{
public:
    KillSoldierNetworkEventHandler(const std::shared_ptr<IWorld>& world)
        : world_(world)
    {
    }

private:
    NetworkEvent GetTargetNetworkEvent() const override { return NetworkEvent::KillSoldier; }

    NetworkEventHandlerResult HandleNetworkMessageImpl(
      // TODO: remove from client: sender_connection_id
      unsigned int /*sender_connection_id*/,
      std::uint8_t soldier_id) override
    {
        world_->KillSoldier(soldier_id);
        return NetworkEventHandlerResult::Success;
    }

    std::shared_ptr<IWorld> world_;
};
} // namespace Soldank
