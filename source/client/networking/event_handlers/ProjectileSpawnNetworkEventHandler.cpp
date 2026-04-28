module;

#include <cstdint>
#include <memory>

export module Networking.ProjectileSpawnNetworkEventHandler;

import Shared.Core.IWorld;

import Shared.Networking.NetworkEventDispatcher;
import Shared.Networking.NetworkPackets;
import Shared.Networking.NetworkEvent;

import Shared.Core.Entities.Bullet;
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
        // unsigned int projectile_id = projectile_spawn_packet.projectile_id;
        BulletType style = projectile_spawn_packet.style;
        WeaponType weapon = projectile_spawn_packet.weapon;
        float position_x = projectile_spawn_packet.position_x;
        float position_y = projectile_spawn_packet.position_y;
        float velocity_x = projectile_spawn_packet.velocity_x;
        float velocity_y = projectile_spawn_packet.velocity_y;
        std::int16_t timeout = projectile_spawn_packet.timeout;
        float hit_multiply = projectile_spawn_packet.hit_multiply;
        TeamType team = projectile_spawn_packet.team;
        std::uint8_t owner_id = projectile_spawn_packet.owner_id;

        BulletParams bullet_params{ style,
                                    weapon,
                                    { position_x, position_y },
                                    { velocity_x, velocity_y },
                                    timeout,
                                    hit_multiply,
                                    team,
                                    owner_id };
        world_->GetStateManager()->CreateProjectile(bullet_params);
        return NetworkEventHandlerResult::Success;
    }

    std::shared_ptr<IWorld> world_;
};
} // namespace Soldank
