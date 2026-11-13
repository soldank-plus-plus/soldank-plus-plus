module;

#include <cstdint>
#include <memory>

export module Networking.HitSoldierNetworkEventHandler;

import Shared.Core.IWorld;
import Shared.Core.Entities.Soldier;

import Shared.Networking.NetworkEventDispatcher;
import Shared.Networking.NetworkEvent;

export namespace Soldank
{
class HitSoldierNetworkEventHandler : public NetworkEventHandlerBase<std::uint8_t, float>
{
public:
    HitSoldierNetworkEventHandler(const std::shared_ptr<IWorld>& world)
        : world_(world)
    {
    }

private:
    NetworkEvent GetTargetNetworkEvent() const override { return NetworkEvent::HitSoldier; }

    NetworkEventHandlerResult HandleNetworkMessageImpl(
      // TODO: remove from client: sender_connection_id
      unsigned int /*sender_connection_id*/,
      std::uint8_t soldier_id,
      float damage) override
    {
        // Once absolute combat snapshots are available, a late legacy delta must not
        // subtract health already represented by a newer authoritative snapshot.
        if (!world_->GetSoldier(soldier_id).has_authoritative_health_snapshot) {
            world_->HitSoldier(soldier_id, damage);
        }
        return NetworkEventHandlerResult::Success;
    }

    std::shared_ptr<IWorld> world_;
};
} // namespace Soldank
