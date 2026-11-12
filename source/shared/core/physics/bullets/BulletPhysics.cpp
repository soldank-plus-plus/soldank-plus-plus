module;

#include <algorithm>
#include <array>
#include <cmath>
#include <initializer_list>
#include <optional>

export module Shared.Core.Physics.BulletPhysics;

import Extern.Glm;

import Shared.Core.Entities.Bullet;
import Shared.Core.Entities.Soldier;
import Shared.Core.Physics.Bullets.BulletEvents;
import Shared.Core.Physics.Bullets.BulletCollision;
import Shared.Core.Physics.Bullets.BulletImpactResolver;
import Shared.Core.Physics.Bullets.BulletTypes;
import Shared.Core.Physics.PhysicsEvents;
import Shared.Core.Map.Map;
import Shared.Core.State.StateManager;
import Shared.Core.Types.BulletType;
import Shared.Core.Types.WeaponType;

namespace Soldank
{
namespace
{
void ApplyTimeoutAndDamageFalloff(const PhysicsEvents& physics_events,
                                  Bullet& bullet,
                                  StateManager& state)
{
    bullet.timeout_prev = bullet.timeout;
    --bullet.timeout;

    if (bullet.timeout <= 0) {
        BulletImpactResolver::ResolveExpiry(physics_events, bullet, state);
        return;
    }

    if (bullet.degrade_count >= 2 || bullet.timeout % 6 != 0) {
        return;
    }

    constexpr auto EXCLUDED_WEAPONS =
      std::array{ WeaponType::Barrett, WeaponType::M79, WeaponType::Knife, WeaponType::LAW };
    if (std::ranges::contains(EXCLUDED_WEAPONS, bullet.weapon)) {
        return;
    }

    constexpr auto DEGRADE_DISTANCES_SQUARED = std::array{ 500.0F * 500.0F, 900.0F * 900.0F };
    const glm::vec2 delta = bullet.particle.position - bullet.initial_position;
    const float distance_squared = delta.x * delta.x + delta.y * delta.y;
    if (distance_squared > DEGRADE_DISTANCES_SQUARED.at(bullet.degrade_count)) {
        bullet.hit_multiply_prev = bullet.hit_multiply;
        bullet.hit_multiply *= 0.5F;
        ++bullet.degrade_count;
    }
}

bool IsOutOfBounds(const Bullet& bullet, const Map& map)
{
    const float bound = static_cast<float>(map.GetSectorsCount() * map.GetSectorsSize() - 10);
    return std::max(std::abs(bullet.particle.position.x), std::abs(bullet.particle.position.y)) >
           bound;
}

void IntegrateParticle(Bullet& bullet, StateManager& state_manager)
{
    bullet.velocity_prev = bullet.particle.velocity_;
    if (bullet.stuck && !bullet.attached_soldier && bullet.timeout < 20) {
        bullet.stuck = false;
        bullet.particle.SetForce({});
    }
    if (bullet.stuck) {
        if (bullet.attached_soldier && bullet.attached_part) {
            const auto& soldier = state_manager.GetSoldier(*bullet.attached_soldier);
            if (soldier.active && soldier.skeleton) {
                bullet.particle.position = soldier.skeleton->GetPos(*bullet.attached_part);
            }
        }
    } else {
        bullet.particle.Euler();
    }
}

std::optional<BulletCollisionResult> FindClosestCollision(
  std::initializer_list<std::optional<BulletCollisionResult>> collisions)
{
    std::optional<BulletCollisionResult> closest_collision;
    for (const auto& collision : collisions) {
        if (!collision.has_value() ||
            (closest_collision.has_value() && collision->distance >= closest_collision->distance)) {
            continue;
        }

        closest_collision = collision;
    }

    return closest_collision;
}
} // namespace
} // namespace Soldank

export namespace Soldank::BulletPhysics
{
void UpdateBullet(const PhysicsEvents& physics_events,
                  Bullet& bullet,
                  const Map& map,
                  StateManager& state_manager)
{
    if (!bullet.active) {
        return;
    }
    IntegrateParticle(bullet, state_manager);
    if (!state_manager.HasAuthoritativeProjectiles()) {
        // Online clients integrate presentation; reliable server events own impacts and removal.
        if (bullet.timeout > 0) {
            --bullet.timeout;
        }
        if (bullet.style == BulletType::Flame) {
            bullet.particle.SetForce(bullet.particle.GetForce() + glm::vec2{ 0.0F, -0.15F });
        }
        return;
    }
    if (IsOutOfBounds(bullet, map)) {
        bullet.active = false;
        auto event = MakeProjectileEvent(
          ProjectileEventKind::ProjectileDestroyed, bullet, state_manager.GetGameTick());
        event.reason = ProjectileDestructionReason::OutOfBounds;
        state_manager.RecordProjectileEvent(event);
        return;
    }

    // A penetrating soldier hit leaves the remaining swept segment available.
    // Every continuation excludes another target, so the loop is bounded by the soldier count.
    while (bullet.active && !bullet.stuck) {
        const auto collision = FindClosestCollision({
          BulletCollision::FindMapCollision(bullet, map),
          BulletCollision::FindColliderCollision(bullet, map),
          BulletCollision::FindSoldierCollision(bullet, state_manager, -1.0F),
          BulletCollision::FindThingCollision(bullet, state_manager),
        });
        if (!collision.has_value()) {
            break;
        }
        const auto previous_hits = bullet.penetrated_soldiers.size();
        BulletImpactResolver::ResolveImpact(physics_events, bullet, state_manager, *collision);
        if (!bullet.active || bullet.penetrated_soldiers.size() == previous_hits) {
            break;
        }
        bullet.particle.old_position = collision->position;
    }

    if (bullet.active) {
        ApplyTimeoutAndDamageFalloff(physics_events, bullet, state_manager);
    }

    if (bullet.style == BulletType::Flame) {
        bullet.particle.SetForce(bullet.particle.GetForce() + glm::vec2{ 0.0F, -0.15F });
    }
}
} // namespace Soldank::BulletPhysics
