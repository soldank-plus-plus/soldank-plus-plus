module;

#include <cstdint>
#include <optional>
#include <vector>

export module Shared.Core.Physics.Bullets.BulletEvents;

import Extern.Glm;
import Shared.Core.Entities.Bullet;
import Shared.Core.Types.BulletType;
import Shared.Core.Types.WeaponType;
import Shared.Core.Types.ItemType;
import Shared.Core.Physics.Bullets.BulletTypes;

export namespace Soldank
{
enum class ProjectileEventKind
{
    ProjectileSpawned,
    ProjectileImpact,
    ProjectileRicocheted,
    ProjectileBounced,
    ProjectilePenetratedSoldier,
    ProjectileStuck,
    SoldierHitByProjectile,
    SoldierProjectilePushScheduled,
    ThingHitByProjectile,
    ProjectileExploded,
    SoldierHitByExplosion,
    ExplosionAffectedThing,
    ProjectileChainDetonated,
    ProjectileFragmented,
    WorldItemSpawnedFromProjectile,
    ProjectileDestroyed,
};

enum class ProjectileDestructionReason
{
    None,
    Timeout,
    Impact,
    Explosion,
    OutOfBounds,
};

// Value snapshots only. No event exposes mutable simulation entities.
struct ProjectilePhysicsEvent
{
    ProjectileEventKind kind{};
    std::uint32_t projectile_id{};
    BulletType style{};
    WeaponType weapon{};
    std::uint8_t owner_id{};
    std::uint32_t seed{};
    unsigned int tick{};
    glm::vec2 position{};
    glm::vec2 velocity_before{};
    glm::vec2 velocity_after{};
    glm::vec2 impulse{};
    float damage{};
    float distance{};
    float radius{};
    unsigned int apply_tick{};
    std::optional<BulletCollisionResult> collision{};
    std::optional<std::uint8_t> soldier_id{};
    std::optional<std::uint8_t> item_id{};
    std::optional<unsigned int> particle_id{};
    ItemType item_style{};
    std::optional<std::uint32_t> target_projectile_id{};
    ProjectileDestructionReason reason{};
    std::vector<BulletParams> children{};
};

ProjectilePhysicsEvent MakeProjectileEvent(ProjectileEventKind kind,
                                           const Bullet& bullet,
                                           unsigned int tick)
{
    return { .kind = kind,
             .projectile_id = bullet.id,
             .style = bullet.style,
             .weapon = bullet.weapon,
             .owner_id = bullet.owner_id,
             .seed = bullet.seed,
             .tick = tick,
             .position = bullet.particle.position,
             .velocity_before = bullet.particle.GetVelocity(),
             .velocity_after = bullet.particle.GetVelocity() };
}
} // namespace Soldank
