module;

#include <algorithm>

export module Shared.Core.Physics.Bullets.BulletLagCompensation;

import Extern.Glm;
import Shared.Core.Entities.Bullet;
import Shared.Core.Entities.Soldier;
import Shared.Core.Types.BulletType;
import Shared.Core.State.StateManager;
import Shared.Core.Physics.Bullets.BulletEvents;

export namespace Soldank::BulletLagCompensation
{
glm::vec2 GetCollisionOrigin(const Soldier& soldier, const Bullet& bullet, unsigned int tick)
{
    if (bullet.style == BulletType::Flame && bullet.timeout > bullet.timeout_real - 2) {
        return soldier.particle.position;
    }
    const auto age = std::min(124U, static_cast<unsigned int>(bullet.owner_ping_ticks));
    if (age == 0) {
        return soldier.particle.position;
    }
    return soldier.GetHistoricalPosition(tick > age ? tick - age : 0);
}

void SchedulePush(StateManager& state, Soldier& soldier, const Bullet& bullet, glm::vec2 impulse)
{
    if (!state.HasAuthoritativeProjectiles()) {
        return;
    }
    soldier.ScheduleProjectilePush(state.GetGameTick(), bullet.owner_ping_ticks, impulse);
    auto event = MakeProjectileEvent(
      ProjectileEventKind::SoldierProjectilePushScheduled, bullet, state.GetGameTick());
    event.soldier_id = soldier.id;
    event.impulse = impulse;
    event.apply_tick = soldier.projectile_pushes.back().tick;
    state.RecordProjectileEvent(event);
}
} // namespace Soldank::BulletLagCompensation
