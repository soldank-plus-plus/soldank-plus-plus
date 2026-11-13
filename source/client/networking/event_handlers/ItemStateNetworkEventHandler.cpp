module;

#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

export module Networking.ItemStateNetworkEventHandler;

import Shared.Core.IWorld;
import Shared.Core.Map.Polygons.PolygonBackgroundState;
import Shared.Core.Types.ItemType;
import Shared.Core.Utility.SerialNumber;
import Shared.Networking.NetworkEvent;
import Shared.Networking.NetworkEventDispatcher;
import Shared.Networking.NetworkPackets;

export namespace Soldank
{
class ItemStateNetworkEventHandler final : public NetworkEventHandlerBase<ItemStatePacket>
{
public:
    explicit ItemStateNetworkEventHandler(std::shared_ptr<IWorld> world)
        : world_(std::move(world))
    {
    }

protected:
    NetworkEvent GetTargetNetworkEvent() const override { return NetworkEvent::ItemState; }
    NetworkEventHandlerResult HandleNetworkMessageImpl(unsigned int /*sender*/,
                                                       ItemStatePacket packet) override
    {
        if (packet.id >= latest_ticks_.size() ||
            (packet.active && (packet.style < ItemType::AlphaFlag || packet.style > ItemType::M2 ||
                               packet.particle_count < 2 || packet.particle_count > 4 ||
                               !std::isfinite(packet.radius) || packet.radius < 0.0F ||
                               (packet.owner != 255 && packet.owner >= 32) ||
                               packet.mounted_soldier < -1 || packet.mounted_soldier >= 32))) {
            return NetworkEventHandlerResult::Failure;
        }
        if (packet.active &&
            (packet.holding_soldier_id < -1 || packet.holding_soldier_id >= 32 ||
             !IsValidBackgroundState(packet.polygon_background,
                                     world_->GetStateManager()->GetConstMap().GetPolygons()) ||
             (packet.polygon_background.map_fingerprint != 0 &&
              packet.polygon_background.map_fingerprint !=
                world_->GetStateManager()->GetConstMap().GetPolygonFingerprint()))) {
            return NetworkEventHandlerResult::Failure;
        }
        if (packet.active && packet.particle_count != (IsItemTypeWeapon(packet.style) ? 2 : 4)) {
            return NetworkEventHandlerResult::Failure;
        }
        for (const auto& particle : packet.particles) {
            if (!std::isfinite(particle.x) || !std::isfinite(particle.y) ||
                !std::isfinite(particle.old_x) || !std::isfinite(particle.old_y)) {
                return NetworkEventHandlerResult::Failure;
            }
        }
        auto& latest = latest_ticks_.at(packet.id);
        if (latest && !IsSerialNumberNewer(packet.server_tick, *latest)) {
            return NetworkEventHandlerResult::Success;
        }
        const auto state = world_->GetStateManager();
        if (!packet.active) {
            latest = packet.server_tick;
            state->RemoveItem(packet.id);
            return NetworkEventHandlerResult::Success;
        }
        auto& item = state->RestoreItem(packet.id,
                                        packet.style,
                                        packet.owner,
                                        { packet.particles.at(0).x, packet.particles.at(0).y });
        if (item.skeleton->GetParticles().size() != packet.particle_count) {
            return NetworkEventHandlerResult::Failure;
        }
        latest = packet.server_tick;
        item.owner = packet.owner;
        item.ammo_count = packet.ammo;
        item.radius = packet.radius;
        item.time_out = packet.timeout;
        item.static_type = packet.is_static;
        item.interest = packet.heat;
        item.mounted_soldier = packet.mounted_soldier;
        item.holding_soldier_id = packet.holding_soldier_id;
        item.polygon_background = packet.polygon_background;
        for (unsigned int i = 0; i < packet.particle_count; ++i) {
            const auto& particle = packet.particles.at(i);
            item.skeleton->SetPos(i + 1, { particle.x, particle.y });
            item.skeleton->SetOldPos(i + 1, { particle.old_x, particle.old_y });
        }
        world_->GetWorldEvents().after_item_updates.Notify(item, state->GetGameTick());
        return NetworkEventHandlerResult::Success;
    }

private:
    std::shared_ptr<IWorld> world_;
    std::array<std::optional<std::uint32_t>, 128> latest_ticks_{};
};
} // namespace Soldank
