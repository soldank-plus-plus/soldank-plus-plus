module;

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

export module Shared.Core.Physics.Bullets.BulletDamage;

import Extern.Glm;
import Shared.Core.Entities.Bullet;
import Shared.Core.Entities.Item;
import Shared.Core.Entities.Soldier;
import Shared.Core.Entities.WeaponParameters;
import Shared.Core.Entities.WeaponParametersFactory;
import Shared.Core.Physics.Bullets.BulletTypes;
import Shared.Core.Physics.Bullets.BulletEvents;
import Shared.Core.Physics.Bullets.BulletEffects;
import Shared.Core.Physics.Bullets.BulletLagCompensation;
import Shared.Core.Physics.PhysicsEvents;
import Shared.Core.State.StateManager;
import Shared.Core.Types.BulletType;
import Shared.Core.Types.WeaponType;
import Shared.Core.Types.TeamType;

export namespace Soldank::BulletDamage
{
enum class DirectHitOutcome
{
    Destroyed,
    Penetrated,
    Retained,
    Explode,
};

bool AllowsTargetCollision(const Soldier& target, const Bullet& bullet, bool explosion)
{
    if (!target.active || target.IsSpectator()) {
        return false;
    }
    const bool self = target.id == bullet.owner_id;
    const bool same_team = bullet.team != TeamType::None && bullet.team == target.team;
    const auto parameters = WeaponParametersFactory::GetParameters(bullet.weapon, false);
    return AllowsWeaponCollision(parameters.no_collision, self, same_team, explosion);
}

bool CanDamage(const StateManager& state,
               const Soldier& target,
               const Bullet& bullet,
               bool explosion)
{
    const bool self = target.id == bullet.owner_id;
    const bool same_team = bullet.team != TeamType::None && bullet.team == target.team;
    return target.GetCeaseFireCounter() < 0 && AllowsTargetCollision(target, bullet, explosion) &&
           (state.IsFriendlyFireEnabled() || self || !same_team);
}

float HitboxModifier(WeaponType weapon, unsigned int part)
{
    const auto parameters = WeaponParametersFactory::GetParameters(weapon, false);
    return part <= 4 ? parameters.modifier_legs
                     : (part <= 11 ? parameters.modifier_chest : parameters.modifier_head);
}

void ApplyDamage(const PhysicsEvents& physics_events,
                 StateManager& state,
                 Soldier& target,
                 const Bullet& bullet,
                 float damage,
                 unsigned int part,
                 bool explosion,
                 float distance = 0.0F,
                 const BulletCollisionResult* collision = nullptr)
{
    if (!state.HasAuthoritativeProjectiles() || !CanDamage(state, target, bullet, explosion)) {
        return;
    }
    target.health -= damage;
    if (!target.dead_meat && target.health <= 0.0F) {
        target.dead_meat = true;
        target.ticks_to_respawn = 180;
    }
    auto event = MakeProjectileEvent(explosion ? ProjectileEventKind::SoldierHitByExplosion
                                               : ProjectileEventKind::SoldierHitByProjectile,
                                     bullet,
                                     state.GetGameTick());
    event.soldier_id = target.id;
    event.particle_id = part;
    if (collision != nullptr) {
        event.position = collision->position;
        event.collision = *collision;
        event.collision->surface_normal =
          (collision->position - target.skeleton->GetPos(part)) * glm::vec2{ 1.3F, -1.3F };
    }
    event.damage = damage;
    event.distance = distance;
    state.RecordProjectileEvent(event);
    // Legacy notification is retained for presentation compatibility. Damage is already committed.
    physics_events.soldier_hit_by_bullet.Notify(target, damage);
}

std::optional<float> GetPenetrationVelocityMultiplier(bool target_was_dead,
                                                      bool target_is_dead,
                                                      float projectile_speed,
                                                      float weapon_speed)
{
    if (target_was_dead) {
        return 0.9F;
    }
    if (target_is_dead || projectile_speed > 23.0F) {
        return 0.75F;
    }
    if (projectile_speed > 5.0F && weapon_speed > 0.0F && projectile_speed / weapon_speed >= 0.9F) {
        return 0.66F;
    }
    return std::nullopt;
}

float ExplosionRadius(BulletType style)
{
    if (style == BulletType::FragGrenade) {
        return 85.0F;
    }
    if (style == BulletType::Cluster || style == BulletType::M2Bullet) {
        return 35.0F;
    }
    return 64.0F;
}

struct ExplosionContext
{
    const PhysicsEvents& events;
    const Bullet& bullet;
    StateManager& state;
    float radius;
    bool fragment;
    WeaponType weapon;
    float damage;
};

void AffectRagdoll(const ExplosionContext& context, Soldier& target)
{
    float nearest = context.radius;
    const auto parts = std::min<std::size_t>(16, target.skeleton->GetParticles().size());
    for (unsigned int part = 1; part <= parts; ++part) {
        const auto delta = target.skeleton->GetPos(part) - context.bullet.particle.position;
        const float distance = glm::length(delta);
        if (distance >= context.radius) {
            continue;
        }
        nearest = std::min(nearest, distance);
        const auto impulse = delta * (4.5F / (distance + 1.0F));
        target.skeleton->SetOldPos(part, target.skeleton->GetOldPos(part) - impulse);
        auto event = MakeProjectileEvent(
          ProjectileEventKind::ExplosionAffectedThing, context.bullet, context.state.GetGameTick());
        event.soldier_id = target.id;
        event.particle_id = part;
        event.impulse = impulse;
        context.state.RecordProjectileEvent(event);
    }
    if (nearest < context.radius) {
        const float distance =
          context.weapon == WeaponType::M79 ? std::max(20.0F, nearest) : nearest;
        ApplyDamage(context.events,
                    context.state,
                    target,
                    context.bullet,
                    context.damage * (context.fragment ? 0.5F : 1.0F) / (distance + 1.0F),
                    1,
                    true,
                    distance);
    }
}

struct ExplosionHitbox
{
    unsigned int part{ 12 };
    float distance{ std::numeric_limits<float>::max() };
    glm::vec2 delta{};
};

ExplosionHitbox FindExplosionHitbox(const ExplosionContext& context, const Soldier& target)
{
    constexpr std::array PARTS{ 12U, 11U, 10U, 6U, 5U, 4U, 3U };
    ExplosionHitbox closest;
    const auto origin = BulletLagCompensation::GetCollisionOrigin(
      target, context.bullet, context.state.GetGameTick());
    for (const auto part : PARTS) {
        const auto delta = origin + target.skeleton->GetPos(part) - target.particle.position -
                           context.bullet.particle.position;
        const float distance = glm::length(delta);
        if (distance < closest.distance) {
            closest = { part, distance, delta };
        }
    }
    return closest;
}

void AffectLivingSoldier(const ExplosionContext& context, Soldier& target)
{
    const auto closest = FindExplosionHitbox(context, target);
    if (closest.distance >= context.radius) {
        return;
    }
    glm::vec2 impulse = closest.delta * (3.75F / (closest.distance + 1.0F));
    if (!context.fragment) {
        impulse.y *= 2.0F;
    }
    BulletLagCompensation::SchedulePush(context.state, target, context.bullet, impulse);
    const float damage = context.damage * HitboxModifier(context.weapon, closest.part) *
                         (context.fragment ? 0.5F : 1.0F) / (closest.distance + 1.0F);
    ApplyDamage(context.events,
                context.state,
                target,
                context.bullet,
                damage,
                closest.part,
                true,
                closest.distance);
}

void AffectExplosionItem(const ExplosionContext& context, Item& item)
{
    if (!context.state.HasAuthoritativeProjectiles() || !item.collide_with_bullets ||
        !item.skeleton) {
        return;
    }
    const auto parts = std::min<std::size_t>(4, item.skeleton->GetParticles().size());
    for (unsigned int part = 1; part <= parts; ++part) {
        const auto delta = item.skeleton->GetPos(part) - context.bullet.particle.position;
        const float distance = glm::length(delta);
        if (distance >= context.radius) {
            continue;
        }
        const auto impulse = delta * (0.5F * 3.75F / (distance + 1.0F));
        item.skeleton->SetOldPos(part, item.skeleton->GetOldPos(part) - impulse);
        item.static_type = false;
        auto event = MakeProjectileEvent(
          ProjectileEventKind::ExplosionAffectedThing, context.bullet, context.state.GetGameTick());
        event.item_id = item.id;
        event.particle_id = part;
        event.impulse = impulse;
        context.state.RecordProjectileEvent(event);
    }
}

void QueueChainExplosions(StateManager& state, const Bullet& bullet, std::vector<Bullet*>& pending)
{
    state.TransformBullets([&](Bullet& candidate) {
        const bool explosive = candidate.style == BulletType::FragGrenade ||
                               candidate.style == BulletType::M79Grenade ||
                               candidate.style == BulletType::LAWMissile;
        if (&candidate == &bullet || !explosive ||
            glm::length(candidate.particle.position - bullet.particle.position) >= 50.0F) {
            return;
        }
        candidate.active = false;
        pending.push_back(&candidate);
        auto event = MakeProjectileEvent(
          ProjectileEventKind::ProjectileChainDetonated, bullet, state.GetGameTick());
        event.target_projectile_id = candidate.id;
        state.RecordProjectileEvent(event);
    });
}

// An iterative work list avoids recursive chain explosions and marks every round before enqueueing.
void Explode(const PhysicsEvents& physics_events, Bullet& initial, StateManager& state)
{
    std::vector<Bullet*> pending{ &initial };
    initial.active = false;
    for (std::size_t index = 0; index < pending.size(); ++index) {
        Bullet& bullet = *pending.at(index);
        const bool fragment =
          bullet.style == BulletType::Cluster || bullet.style == BulletType::M2Bullet;
        const auto weapon = bullet.style == BulletType::FragGrenade || fragment
                              ? WeaponType::FragGrenade
                              : WeaponType::M79;
        const ExplosionContext context{
            physics_events,
            bullet,
            state,
            ExplosionRadius(bullet.style),
            fragment,
            weapon,
            WeaponParametersFactory::GetParameters(weapon, false).hit_multiply
        };
        state.TransformSoldiers([&](Soldier& target) {
            if (!state.HasAuthoritativeProjectiles() || !target.skeleton ||
                !AllowsTargetCollision(target, bullet, true)) {
                return;
            }
            if (target.dead_meat) {
                AffectRagdoll(context, target);
            } else {
                AffectLivingSoldier(context, target);
            }
        });
        state.TransformItems([&](Item& item) { AffectExplosionItem(context, item); });
        if (!fragment) {
            QueueChainExplosions(state, bullet, pending);
        }
        auto event =
          MakeProjectileEvent(ProjectileEventKind::ProjectileExploded, bullet, state.GetGameTick());
        event.radius = context.radius;
        state.RecordProjectileEvent(event);
        event.kind = ProjectileEventKind::ProjectileDestroyed;
        event.reason = ProjectileDestructionReason::Explosion;
        state.RecordProjectileEvent(event);
    }
}

DirectHitOutcome ApplyDirectHit(const PhysicsEvents& physics_events,
                                Bullet& bullet,
                                StateManager& state,
                                const BulletCollisionResult& collision)
{
    auto& target = state.GetSoldier(*collision.soldier_id);
    const auto part = static_cast<unsigned int>(*collision.body_part_id);
    const auto velocity = bullet.particle.GetVelocity();
    const float speed = glm::length(velocity);
    const bool was_dead = target.dead_meat;
    if (!was_dead && bullet.style != BulletType::FragGrenade && bullet.style != BulletType::Flame &&
        bullet.style != BulletType::Arrow) {
        BulletLagCompensation::SchedulePush(state, target, bullet, velocity * bullet.push);
    }
    switch (bullet.style) {
        case BulletType::Bullet:
        case BulletType::GaugeBullet:
        case BulletType::Fist:
        case BulletType::Blade:
        case BulletType::M2Bullet: {
            ApplyDamage(physics_events,
                        state,
                        target,
                        bullet,
                        speed * bullet.hit_multiply * HitboxModifier(bullet.weapon, part),
                        part,
                        false,
                        collision.distance,
                        &collision);
            if (bullet.style == BulletType::Fist &&
                (bullet.team == TeamType::None || bullet.team != target.team)) {
                BulletEffects::DropWeapon(state, target, bullet);
            }
            const auto parameters = WeaponParametersFactory::GetParameters(bullet.weapon, false);
            const auto retention =
              GetPenetrationVelocityMultiplier(was_dead, target.dead_meat, speed, parameters.speed);
            if (retention) {
                bullet.penetrated_soldiers.push_back(target.id);
                bullet.particle.SetVelocity(velocity * *retention);
                auto event = MakeProjectileEvent(
                  ProjectileEventKind::ProjectilePenetratedSoldier, bullet, state.GetGameTick());
                event.collision = collision;
                event.soldier_id = target.id;
                event.particle_id = part;
                event.position = collision.position;
                event.distance = collision.distance;
                event.velocity_before = velocity;
                state.RecordProjectileEvent(event);
                return DirectHitOutcome::Penetrated;
            }
            return DirectHitOutcome::Destroyed;
        }
        case BulletType::Arrow:
            ApplyDamage(physics_events,
                        state,
                        target,
                        bullet,
                        speed * bullet.hit_multiply * HitboxModifier(bullet.weapon, part),
                        part,
                        false,
                        collision.distance,
                        &collision);
            bullet.stuck = true;
            bullet.attached_soldier = target.id;
            bullet.attached_part = part;
            bullet.timeout = std::min<std::int16_t>(bullet.timeout, 280);
            bullet.particle.position = collision.position;
            bullet.particle.SetVelocity({});
            return DirectHitOutcome::Retained;
        case BulletType::ThrownKnife:
            ApplyDamage(physics_events,
                        state,
                        target,
                        bullet,
                        speed * bullet.hit_multiply * 0.01F,
                        part,
                        false,
                        collision.distance,
                        &collision);
            if (was_dead) {
                bullet.penetrated_soldiers.push_back(target.id);
                return DirectHitOutcome::Penetrated;
            }
            BulletEffects::CreateKnife(state, bullet, collision.position);
            return DirectHitOutcome::Destroyed;
        case BulletType::Flame: {
            const bool should_damage = bullet.timeout < 3 && bullet.ricochet_count < 2;
            BulletEffects::PropagateFlame(state, bullet, target, part);
            if (should_damage) {
                ApplyDamage(physics_events,
                            state,
                            target,
                            bullet,
                            bullet.hit_multiply,
                            part,
                            false,
                            collision.distance,
                            &collision);
            }
            return DirectHitOutcome::Retained;
        }
        case BulletType::ClusterGrenade:
            return DirectHitOutcome::Retained;
        case BulletType::FragGrenade:
            return was_dead ? DirectHitOutcome::Retained : DirectHitOutcome::Explode;
        case BulletType::M79Grenade:
        case BulletType::FlameArrow:
        case BulletType::LAWMissile:
            if (was_dead) {
                return DirectHitOutcome::Retained;
            }
            ApplyDamage(physics_events,
                        state,
                        target,
                        bullet,
                        speed * bullet.hit_multiply,
                        part,
                        false,
                        collision.distance,
                        &collision);
            return DirectHitOutcome::Explode;
        case BulletType::Cluster:
            return DirectHitOutcome::Explode;
    }
    return DirectHitOutcome::Destroyed;
}
} // namespace Soldank::BulletDamage
