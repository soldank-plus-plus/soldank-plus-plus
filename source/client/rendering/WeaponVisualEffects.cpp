module;

#include <algorithm>
#include <array>
#include <cstdint>
#include <cmath>
#include <deque>
#include <optional>
#include <span>
#include <vector>

export module WeaponVisualEffects;

import Extern.Glm;
import Shared.Core.Entities.Soldier;
import Shared.Core.Entities.Weapon;
import Shared.Core.State.StateManager;
import Shared.Core.Types.WeaponType;
import Shared.Core.Types.BulletType;
import Shared.Core.Types.ItemType;
import Shared.Core.Physics.Bullets.BulletEvents;
import Shared.Core.Utility.SerialNumber;

export namespace Soldank
{
enum class WeaponVisualEffectKind
{
    Casing,
    Explosion,
    ExplosionSmoke,
    ExplosionAfterSmoke,
    ClusterSmoke,
    Smoke,
    Spark,
    Flame,
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

// Presentation-only particles. Their RNG and collisions never modify simulation state.
class WeaponVisualEffects
{
public:
    const std::vector<WeaponVisualEffect>& GetEffects() const { return effects_; }

    void Update(const StateManager& state, std::span<const ProjectilePhysicsEvent> events)
    {
        if (state.IsGamePaused()) {
            return;
        }
        const auto tick = state.GetGameTick();
        if (!last_tick_ || tick != *last_tick_) {
            const unsigned int steps = last_tick_ ? std::min(5U, tick - *last_tick_) : 0;
            for (unsigned int step = 0; step < steps; ++step) {
                Advance(state);
            }
            std::array<bool, 32> seen{};
            state.ForEachSoldier([&](const Soldier& soldier) {
                if (soldier.id < seen.size()) {
                    seen.at(soldier.id) = true;
                }
                ObserveSoldier(soldier);
            });
            for (std::size_t id = 0; id < seen.size(); ++id) {
                if (!seen.at(id)) {
                    shooters_.at(id).initialized = false;
                }
            }
            if (tick % 3U == 0) {
                state.ForEachBullet([&](const auto& bullet) {
                    if (bullet.style == BulletType::FlameArrow ||
                        bullet.style == BulletType::Flame) {
                        Add(WeaponVisualEffectKind::Flame, bullet.particle.position, {}, 4.0F, 12);
                    } else if (bullet.style == BulletType::LAWMissile ||
                               bullet.style == BulletType::M79Grenade) {
                        Add(WeaponVisualEffectKind::Smoke,
                            bullet.particle.position,
                            { 0.0F, -0.1F },
                            3.0F,
                            20);
                    }
                });
            }
            state.ForEachItem([&](const auto& item) {
                if (item.style != ItemType::M2 || item.id >= m2_heat_.size()) {
                    return;
                }
                if (item.interest > m2_heat_.at(item.id)) {
                    Add(WeaponVisualEffectKind::Casing,
                        item.skeleton->GetPos(4),
                        { 0.5F, -1.0F },
                        1.0F,
                        60,
                        WeaponType::M2);
                    Add(WeaponVisualEffectKind::Flame,
                        item.skeleton->GetPos(4) + glm::vec2(4.0F, -10.0F),
                        {},
                        5.0F,
                        4);
                }
                m2_heat_.at(item.id) = item.interest;
            });
            last_tick_ = tick;
        }
        for (const auto& event : events) {
            ObserveEvent(event);
        }
    }

private:
    struct Shooter
    {
        bool initialized{};
        std::array<WeaponState, 2> weapons{};
    };
    struct EventKey
    {
        std::uint32_t projectile;
        unsigned int tick;
        ProjectileEventKind kind;
        std::optional<std::uint8_t> soldier;
        glm::vec2 position;
        bool operator==(const EventKey&) const = default;
    };
    static bool HasCasing(WeaponType kind)
    {
        return kind >= WeaponType::DesertEagles && kind <= WeaponType::USSOCOM;
    }
    void Add(WeaponVisualEffectKind kind,
             glm::vec2 position,
             glm::vec2 velocity,
             float size,
             int lifetime,
             WeaponType weapon = WeaponType::NoWeapon)
    {
        constexpr std::size_t MAX_EFFECTS = 256;
        if (effects_.size() == MAX_EFFECTS) {
            effects_.erase(effects_.begin());
        }
        effects_.push_back({ .kind = kind,
                             .weapon = weapon,
                             .position = position,
                             .old_position = position,
                             .velocity = velocity,
                             .size = size,
                             .lifetime = lifetime,
                             .initial_lifetime = lifetime });
    }
    void ObserveSoldier(const Soldier& soldier)
    {
        if (soldier.id >= shooters_.size() || soldier.active_weapon >= 2) {
            return;
        }
        auto& shooter = shooters_.at(soldier.id);
        if (!soldier.IsAlive()) {
            shooter.initialized = false;
            return;
        }
        const auto state = soldier.GetActiveWeapon().GetState();
        auto& previous = shooter.weapons.at(soldier.active_weapon);
        if (shooter.initialized && previous.kind == state.kind &&
            previous.instance_id == state.instance_id && previous.realistic == state.realistic &&
            IsSerialNumberNewer(state.shot_sequence, previous.shot_sequence) &&
            HasCasing(state.kind)) {
            const auto count = state.kind == WeaponType::DesertEagles ? 2 : 1;
            for (int casing = 0; casing < count; ++casing) {
                const float variation =
                  static_cast<float>((state.shot_sequence + casing) % 7U) / 10.0F;
                Add(WeaponVisualEffectKind::Casing,
                    soldier.skeleton->GetPos(15),
                    soldier.particle.GetVelocity() +
                      glm::vec2(0.4F * static_cast<float>(soldier.direction), -1.0F - variation),
                    1.0F,
                    60,
                    state.kind);
            }
        }
        if (!shooter.initialized || previous.kind != state.kind ||
            previous.instance_id != state.instance_id || previous.realistic != state.realistic ||
            !IsSerialNumberOlder(state.shot_sequence, previous.shot_sequence)) {
            previous = state;
        }
        shooter.initialized = true;
    }
    void Advance(const StateManager& state)
    {
        for (auto& effect : effects_) {
            effect.old_position = effect.position;
            effect.old_rotation = effect.rotation;
            effect.position += effect.velocity;
            if (effect.kind == WeaponVisualEffectKind::Casing) {
                effect.velocity.y += 0.06F;
                effect.rotation += 0.2F;
                glm::vec2 perpendicular{};
                if (state.GetConstMap().CollisionTest(effect.position, perpendicular)) {
                    effect.lifetime = 0;
                }
            }
            effect.velocity *= 0.99F;
            --effect.lifetime;
        }
        std::erase_if(effects_, [](const auto& effect) { return effect.lifetime <= 0; });
    }
    void ObserveEvent(const ProjectilePhysicsEvent& event)
    {
        const EventKey key{
            event.projectile_id, event.tick, event.kind, event.soldier_id, event.position
        };
        if (std::ranges::find(seen_events_, key) != seen_events_.end()) {
            return;
        }
        seen_events_.push_back(key);
        if (seen_events_.size() > 512) {
            seen_events_.pop_front();
        }
        switch (event.kind) {
            case ProjectileEventKind::ProjectileExploded:
                if (event.style == BulletType::M2Bullet) {
                    Add(WeaponVisualEffectKind::ClusterSmoke, event.position, {}, 1.0F, 55);
                    break;
                }
                if (event.style != BulletType::Cluster) {
                    const int smoke_lifetime = event.style == BulletType::FragGrenade ? 190 : 255;
                    Add(WeaponVisualEffectKind::ExplosionAfterSmoke,
                        event.position,
                        {},
                        1.0F,
                        smoke_lifetime);
                    Add(WeaponVisualEffectKind::ExplosionSmoke, event.position, {}, 1.0F, 50);
                }
                Add(WeaponVisualEffectKind::Explosion,
                    event.position,
                    {},
                    1.0F,
                    48,
                    event.style == BulletType::Cluster
                      ? WeaponType::Cluster
                      : (event.style == BulletType::FragGrenade ? WeaponType::FragGrenade
                                                                : WeaponType::M79));
                break;
            case ProjectileEventKind::ProjectileFragmented:
                Add(WeaponVisualEffectKind::ClusterSmoke, event.position, {}, 1.0F, 55);
                break;
            case ProjectileEventKind::ProjectileRicocheted:
                Add(WeaponVisualEffectKind::Spark,
                    event.position,
                    event.velocity_after * 0.1F,
                    3.0F,
                    12);
                break;
            case ProjectileEventKind::ProjectileImpact:
                Add(WeaponVisualEffectKind::Smoke, event.position, { 0.0F, -0.1F }, 4.0F, 15);
                break;
            default:
                break;
        }
    }
    std::array<int, 128> m2_heat_{};
    std::optional<unsigned int> last_tick_;
    std::array<Shooter, 32> shooters_{};
    std::deque<EventKey> seen_events_;
    std::vector<WeaponVisualEffect> effects_;
};
} // namespace Soldank
