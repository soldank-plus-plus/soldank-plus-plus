module;

#include <cstdint>
#include <cstddef>

export module Shared.Core.Physics.Bullets.BulletImpactResolver;

import Extern.Glm;

import Shared.Core.Entities.Bullet;
import Shared.Core.Physics.Bullets.BulletEvents;
import Shared.Core.Physics.Bullets.BulletEffects;
import Shared.Core.Entities.Item;
import Shared.Core.Physics.Bullets.BulletDamage;
import Shared.Core.Physics.Bullets.BulletTypes;
import Shared.Core.Physics.PhysicsEvents;
import Shared.Core.State.StateManager;
import Shared.Core.Types.BulletType;
import Shared.Core.Types.ItemType;

namespace Soldank::BulletImpactResolver
{
namespace
{
constexpr float RICOCHET_REPEAT_DISTANCE = 50.0F;
constexpr float GRENADE_SURFACE_COEFFICIENT = 0.88F;
constexpr std::int16_t ARROW_RESIST_TIMEOUT = 280;
constexpr float THING_PUSH_MULTIPLIER = 9.0F;
constexpr unsigned int THING_COLLISION_COOLDOWN = 60;

bool CanRicochet(const Bullet& bullet, const BulletCollisionResult& collision)
{
    if (!collision.surface_normal ||
        glm::length(collision.position - bullet.hit_spot) <= RICOCHET_REPEAT_DISTANCE) {
        return false;
    }
    const auto velocity = bullet.particle.GetVelocity();
    const auto response = velocity * (25.0F / 35.0F) +
                          *collision.surface_normal * glm::length(velocity) * (10.0F / 35.0F);
    return glm::dot(response, *collision.surface_normal) > 0.0F;
}

void Ricochet(Bullet& bullet, const BulletCollisionResult& collision)
{
    const glm::vec2 normal = *collision.surface_normal;
    const glm::vec2 velocity = bullet.particle.GetVelocity();

    bullet.hit_spot = collision.position;
    ++bullet.ricochet_count;
    bullet.particle.SetVelocity(velocity * (25.0F / 35.0F) +
                                normal * glm::length(velocity) * (10.0F / 35.0F));
    bullet.particle.position = collision.position + normal * 0.01F;
    bullet.particle.old_position = bullet.particle.position;
}

void Bounce(Bullet& bullet, const BulletCollisionResult& collision)
{
    const glm::vec2 normal = *collision.surface_normal;
    const glm::vec2 velocity = bullet.particle.GetVelocity();

    bullet.particle.SetVelocity((velocity - 2.0F * glm::dot(velocity, normal) * normal) *
                                GRENADE_SURFACE_COEFFICIENT);
    bullet.particle.position = collision.position + normal * 0.01F;
    bullet.particle.old_position = bullet.particle.position;
}

void StickArrow(Bullet& bullet, const BulletCollisionResult& collision)
{
    bullet.stuck = true;
    bullet.particle.position = collision.position - bullet.particle.GetVelocity();
    bullet.particle.SetVelocity({});
    bullet.particle.old_position = bullet.particle.position;
    bullet.particle.SetForce(bullet.particle.GetForce() + glm::vec2{ 0.0F, -0.135F });
    if (bullet.timeout > ARROW_RESIST_TIMEOUT) {
        bullet.timeout = ARROW_RESIST_TIMEOUT;
    }
    if (bullet.timeout < 20) {
        bullet.particle.SetForce(bullet.particle.GetForce() + glm::vec2{ 0.0F, 0.135F });
    }
}

void ResolveItemImpact(Bullet& bullet,
                       StateManager& state_manager,
                       const BulletCollisionResult& collision)
{
    if (!state_manager.HasAuthoritativeProjectiles()) {
        return;
    }
    if (!collision.item_id.has_value() || !collision.item_particle_id.has_value()) {
        return;
    }

    const unsigned int game_tick = state_manager.GetGameTick();
    bool has_expired_cooldown = false;
    for (std::size_t cooldown_index = 0; cooldown_index < bullet.item_collision_cooldowns.size();
         ++cooldown_index) {
        const auto& cooldown = bullet.item_collision_cooldowns[cooldown_index];
        if (cooldown.item_id != *collision.item_id) {
            continue;
        }

        if (game_tick < cooldown.cooldown_end_tick) {
            return;
        }

        has_expired_cooldown = true;
        break;
    }

    bool applied_push = false;
    state_manager.TransformItems([&](Item& item) {
        if (item.id != *collision.item_id || !item.skeleton ||
            *collision.item_particle_id > item.skeleton->GetParticles().size()) {
            return;
        }

        const unsigned int particle_id = *collision.item_particle_id;
        const glm::vec2 item_velocity =
          item.skeleton->GetPos(particle_id) - item.skeleton->GetOldPos(particle_id);
        const glm::vec2 push =
          (bullet.particle.GetVelocity() - item_velocity) * bullet.push * THING_PUSH_MULTIPLIER;
        item.skeleton->SetPos(particle_id, item.skeleton->GetPos(particle_id) + push);
        item.static_type = false;
        auto event =
          MakeProjectileEvent(ProjectileEventKind::ThingHitByProjectile, bullet, game_tick);
        event.item_id = item.id;
        event.particle_id = particle_id;
        event.impulse = push;
        state_manager.RecordProjectileEvent(event);
        applied_push = true;
    });

    if (!applied_push) {
        return;
    }

    if (!has_expired_cooldown) {
        bullet.item_collision_cooldowns.push_back(
          { *collision.item_id, game_tick + THING_COLLISION_COOLDOWN });
        return;
    }

    for (std::size_t cooldown_index = 0; cooldown_index < bullet.item_collision_cooldowns.size();
         ++cooldown_index) {
        auto& cooldown = bullet.item_collision_cooldowns[cooldown_index];
        if (cooldown.item_id == *collision.item_id) {
            cooldown.cooldown_end_tick = game_tick + THING_COLLISION_COOLDOWN;
            return;
        }
    }
}
} // namespace
} // namespace Soldank::BulletImpactResolver

export namespace Soldank::BulletImpactResolver
{
void ResolveMapImpact(const PhysicsEvents& physics_events,
                      Bullet& bullet,
                      StateManager& state_manager,
                      const BulletCollisionResult& collision)
{
    switch (bullet.style) {
        case BulletType::Bullet:
        case BulletType::GaugeBullet:
        case BulletType::Fist:
        case BulletType::Blade:
        case BulletType::M2Bullet:
        case BulletType::M79Grenade:
        case BulletType::FlameArrow:
        case BulletType::LAWMissile:
            if (CanRicochet(bullet, collision)) {
                Ricochet(bullet, collision);
            } else {
                bullet.particle.position = collision.position;
                if (bullet.style == BulletType::M79Grenade ||
                    bullet.style == BulletType::FlameArrow ||
                    bullet.style == BulletType::LAWMissile) {
                    BulletDamage::Explode(physics_events, bullet, state_manager);
                } else {
                    bullet.active = false;
                }
            }
            break;
        case BulletType::FragGrenade:
        case BulletType::Flame:
            Bounce(bullet, collision);
            if (bullet.style == BulletType::Flame && bullet.timeout > 16) {
                bullet.timeout = 16;
            }
            break;
        case BulletType::Arrow:
            StickArrow(bullet, collision);
            break;
        case BulletType::ClusterGrenade:
            bullet.particle.position = collision.position;
            BulletEffects::Fragment(state_manager, bullet);
            bullet.active = false;
            break;
        case BulletType::Cluster:
            bullet.particle.position = collision.position;
            BulletDamage::Explode(physics_events, bullet, state_manager);
            break;
        case BulletType::ThrownKnife:
            bullet.particle.position = collision.position - bullet.particle.GetVelocity();
            BulletEffects::CreateKnife(state_manager, bullet, bullet.particle.position);
            bullet.active = false;
            break;
    }

    physics_events.bullet_collides_with_polygon.Notify(bullet, collision.position);
}

void ResolveBlockingImpact(Bullet& bullet, const BulletCollisionResult& collision)
{
    bullet.particle.position = collision.position;
    bullet.active = false;
}

void PublishImpactEffects(Bullet& bullet,
                          StateManager& state_manager,
                          const BulletCollisionResult& collision,
                          glm::vec2 velocity_before,
                          std::int32_t ricochets_before,
                          std::size_t events_before)
{
    auto event = MakeProjectileEvent(
      ProjectileEventKind::ProjectileImpact, bullet, state_manager.GetGameTick());
    event.position = collision.position;
    event.collision = collision;
    event.soldier_id = collision.soldier_id;
    event.item_id = collision.item_id;
    if (collision.body_part_id) {
        event.particle_id = static_cast<unsigned int>(*collision.body_part_id);
    } else {
        event.particle_id = collision.item_particle_id;
    }
    event.distance = collision.distance;
    event.velocity_before = velocity_before;
    state_manager.RecordProjectileEvent(event);
    if (bullet.active && bullet.ricochet_count > ricochets_before) {
        event.kind = ProjectileEventKind::ProjectileRicocheted;
        state_manager.RecordProjectileEvent(event);
    } else if (bullet.active && collision.kind == BulletCollisionKind::MapPolygon &&
               (bullet.style == BulletType::FragGrenade || bullet.style == BulletType::Flame)) {
        event.kind = ProjectileEventKind::ProjectileBounced;
        state_manager.RecordProjectileEvent(event);
    }
    if (bullet.stuck) {
        event.kind = ProjectileEventKind::ProjectileStuck;
        state_manager.RecordProjectileEvent(event);
    }
    bool already_destroyed = false;
    const auto& events = state_manager.GetProjectileEvents();
    for (auto index = events_before; index < events.size(); ++index) {
        if (events.at(index).kind == ProjectileEventKind::ProjectileDestroyed &&
            events.at(index).projectile_id == bullet.id) {
            already_destroyed = true;
        }
    }
    if (!bullet.active && !already_destroyed) {
        event.kind = ProjectileEventKind::ProjectileDestroyed;
        event.reason = ProjectileDestructionReason::Impact;
        state_manager.RecordProjectileEvent(event);
    }
}

void ResolveImpact(const PhysicsEvents& physics_events,
                   Bullet& bullet,
                   StateManager& state_manager,
                   const BulletCollisionResult& collision)
{
    const auto velocity_before = bullet.particle.GetVelocity();
    const auto ricochets_before = bullet.ricochet_count;
    const auto events_before = state_manager.GetProjectileEvents().size();
    switch (collision.kind) {
        case BulletCollisionKind::MapPolygon:
            ResolveMapImpact(physics_events, bullet, state_manager, collision);
            break;
        case BulletCollisionKind::Soldier: {
            const auto outcome =
              BulletDamage::ApplyDirectHit(physics_events, bullet, state_manager, collision);
            if (outcome == BulletDamage::DirectHitOutcome::Destroyed) {
                bullet.active = false;
            } else if (outcome == BulletDamage::DirectHitOutcome::Explode) {
                bullet.particle.position = collision.position;
                BulletDamage::Explode(physics_events, bullet, state_manager);
            }
            break;
        }
        case BulletCollisionKind::MapCollider:
            bullet.particle.position = collision.position;
            if (bullet.style == BulletType::ThrownKnife) {
                BulletEffects::CreateKnife(
                  state_manager, bullet, collision.position - bullet.particle.GetVelocity());
            }
            if (bullet.style == BulletType::ClusterGrenade) {
                BulletEffects::Fragment(state_manager, bullet);
            } else if (bullet.style == BulletType::FragGrenade ||
                       bullet.style == BulletType::M79Grenade ||
                       bullet.style == BulletType::LAWMissile ||
                       bullet.style == BulletType::FlameArrow ||
                       bullet.style == BulletType::Cluster) {
                BulletDamage::Explode(physics_events, bullet, state_manager);
            }
            bullet.active = false;
            break;
        case BulletCollisionKind::Item:
            ResolveItemImpact(bullet, state_manager, collision);
            break;
    }
    PublishImpactEffects(
      bullet, state_manager, collision, velocity_before, ricochets_before, events_before);
}

void ResolveExpiry(const PhysicsEvents& physics_events, Bullet& bullet, StateManager& state)
{
    switch (bullet.style) {
        case BulletType::FragGrenade:
        case BulletType::M79Grenade:
        case BulletType::FlameArrow:
        case BulletType::LAWMissile:
        case BulletType::Cluster:
        case BulletType::M2Bullet:
            BulletDamage::Explode(physics_events, bullet, state);
            return;
        default:
            bullet.active = false;
            auto event = MakeProjectileEvent(
              ProjectileEventKind::ProjectileDestroyed, bullet, state.GetGameTick());
            event.reason = ProjectileDestructionReason::Timeout;
            state.RecordProjectileEvent(event);
            return;
    }
}
} // namespace Soldank::BulletImpactResolver
