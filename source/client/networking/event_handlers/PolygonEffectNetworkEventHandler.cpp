module;

#include <memory>
#include <cmath>
#include <utility>

export module Networking.PolygonEffectNetworkEventHandler;

import Shared.Core.IWorld;
import Shared.Core.Physics.Polygons.PolygonContact;
import Shared.Networking.NetworkPackets;
import Shared.Networking.NetworkEvent;
import Shared.Networking.NetworkEventDispatcher;

export namespace Soldank
{
class PolygonEffectNetworkEventHandler final : public NetworkEventHandlerBase<PolygonEffectPacket>
{
public:
    explicit PolygonEffectNetworkEventHandler(std::shared_ptr<IWorld> world)
        : world_(std::move(world))
    {
    }

protected:
    NetworkEvent GetTargetNetworkEvent() const override { return NetworkEvent::PolygonEffect; }
    NetworkEventHandlerResult HandleNetworkMessageImpl(unsigned int /*sender*/,
                                                       PolygonEffectPacket packet) override
    {
        auto event = packet.event;
        if (event.kind > PolygonEventKind::Death || event.soldier >= 32 ||
            event.severity > DeathSeverity::Brutal || !std::isfinite(event.amount) ||
            !std::isfinite(event.position[0]) || !std::isfinite(event.position[1]) ||
            event.polygon >= world_->GetStateManager()->GetConstMap().GetPolygonsCount() ||
            (packet.map_fingerprint != 0 &&
             packet.map_fingerprint !=
               world_->GetStateManager()->GetConstMap().GetPolygonFingerprint())) {
            return NetworkEventHandlerResult::Failure;
        }
        // This notification is presentation only. Health and spawns arrive through authority
        // packets.
        if (packet.map_fingerprint != 0) {
            event.map_fingerprint = packet.map_fingerprint;
        }
        world_->GetPhysicsEvents().polygon_event.Notify(event);
        return NetworkEventHandlerResult::Success;
    }

private:
    std::shared_ptr<IWorld> world_;
};
} // namespace Soldank
