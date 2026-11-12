module;

#include <cstdint>
#include <memory>

export module Networking.ProjectileSpawnNetworkEventHandler;

import Shared.Core.IWorld;
import Shared.Networking.ProtocolConversions;

import Shared.Networking.NetworkEventDispatcher;
import Shared.Networking.NetworkPackets;
import Shared.Networking.NetworkEvent;

import Shared.Core.Entities.Bullet;
import Shared.Core.Entities.WeaponParametersFactory;
import Shared.Core.Types.BulletType;
import Shared.Core.Types.TeamType;
import Shared.Core.Types.WeaponType;

export namespace Soldank
{
class ProjectileSpawnNetworkEventHandler : public NetworkEventHandlerBase<ProjectileSpawnPacket>
{
public:
    ProjectileSpawnNetworkEventHandler(const std::shared_ptr<IWorld>& world)
        : world_(world)
    {
    }

private:
    NetworkEvent GetTargetNetworkEvent() const override { return NetworkEvent::ProjectileSpawn; }

    NetworkEventHandlerResult HandleNetworkMessageImpl(
      unsigned int /*sender_connection_id*/,
      ProjectileSpawnPacket projectile_spawn_packet) override
    {
        const auto event = ProtocolConversions::ToProjectileSpawnedEvent(projectile_spawn_packet);
        world_->GetStateManager()->CreateProjectile(event.bullet_params);
        return NetworkEventHandlerResult::Success;
    }

    std::shared_ptr<IWorld> world_;
};
} // namespace Soldank
