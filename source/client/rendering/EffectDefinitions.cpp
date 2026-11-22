module;

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <vector>

export module EffectDefinitions;

import Extern.Glm;
import Shared.Core.Entities.Bullet;
import Shared.Core.Types.WeaponType;
import Shared.Core.Types.BulletType;

export namespace Soldank
{
enum class WeaponVisualEffectKind
{
    Casing,
    Blood,
    Explosion,
    ExplosionSmoke,
    ExplosionAfterSmoke,
    ClusterSmoke,
    Smoke,
    Spark,
    Flame,
    MuzzleSmoke,
    Headgear,
    Cigar,
    Ember,
    BlackSmoke,
    Exhaust,
    JetFire,
    SmallBlood,
    Splat,
    Puff,
    Spawn,
    Capture,
    Pin,
    Magazine,
    Debris,
    Cloth,
    Rain,
    Sand,
    Snow,
    Spit,
    Match,
    Urine,
    CigarSmoke,
};

struct WeaponVisualEffect
{
    WeaponVisualEffectKind kind{};
    WeaponType weapon{};
    glm::vec2 position{};
    glm::vec2 old_position{};
    glm::vec2 velocity{};
    float rotation{};
    float old_rotation{};
    float size{ 1.0F };
    int lifetime{ 30 };
    int initial_lifetime{ 30 };
    std::uint8_t owner{ 255 };
    glm::vec4 tint{ 1.0F };
    std::uint8_t variant{};
    std::uint8_t team{};
    bool carrying_flag{};
    bool has_owner{};
    std::uint8_t collisions{};
    std::uint32_t owner_life{};
};

struct EffectAppearance
{
    glm::vec2 position{};
    glm::vec2 scale{ 1.0F };
    glm::vec4 color{ 1.0F };
    float rotation{};
};

EffectAppearance GetEffectAppearance(const WeaponVisualEffect& effect, float frame_percent)
{
    const float fraction = std::clamp(frame_percent, 0.0F, 1.0F);
    const float life = std::max(0.01F,
                                std::min(static_cast<float>(effect.initial_lifetime),
                                         static_cast<float>(effect.lifetime) + 1.0F - fraction));
    EffectAppearance result{
        .position = effect.old_position + (effect.position - effect.old_position) * fraction,
        .color = effect.tint,
        .rotation = effect.old_rotation + (effect.rotation - effect.old_rotation) * fraction
    };
    float alpha = life + 10.0F;
    switch (effect.kind) {
        case WeaponVisualEffectKind::SmallBlood:
            result.scale = glm::vec2(0.75F);
            result.rotation = life * 10.0F * 0.0174532925F;
            alpha = life * 2.0F + 65.0F;
            break;
        case WeaponVisualEffectKind::Blood:
            result.scale = glm::vec2(life > 10.0F ? 0.33F + 10.0F / life : 1.0F);
            result.rotation = life * 2.0F * 0.0174532925F;
            alpha = life * 2.0F + 85.0F;
            break;
        case WeaponVisualEffectKind::Splat:
            result.scale = glm::vec2(life > 20.0F ? 0.63F + 10.0F / life : 1.0F);
            alpha = life * 2.0F + 55.0F;
            break;
        case WeaponVisualEffectKind::Flame:
        case WeaponVisualEffectKind::BlackSmoke:
            result.scale =
              glm::vec2(life / (effect.kind == WeaponVisualEffectKind::Flame ? 35.0F : 75.0F));
            result.position.y -= 1.0F / result.scale.y;
            alpha =
              effect.kind == WeaponVisualEffectKind::Flame ? life * 2.0F + 185.0F : life * 3.0F;
            break;
        case WeaponVisualEffectKind::Exhaust:
            result.position.y -= 50.0F - life;
            result.scale = glm::vec2(1.5F + 620.0F / life / 50.0F);
            alpha = life * 2.0F;
            break;
        case WeaponVisualEffectKind::JetFire:
            result.rotation = life * 0.0174532925F;
            alpha = life * 5.0F;
            break;
        case WeaponVisualEffectKind::MuzzleSmoke:
            alpha = life * 13.0F;
            break;
        case WeaponVisualEffectKind::Ember:
            alpha = life;
            break;
        case WeaponVisualEffectKind::Debris:
            result.scale = glm::vec2(effect.variant >= 4 ? 0.7F : 1.0F);
            alpha = effect.variant >= 4 ? life * 2.0F + 15.0F : life + 10.0F;
            break;
        case WeaponVisualEffectKind::Cloth:
            alpha = life * 2.0F + 15.0F;
            break;
        case WeaponVisualEffectKind::Spark:
            alpha = life * 3.0F + (effect.variant == 1 ? 154.0F : 10.0F);
            break;
        case WeaponVisualEffectKind::Spit:
            alpha = life + 10.0F;
            break;
        case WeaponVisualEffectKind::Urine:
            alpha = life * 2.0F + 10.0F;
            break;
        case WeaponVisualEffectKind::Match:
        case WeaponVisualEffectKind::Cigar:
            result.rotation = life * 4.0F * 0.0174532925F;
            alpha = 255.0F;
            break;
        case WeaponVisualEffectKind::Puff:
            alpha = life + 5.0F;
            break;
        case WeaponVisualEffectKind::Spawn:
        case WeaponVisualEffectKind::Capture:
            result.position -=
              glm::vec2(effect.kind == WeaponVisualEffectKind::Spawn ? 20.0F : 10.0F);
            result.scale = glm::vec2(effect.kind == WeaponVisualEffectKind::Spawn ? 1.0F : 0.5F);
            result.rotation = life * 0.0174532925F;
            alpha = life * (effect.kind == WeaponVisualEffectKind::Spawn ? 6.0F : 14.0F);
            break;
        case WeaponVisualEffectKind::Rain:
        case WeaponVisualEffectKind::Sand:
        case WeaponVisualEffectKind::Snow:
            alpha = 105.0F;
            break;
        case WeaponVisualEffectKind::Casing:
        case WeaponVisualEffectKind::Magazine:
        case WeaponVisualEffectKind::Pin:
        case WeaponVisualEffectKind::Headgear:
            alpha = 255.0F;
            break;
        default:
            break;
    }
    result.color.a *= std::clamp(alpha / 255.0F, 0.0F, 1.0F);
    return result;
};

enum class ExplosionSpriteKind
{
    Blast,
    Smoke,
    BigSmoke,
    BigSmoke2,
};

struct ExplosionSprite
{
    ExplosionSpriteKind kind{};
    int frame{};
    glm::vec2 position{};
    glm::vec2 scale{ 1.0F };
    glm::vec4 color{ 1.0F };
};

// Sparks.pas uses Pascal Round (ties to even), not truncation or round-away-from-zero.
int RoundExplosionFrame(float value)
{
    const auto lower = static_cast<int>(std::floor(value));
    const float fraction = value - static_cast<float>(lower);
    return lower + static_cast<int>(fraction > 0.5F || (fraction == 0.5F && lower % 2 != 0));
}

// Pure render descriptions keep the reference frame, anchor and compositing rules testable
// without an OpenGL context. Scales are relative to the asset's native game size.
std::vector<ExplosionSprite> GetExplosionSprites(const WeaponVisualEffect& effect,
                                                 float frame_percent)
{
    const float life =
      std::min(static_cast<float>(effect.initial_lifetime),
               static_cast<float>(effect.lifetime) + 1.0F - std::clamp(frame_percent, 0.0F, 1.0F));
    const auto color = [](float shade, float alpha) {
        return glm::vec4(glm::vec3(shade / 255.0F), std::clamp(alpha / 255.0F, 0.0F, 1.0F));
    };
    if (effect.kind == WeaponVisualEffectKind::Explosion) {
        const bool cluster = effect.weapon == WeaponType::Cluster;
        const bool frag = effect.weapon == WeaponType::FragGrenade;
        const int frame =
          std::clamp(15 - RoundExplosionFrame(life / (cluster ? 3.0F : 4.0F)), 0, 15);
        const glm::vec2 scale(cluster ? 0.5F : (frag ? 1.0F : 0.75F));
        const auto position = effect.position + (cluster ? glm::vec2(-15.0F, -37.0F)
                                                         : (frag ? glm::vec2(-25.0F, -50.0F)
                                                                 : glm::vec2(-19.0F, -38.0F)));
        std::vector<ExplosionSprite> sprites;
        if (!cluster && frame > 0) {
            sprites.push_back({ ExplosionSpriteKind::Blast,
                                frame - 1,
                                position,
                                scale,
                                color(frag ? 170.0F : 173.0F, 100.0F) });
        }
        sprites.push_back({ ExplosionSpriteKind::Blast,
                            frame,
                            position,
                            scale,
                            color(255.0F, cluster ? 255.0F - 2.0F * life : 175.0F + life) });
        return sprites;
    }
    if (effect.kind == WeaponVisualEffectKind::ExplosionSmoke && life <= 40.0F) {
        const int frame = std::clamp(10 - RoundExplosionFrame(life / 4.0F), 0, 10);
        const auto position = effect.position + glm::vec2(-26.0F, -48.0F);
        std::vector<ExplosionSprite> sprites;
        if (frame > 0) {
            sprites.push_back({ ExplosionSpriteKind::Smoke,
                                frame - 1,
                                position,
                                glm::vec2(1.0F),
                                color(204.0F, 2.0F * life + 10.0F) });
        }
        sprites.push_back({ ExplosionSpriteKind::Smoke,
                            frame,
                            position,
                            glm::vec2(1.0F),
                            color(221.0F, 3.0F * life + 10.0F) });
        return sprites;
    }
    if (effect.kind == WeaponVisualEffectKind::ExplosionAfterSmoke) {
        const glm::vec2 scale(0.5F + 16.0F / (life + 50.0F));
        const auto position = effect.position + glm::vec2(-14.0F * scale.x, -30.0F);
        return { { ExplosionSpriteKind::BigSmoke,
                   0,
                   position,
                   scale,
                   color(255.0F, std::floor(life / 3.3F)) },
                 { ExplosionSpriteKind::BigSmoke2,
                   0,
                   position,
                   scale,
                   color(life > 30.0F ? 102.0F : 191.0F,
                         std::floor(life > 30.0F ? (255.0F - life) / 9.0F : life)) } };
    }
    if (effect.kind == WeaponVisualEffectKind::ClusterSmoke) {
        const glm::vec2 scale(0.5F * (0.6F + (75.0F / life) / 96.0F),
                              0.5F * (0.6F + (75.0F / life) / 90.0F));
        return { { ExplosionSpriteKind::BigSmoke,
                   0,
                   effect.position + glm::vec2(-22.0F * scale.x, -48.0F + life / 1.5F),
                   scale,
                   color(255.0F, std::floor(2.5F * life)) } };
    }
    return {};
}

struct ProjectileTrail
{
    glm::vec2 position{};
    glm::vec2 scale{ 1.0F };
    glm::vec4 color{ 1.0F };
    float rotation{};
};

std::optional<ProjectileTrail> GetProjectileTrail(const Bullet& bullet,
                                                  unsigned int tick,
                                                  float fraction)
{
    if (!bullet.active || bullet.stuck || tick < bullet.spawn_tick) {
        return std::nullopt;
    }
    const float age =
      static_cast<float>(tick - bullet.spawn_tick) + std::clamp(fraction, 0.0F, 1.0F);
    const auto velocity = bullet.particle.GetVelocity();
    const float speed = glm::length(velocity);
    if (speed < 0.001F) {
        return std::nullopt;
    }
    ProjectileTrail trail{ .position =
                             bullet.particle.old_position +
                             (bullet.particle.position - bullet.particle.old_position) * fraction,
                           .rotation = -std::atan2(-velocity.y, -velocity.x) };
    float divisor = 3.5F;
    float threshold = 7.0F;
    float alpha = 55.0F;
    switch (bullet.style) {
        case BulletType::Bullet:
            if (!bullet.penetrated_soldiers.empty()) {
                divisor = 4.0F;
                trail.color = { 1.0F, 221.0F / 255.0F, 221.0F / 255.0F, 1.0F };
            }
            break;
        case BulletType::FragGrenade:
            threshold = 3.0F;
            divisor = 3.0F;
            trail.position += glm::vec2(velocity.y > 0.0F ? -1.0F : 1.0F,
                                        -3.0F + (velocity.x > 0.0F ? 1.0F : -1.0F));
            trail.color = { 100.0F / 255.0F, 1.0F, 100.0F / 255.0F, 1.0F };
            alpha = 82.5F;
            break;
        case BulletType::GaugeBullet:
            threshold = 3.0F;
            divisor = 9.0F;
            alpha = 22.0F;
            break;
        case BulletType::M79Grenade:
            threshold = 4.0F;
            divisor = 4.0F;
            trail.scale.y = 1.3F;
            trail.position +=
              glm::vec2(velocity.y > 0.0F ? -1.0F : 1.0F, velocity.x > 0.0F ? 1.0F : -1.0F);
            trail.color = { 1.0F, 1.0F, 85.0F / 255.0F, 1.0F };
            alpha = 110.0F;
            break;
        case BulletType::Arrow:
            if (bullet.timeout <= 280) {
                return std::nullopt;
            }
            threshold = 2.0F;
            divisor = 3.0F;
            alpha = 15.0F;
            break;
        case BulletType::LAWMissile:
            divisor = 3.0F;
            alpha = 22.0F;
            break;
        case BulletType::M2Bullet:
            threshold = 13.0F;
            divisor = 3.0F;
            alpha = 22.0F;
            break;
        default:
            // Cluster grenades, fragments, flaming arrows and knives have no trail pass
            // in the reference renderer. Their smoke/flame emissions are independent.
            return std::nullopt;
    }
    if (age <= threshold) {
        return std::nullopt;
    }
    trail.scale.x = speed / divisor;
    trail.color.a = alpha / 255.0F;
    return trail;
}

} // namespace Soldank
