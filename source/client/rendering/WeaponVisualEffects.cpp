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
import Shared.Core.Animations;
import Shared.Core.Entities.Weapon;
import Shared.Core.State.StateManager;
import Shared.Core.Types.WeaponType;
import Shared.Core.Types.BulletType;
import Shared.Core.Types.ItemType;
import Shared.Core.Physics.Bullets.BulletEvents;
import Shared.Core.Utility.SerialNumber;
import Shared.Core.Physics.Polygons.PolygonContact;
import Shared.Core.Map.Map;
import Shared.Core.Map.PMSEnums;
import Shared.Core.Entities.Bullet;
import Shared.Core.Map.Polygons.PolygonCollisionPolicy;

export import EffectDefinitions;

export namespace Soldank
{
// Presentation-only particles. Their RNG and collisions never modify simulation state.
class WeaponVisualEffects
{
public:
    const std::vector<WeaponVisualEffect>& GetEffects() const { return effects_; }
    unsigned int GetCigarSmokeSoundCount() const { return cigar_smoke_sound_count_; }
    void ObservePolygon(const PolygonEffectEvent& event)
    {
        current_owner_.reset();
        if (event.kind == PolygonEventKind::Lava) {
            Add(WeaponVisualEffectKind::Flame,
                event.position - glm::vec2(0.0F, 3.0F),
                { 0.0F, -1.3F },
                4.0F,
                40);
        } else if (event.kind == PolygonEventKind::Death) {
            const unsigned int count = event.severity == DeathSeverity::Brutal ? 12
                                       : event.severity == DeathSeverity::Head ? 6
                                                                               : 2;
            for (unsigned int index = 0; index < count; ++index) {
                const float angle = static_cast<float>(index) * 2.4F;
                Add(WeaponVisualEffectKind::Blood,
                    event.position,
                    { std::cos(angle) * 1.4F, std::sin(angle) * 1.4F - 0.5F },
                    3.0F,
                    35);
            }
        }
    }

    void ObserveEmote(const Soldier& soldier, std::uint32_t cues)
    {
        current_owner_ = OwnerContext{ soldier.id,
                                       static_cast<std::uint8_t>(soldier.GetTeam()),
                                       soldier.is_holding_flags,
                                       soldier.life_sequence };
        const auto has = [&](EmoteCue cue) {
            return (cues & static_cast<std::uint32_t>(cue)) != 0;
        };
        const auto emit = [&](EmoteCue cue,
                              WeaponVisualEffectKind kind,
                              glm::vec2 position,
                              glm::vec2 velocity,
                              int lifetime) {
            unsigned int index = 0;
            while ((1U << index) != static_cast<std::uint32_t>(cue))
                ++index;
            const int age = soldier.emote.cue_ages[index];
            if (!has(cue) || age >= lifetime)
                return false;
            const auto& origins = soldier.emote.cigar_origins;
            const auto slot = index - 3U;
            if ((origins.valid_mask & (1U << slot)) != 0) {
                position = kind == WeaponVisualEffectKind::Match ? origins.match_position
                                                                 : origins.positions[slot];
                if (kind == WeaponVisualEffectKind::Match)
                    velocity.x = origins.directions[slot] / 2.0F;
                if (kind == WeaponVisualEffectKind::Cigar)
                    velocity.x = origins.directions[slot] / 1.5F;
            }
            Add(kind, position, velocity, 1, lifetime - age);
            effects_.back().initial_lifetime = lifetime;
            return true;
        };
        const float direction = static_cast<float>(soldier.direction);
        const auto head = soldier.skeleton->GetPos(12);
        const auto hand = soldier.skeleton->GetPos(15);
        if (has(EmoteCue::Spit)) {
            auto aim = glm::vec2(soldier.control.mouse_aim_x, soldier.control.mouse_aim_y) - head;
            const float length = glm::length(aim);
            Add(WeaponVisualEffectKind::Spit,
                head,
                length > 0.001F ? aim * (2.0F / length) : glm::vec2(direction * 2, 0),
                1,
                245);
        }
        if (emit(EmoteCue::Light,
                 WeaponVisualEffectKind::Match,
                 hand,
                 { direction / 2, 0.15F },
                 245)) {
            effects_.back().tint = { 187.0F / 255, 170.0F / 255, 169.0F / 255, 1 };
        }
        for (auto cue : { EmoteCue::Light, EmoteCue::PuffFirst, EmoteCue::PuffSecond }) {
            emit(cue,
                 WeaponVisualEffectKind::CigarSmoke,
                 head + glm::vec2(direction * 4, 0),
                 { 0, -0.7F },
                 65);
        }
        emit(
          EmoteCue::Discard, WeaponVisualEffectKind::Cigar, hand, { direction / 1.5F, 0.1F }, 245);
        current_owner_.reset();
    }

    void ObserveSpawn(const Soldier& soldier)
    {
        if (soldier.id >= spawned_lives_.size() ||
            spawned_lives_.at(soldier.id) == soldier.life_sequence) {
            return;
        }
        spawned_lives_.at(soldier.id) = soldier.life_sequence;
        current_owner_ = OwnerContext{ soldier.id,
                                       static_cast<std::uint8_t>(soldier.GetTeam()),
                                       soldier.is_holding_flags,
                                       soldier.life_sequence };
        Add(WeaponVisualEffectKind::Spawn, soldier.particle.position, {}, 1.0F, 33);
        effects_.back().tint = soldier.appearance.shirt;
        current_owner_.reset();
    }

    void Reset()
    {
        effects_.clear();
        cigar_smoke_sound_count_ = 0;
        seen_events_.clear();
        shooters_ = {};
        spawned_lives_ = {};
        m2_heat_ = {};
        last_tick_.reset();
        current_owner_.reset();
        random_ = 0xA341316CU;
    }

    glm::vec2 GetCameraShake(glm::vec2 camera, float fraction) const
    {
        glm::vec2 offset{};
        for (const auto& effect : effects_) {
            if (effect.kind != WeaponVisualEffectKind::Explosion || effect.lifetime <= 36) {
                continue;
            }
            const float attenuation =
              std::clamp(1.0F - glm::length(effect.position - camera) / 500.0F, 0.0F, 1.0F);
            const float phase = static_cast<float>(effect.lifetime) + fraction;
            offset += glm::vec2(std::sin(phase * 13.0F), std::cos(phase * 17.0F)) * attenuation *
                      (effect.lifetime / 6.0F);
        }
        return glm::clamp(offset, glm::vec2(-10.0F), glm::vec2(10.0F));
    }

    void Update(const StateManager& state,
                std::span<const ProjectilePhysicsEvent> events,
                glm::vec2 camera = {},
                glm::vec2 viewport = { 800.0F, 600.0F },
                bool weather = true)
    {
        cigar_smoke_sound_count_ = 0;
        if (state.IsGamePaused()) {
            return;
        }
        const auto tick = state.GetGameTick();
        if (last_tick_ && IsSerialNumberOlder(tick, *last_tick_)) {
            Reset();
        }
        if (!last_tick_ || tick != *last_tick_) {
            const unsigned int steps = last_tick_ ? std::min(5U, tick - *last_tick_) : 0;
            for (unsigned int step = 0; step < steps; ++step) {
                Advance(state);
            }
            last_observed_tick_ = tick;
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
            current_owner_.reset();
            state.ForEachBullet([&](const auto& bullet) {
                const auto* owner = state.FindSoldier(
                  [&](const Soldier& soldier) { return soldier.id == bullet.owner_id; });
                current_owner_ =
                  owner != nullptr
                    ? std::optional(OwnerContext{ owner->id,
                                                  static_cast<std::uint8_t>(owner->GetTeam()),
                                                  owner->is_holding_flags,
                                                  owner->life_sequence })
                    : std::nullopt;
                if (!bullet.active) {
                    return;
                }
                if (bullet.style == BulletType::FlameArrow) {
                    if (Chance(2)) {
                        Add(WeaponVisualEffectKind::Flame,
                            bullet.particle.position,
                            { 0.0F, -0.5F },
                            1.0F,
                            40);
                    }
                    if (Chance(2)) {
                        Add(WeaponVisualEffectKind::BlackSmoke,
                            bullet.particle.position,
                            { 0.0F, -0.5F },
                            1.0F,
                            40);
                    }
                } else if (bullet.style == BulletType::LAWMissile) {
                    Add(WeaponVisualEffectKind::Exhaust,
                        bullet.particle.position,
                        { 0.0F, -1.5F },
                        1.0F,
                        50);
                }
                if (!bullet.penetrated_soldiers.empty() && Chance(5)) {
                    Add(WeaponVisualEffectKind::SmallBlood,
                        bullet.particle.position,
                        bullet.particle.GetVelocity() * 0.5F,
                        1.0F,
                        90);
                }
            });
            current_owner_.reset();
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
                    effects_.back().owner = static_cast<std::uint8_t>(item.mounted_soldier);
                    Add(WeaponVisualEffectKind::Flame,
                        item.skeleton->GetPos(4) + glm::vec2(4.0F, -10.0F),
                        {},
                        5.0F,
                        4);
                }
                m2_heat_.at(item.id) = item.interest;
            });
            current_owner_.reset();
            if (weather && tick % 17U == 0) {
                const auto weather_type = state.GetConstMap().GetWeatherType();
                if (weather_type != PMSWeatherType::None) {
                    const auto kind =
                      weather_type == PMSWeatherType::Rain   ? WeaponVisualEffectKind::Rain
                      : weather_type == PMSWeatherType::Snow ? WeaponVisualEffectKind::Snow
                                                             : WeaponVisualEffectKind::Sand;
                    const glm::vec2 velocity =
                      weather_type == PMSWeatherType::Rain   ? glm::vec2(0.0F, 12.0F)
                      : weather_type == PMSWeatherType::Snow ? glm::vec2(1.0F, 2.0F)
                                                             : glm::vec2(10.0F, 7.0F);
                    const unsigned int count =
                      static_cast<unsigned int>(std::clamp(viewport.x / 128.0F, 8.0F, 24.0F));
                    for (unsigned int index = 0; index < count; ++index) {
                        const glm::vec2 position =
                          camera - viewport * 0.5F +
                          glm::vec2(-128.0F + (viewport.x + 256.0F) * index / count +
                                      RandomVelocity() * 45.0F,
                                    -128.0F + RandomVelocity() * 75.0F);
                        Add(kind,
                            position,
                            velocity,
                            1.0F,
                            kind == WeaponVisualEffectKind::Rain ? 60 : 80);
                    }
                }
            }
            last_tick_ = tick;
        }
        for (const auto& event : events) {
            const auto* owner = state.FindSoldier(
              [&](const Soldier& soldier) { return soldier.id == event.owner_id; });
            current_owner_ =
              owner != nullptr
                ? std::optional(OwnerContext{ owner->id,
                                              static_cast<std::uint8_t>(owner->GetTeam()),
                                              owner->is_holding_flags,
                                              owner->life_sequence })
                : std::nullopt;
            ObserveEvent(event);
        }
        current_owner_.reset();
    }

private:
    struct OwnerContext
    {
        std::uint8_t id{};
        std::uint8_t team{};
        bool carrying_flag{};
        std::uint32_t life{};
    };
    std::optional<OwnerContext> current_owner_;
    struct Shooter
    {
        bool initialized{};
        std::array<WeaponState, 2> weapons{};
        std::uint32_t life_sequence{};
        bool alive{};
        bool on_ground{};
        bool holding_flag{};
        std::uint8_t has_cigar{};
    };
    struct EventKey
    {
        std::uint32_t projectile;
        unsigned int tick;
        ProjectileEventKind kind;
        std::optional<std::uint8_t> soldier;
        std::optional<std::uint8_t> item;
        std::optional<unsigned int> particle;
        std::optional<unsigned int> polygon;
        bool operator==(const EventKey&) const = default;
    };
    bool Chance(unsigned int denominator)
    {
        random_ ^= random_ << 13;
        random_ ^= random_ >> 17;
        random_ ^= random_ << 5;
        return random_ % denominator == 0;
    }
    unsigned int RandomStep(unsigned int range)
    {
        Chance(1);
        return random_ % range;
    }
    float RandomVelocity()
    {
        Chance(1);
        return static_cast<float>(random_ % 2001U) / 1000.0F - 1.0F;
    }
    void Burst(WeaponVisualEffectKind kind,
               glm::vec2 position,
               glm::vec2 velocity,
               unsigned int count,
               float speed,
               int lifetime)
    {
        for (unsigned int index = 0; index < count; ++index) {
            Add(kind,
                position,
                velocity + glm::vec2(RandomVelocity(), RandomVelocity()) * speed,
                1.0F,
                lifetime);
        }
    }
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
        constexpr std::size_t MAX_EFFECTS = 2048;
        if (effects_.size() == MAX_EFFECTS) {
            const auto expendable = std::ranges::find_if(effects_, [](const auto& effect) {
                return effect.kind != WeaponVisualEffectKind::Explosion &&
                       effect.kind != WeaponVisualEffectKind::ExplosionSmoke;
            });
            effects_.erase(expendable == effects_.end() ? effects_.begin() : expendable);
        }
        effects_.push_back(
          { .kind = kind,
            .weapon = weapon,
            .position = position,
            .old_position = position,
            .velocity = velocity,
            .size = size,
            .lifetime = lifetime,
            .initial_lifetime = lifetime,
            .owner = current_owner_ ? current_owner_->id : static_cast<std::uint8_t>(255),
            .variant = static_cast<std::uint8_t>(random_ % 4U),
            .team = current_owner_ ? current_owner_->team : static_cast<std::uint8_t>(0),
            .carrying_flag = current_owner_ && current_owner_->carrying_flag,
            .has_owner = current_owner_.has_value(),
            .owner_life = current_owner_ ? current_owner_->life : 0U });
    }
    void ObserveSoldier(const Soldier& soldier)
    {
        if (soldier.id >= shooters_.size() || soldier.active_weapon >= 2) {
            return;
        }
        current_owner_ = OwnerContext{ soldier.id,
                                       static_cast<std::uint8_t>(soldier.GetTeam()),
                                       soldier.is_holding_flags,
                                       soldier.life_sequence };
        auto& shooter = shooters_.at(soldier.id);
        const bool alive = soldier.IsAlive();
        if (shooter.initialized &&
            (shooter.life_sequence != soldier.life_sequence || (!shooter.alive && alive))) {
            ObserveSpawn(soldier);
            current_owner_ = OwnerContext{ soldier.id,
                                           static_cast<std::uint8_t>(soldier.GetTeam()),
                                           soldier.is_holding_flags,
                                           soldier.life_sequence };
            shooter.weapons = {};
        }
        if (shooter.initialized && shooter.alive && !alive) {
            Burst(WeaponVisualEffectKind::Blood,
                  soldier.skeleton->GetPos(9),
                  soldier.particle.GetVelocity() * 0.5F,
                  12,
                  2.0F,
                  80);
            if (soldier.death_severity != DeathSeverity::Normal) {
                Burst(
                  WeaponVisualEffectKind::Debris, soldier.skeleton->GetPos(9), {}, 8, 3.0F, 180);
                for (unsigned int index = 0; index < 6; ++index) {
                    Add(WeaponVisualEffectKind::Cloth,
                        soldier.skeleton->GetPos(6),
                        glm::vec2(RandomVelocity(), RandomVelocity()) * 2.0F,
                        1.0F,
                        120);
                    effects_.back().tint =
                      index % 2 == 0 ? soldier.appearance.shirt : soldier.appearance.pants;
                }
            }
        }
        if (shooter.initialized && !shooter.on_ground && soldier.on_ground && alive) {
            Burst(WeaponVisualEffectKind::Smoke, soldier.skeleton->GetPos(1), {}, 2, 0.3F, 35);
        }
        if (shooter.initialized && shooter.alive && !alive) {
            if (soldier.appearance.headgear != 0 && soldier.wear_helmet == 1) {
                Add(WeaponVisualEffectKind::Headgear,
                    soldier.skeleton->GetPos(12),
                    soldier.particle.GetVelocity(),
                    1.0F,
                    198);
                effects_.back().variant = soldier.appearance.headgear;
                effects_.back().tint = soldier.appearance.shirt;
            }
            const bool already_discarded =
              (soldier.emote.cues & static_cast<std::uint32_t>(EmoteCue::Discard)) != 0;
            if (soldier.has_cigar == 10 ||
                (!already_discarded && shooter.life_sequence == soldier.life_sequence &&
                 shooter.has_cigar == 10)) {
                Add(WeaponVisualEffectKind::Cigar,
                    soldier.skeleton->GetPos(12),
                    soldier.particle.GetVelocity(),
                    1.0F,
                    245);
            }
        }
        shooter.life_sequence = soldier.life_sequence;
        shooter.alive = alive;
        shooter.on_ground = soldier.on_ground;
        shooter.has_cigar = soldier.has_cigar;
        if (!alive) {
            shooter.initialized = true;
            return;
        }
        if (soldier.control.jets && soldier.jets_count > 0) {
            for (unsigned int foot = 1; foot <= 2; ++foot) {
                const glm::vec2 position = soldier.skeleton->GetPos(foot) + glm::vec2(-1.0F, 3.0F);
                if (Chance(8)) {
                    Add(WeaponVisualEffectKind::Smoke,
                        position,
                        soldier.particle.GetVelocity(),
                        1.0F,
                        75);
                }
                if (Chance(7)) {
                    const auto leg = foot == 1
                                       ? soldier.skeleton->GetPos(5) - soldier.skeleton->GetPos(4)
                                       : soldier.skeleton->GetPos(6) - soldier.skeleton->GetPos(3);
                    const float length = glm::length(leg);
                    Add(WeaponVisualEffectKind::JetFire,
                        position,
                        length > 0.001F ? leg * (-0.5F / length) : glm::vec2(0.0F, 0.5F),
                        1.0F,
                        40);
                    effects_.back().tint = soldier.appearance.jet;
                }
            }
        }
        if (soldier.health < 90.0F && Chance(12)) {
            Add(WeaponVisualEffectKind::SmallBlood,
                soldier.skeleton->GetPos(6),
                soldier.particle.GetVelocity() * 0.5F,
                1.0F,
                60);
        }
        if (soldier.body_animation->GetType() == AnimationType::Piss) {
            const auto frame = soldier.body_animation->GetFrame();
            const int chance = frame < 22 ? 2 : frame < 34 ? 3 : 4;
            if (frame > 8 && frame < 35 && Chance(chance)) {
                const auto origin = soldier.skeleton->GetPos(20);
                auto aim = glm::vec2(soldier.control.mouse_aim_x, soldier.control.mouse_aim_y) - origin;
                const float length = glm::length(aim);
                const float speed = frame >= 22 && frame < 34 ? 1.9F : 1.3F;
                Add(WeaponVisualEffectKind::Urine,
                    origin,
                    length > 0.001F ? aim * (speed / length)
                                    : glm::vec2(speed * soldier.direction, 0),
                    1,
                    frame < 22 ? 165 : 120);
                effects_.back().tint = { 1, 0.9F, 0.2F, 1 };
            }
        }
        if (soldier.has_cigar == 10 && last_observed_tick_ % 160U == 0) {
            const float direction = static_cast<float>(soldier.direction);
            const auto mouth = soldier.skeleton->GetPos(9) - glm::vec2(0, 2);
            Add(WeaponVisualEffectKind::CigarSmoke,
                mouth + glm::vec2(direction * 4, 0),
                { 0, -0.75F },
                1,
                55);
            if (Chance(2)) {
                Add(WeaponVisualEffectKind::CigarSmoke,
                    mouth + glm::vec2(direction * 4.1F, 0),
                    { 0, -0.69F },
                    1,
                    55);
                ++cigar_smoke_sound_count_;
                if (Chance(2)) {
                    Add(WeaponVisualEffectKind::CigarSmoke,
                        mouth + glm::vec2(direction * 3.9F, 0),
                        { 0, -0.81F },
                        1,
                        55);
                }
            }
        }
        if (soldier.on_fire > 0) {
            if (Chance(2)) {
                Add(WeaponVisualEffectKind::Flame,
                    soldier.skeleton->GetPos(6),
                    { 0.0F, -0.5F },
                    1.0F,
                    35);
            }
            if (Chance(4)) {
                Add(WeaponVisualEffectKind::BlackSmoke,
                    soldier.skeleton->GetPos(9),
                    { 0.0F, -0.5F },
                    1.0F,
                    75);
            }
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
                    255,
                    state.kind);
                effects_.back().owner = soldier.id;
            }
        }
        if (shooter.initialized && previous.kind == state.kind &&
            previous.instance_id == state.instance_id &&
            IsSerialNumberNewer(state.shot_sequence, previous.shot_sequence)) {
            Add(WeaponVisualEffectKind::MuzzleSmoke,
                soldier.skeleton->GetPos(15),
                { 0.0F, -0.1F },
                1.0F,
                10);
        }
        const auto clip_out = soldier.GetActiveWeapon().GetClipOutTime();
        if (shooter.initialized && previous.instance_id == state.instance_id &&
            previous.kind == state.kind && previous.reloading && state.reloading && clip_out > 0 &&
            previous.reload > clip_out && state.reload <= clip_out) {
            Add(WeaponVisualEffectKind::Magazine,
                soldier.skeleton->GetPos(15),
                soldier.particle.GetVelocity() + glm::vec2(0.3F * soldier.direction, 0.5F),
                1.0F,
                255,
                state.kind);
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
        std::vector<WeaponVisualEffect> children;
        const auto& map = state.GetConstMap();
        for (auto& effect : effects_) {
            effect.old_position = effect.position;
            effect.old_rotation = effect.rotation;
            const bool cigar_debris = effect.kind == WeaponVisualEffectKind::Match ||
                                      effect.kind == WeaponVisualEffectKind::Cigar;
            const bool cigar_smoke = effect.kind == WeaponVisualEffectKind::CigarSmoke;
            const bool debris = effect.kind == WeaponVisualEffectKind::Casing ||
                                effect.kind == WeaponVisualEffectKind::Pin ||
                                effect.kind == WeaponVisualEffectKind::Magazine ||
                                effect.kind == WeaponVisualEffectKind::Debris ||
                                effect.kind == WeaponVisualEffectKind::Cloth ||
                                effect.kind == WeaponVisualEffectKind::Headgear ||
                                effect.kind == WeaponVisualEffectKind::Cigar ||
                                effect.kind == WeaponVisualEffectKind::Match ||
                                effect.kind == WeaponVisualEffectKind::Spit ||
                                effect.kind == WeaponVisualEffectKind::Urine;
            const bool blood = effect.kind == WeaponVisualEffectKind::Blood ||
                               effect.kind == WeaponVisualEffectKind::SmallBlood;
            if (debris || blood || effect.kind == WeaponVisualEffectKind::Ember ||
                effect.kind == WeaponVisualEffectKind::Spark) {
                effect.velocity.y += cigar_debris ? 0.06F / 1.4F : 0.06F;
                if (!cigar_debris)
                    effect.rotation += 0.2F;
            }
            // Reference smoke style 31 skips Euler integration entirely.
            if (!cigar_smoke)
                effect.position += effect.velocity;
            if (cigar_debris)
                effect.velocity *= 0.998F;
            if ((debris || blood || effect.kind == WeaponVisualEffectKind::JetFire) &&
                effect.has_owner) {
                const PolygonQuery query{ .kind = PolygonQueryKind::Cosmetic,
                                          .team = effect.team,
                                          .carrying_flag = effect.carrying_flag,
                                          .has_owner = true };
                const auto sector = map.GetSectorIndex(effect.position);
                const auto collide = [&](unsigned int index) {
                    const auto& polygon = map.GetPolygons().at(index);
                    if (!AllowsPolygon(polygon.polygon_type, query) ||
                        !Map::PointInPoly(effect.position, polygon)) {
                        return false;
                    }
                    float distance = 0.0F;
                    int edge = 0;
                    auto normal = map.ClosestPerpendicular(
                      static_cast<int>(index), effect.position, &distance, &edge);
                    const float length = glm::length(normal);
                    if (length > 0.001F) {
                        normal /= length;
                        if (cigar_debris) {
                            const auto perpendicular = normal * distance;
                            effect.velocity = (effect.velocity - perpendicular) * 0.7F;
                            auto spark = effect;
                            spark.kind = WeaponVisualEffectKind::Spark;
                            spark.velocity = perpendicular * 2.5F;
                            spark.velocity.x += -0.5F + static_cast<float>(RandomStep(11)) / 10.0F;
                            spark.velocity.y = -spark.velocity.y;
                            spark.lifetime = spark.initial_lifetime = 35;
                            spark.collisions = 0;
                            const bool yellow = Chance(7);
                            spark.variant = 1;
                            spark.tint = yellow
                                           ? glm::vec4(1, 254.0F / 255, 53.0F / 255, 1)
                                           : glm::vec4(170.0F / 255, 170.0F / 255, 170.0F / 255, 1);
                            children.push_back(spark);
                        } else {
                            effect.velocity = (effect.velocity -
                                               normal * 2.0F * glm::dot(effect.velocity, normal)) *
                                              0.5F;
                        }
                    }
                    if (!cigar_debris)
                        effect.position = effect.old_position;
                    if (effect.kind == WeaponVisualEffectKind::Blood) {
                        auto splat = effect;
                        splat.kind = WeaponVisualEffectKind::Splat;
                        splat.velocity = {};
                        splat.lifetime = splat.initial_lifetime = 30;
                        children.push_back(splat);
                    }
                    if (effect.kind == WeaponVisualEffectKind::Urine && effect.collisions == 0) {
                        auto splash = effect;
                        splash.velocity = { RandomVelocity() * 0.5F, -0.5F };
                        splash.lifetime = splash.initial_lifetime = 50;
                        splash.collisions = 4;
                        children.push_back(splash);
                        effect.lifetime = 0;
                    }
                    ++effect.collisions;
                    if (effect.collisions > (cigar_debris ? 5 : blood ? 1 : 4)) {
                        effect.lifetime = 0;
                    }
                    return true;
                };
                if (sector.x >= 0 && sector.y >= 0 && sector.x < map.GetSectorsPerAxis() &&
                    sector.y < map.GetSectorsPerAxis()) {
                    for (const auto polygon : map.GetSector(sector.x, sector.y).polygons) {
                        if (polygon > 0 && polygon <= map.GetPolygons().size() &&
                            collide(polygon - 1U)) {
                            break;
                        }
                    }
                }
            }
            if (!cigar_smoke && !cigar_debris && effect.kind != WeaponVisualEffectKind::Rain &&
                effect.kind != WeaponVisualEffectKind::Sand &&
                effect.kind != WeaponVisualEffectKind::Snow) {
                effect.velocity *= 0.99F;
            }
            --effect.lifetime;
        }
        std::erase_if(effects_, [](const auto& effect) { return effect.lifetime <= 0; });
        for (const auto& child : children) {
            if (effects_.size() < 2048) {
                effects_.push_back(child);
            }
        }
    }
    void ExplosionDebris(const ProjectilePhysicsEvent& event)
    {
        const bool frag = event.style == BulletType::FragGrenade;
        auto position = event.position - event.velocity_before;
        const auto dirt_velocity = [&]() {
            const auto base = event.velocity_before * (frag ? -0.2F : -0.15F);
            return glm::vec2(-base.x + RandomVelocity() * 3.5F,
                             base.y - 0.25F + RandomVelocity() * 3.25F);
        };
        for (unsigned int index = 0; index < (frag ? 6U : 7U); ++index) {
            const auto velocity = dirt_velocity();
            for (unsigned int variant = 0; variant < 4; ++variant) {
                if (Chance(4)) {
                    Add(WeaponVisualEffectKind::Debris,
                        position,
                        velocity,
                        1.0F,
                        180 + random_ % 50U);
                    effects_.back().variant = static_cast<std::uint8_t>(variant);
                }
            }
        }
        for (unsigned int index = 0; index < (frag ? 7U : 5U); ++index) {
            const auto velocity = dirt_velocity();
            for (unsigned int variant = 0; variant < 4; ++variant) {
                if (Chance(frag ? 4 : 3)) {
                    Add(WeaponVisualEffectKind::Debris, position, velocity, 1.0F, 120);
                    effects_.back().variant = static_cast<std::uint8_t>(variant + 4);
                }
            }
        }
        for (unsigned int index = 0; index < (frag ? 3U : 4U); ++index) {
            for (unsigned int spark = 0; spark < 3; ++spark) {
                if (Chance(frag ? 23 : 22)) {
                    Add(WeaponVisualEffectKind::Ember, position, dirt_velocity(), 1.0F, 120);
                }
            }
            position += glm::vec2(RandomVelocity(), RandomVelocity()) * (frag ? 25.0F : 20.0F);
            Add(WeaponVisualEffectKind::Flame, position, dirt_velocity(), 1.0F, 35);
        }
    }
    void ObserveEvent(const ProjectilePhysicsEvent& event)
    {
        const EventKey key{ event.projectile_id,
                            event.tick,
                            event.kind,
                            event.soldier_id,
                            event.item_id,
                            event.particle_id,
                            event.collision ? event.collision->polygon_id : std::nullopt };
        if (std::ranges::find(seen_events_, key) != seen_events_.end()) {
            return;
        }
        seen_events_.push_back(key);
        if (seen_events_.size() > 512) {
            seen_events_.pop_front();
        }
        switch (event.kind) {
            case ProjectileEventKind::SoldierHitByProjectile:
            case ProjectileEventKind::SoldierHitByExplosion:
                if (event.damage > 0.0F) {
                    auto velocity = event.velocity_before * 0.025F * glm::vec2(1.2F, 0.85F);
                    Add(WeaponVisualEffectKind::SmallBlood, event.position, velocity, 1.0F, 70);
                    velocity *= glm::vec2(0.745F, 1.1F);
                    Add(WeaponVisualEffectKind::SmallBlood, event.position, velocity, 1.0F, 75);
                    velocity *= glm::vec2(0.9F, 0.85F);
                    if (Chance(2)) {
                        Add(WeaponVisualEffectKind::SmallBlood, event.position, velocity, 1.0F, 75);
                    }
                    velocity *= glm::vec2(1.2F, 0.85F);
                    Add(WeaponVisualEffectKind::Blood, event.position, velocity, 1.0F, 80);
                    Add(WeaponVisualEffectKind::Blood, event.position, velocity, 1.0F, 85);
                }
                break;
            case ProjectileEventKind::ProjectileBounced:
                Burst(WeaponVisualEffectKind::Puff, event.position, {}, 2, 0.2F, 20);
                break;
            case ProjectileEventKind::ProjectileSpawned:
                if (event.style == BulletType::FragGrenade ||
                    event.style == BulletType::ClusterGrenade) {
                    Add(WeaponVisualEffectKind::Pin, event.position, { 0.3F, -0.5F }, 1.0F, 255);
                }
                break;
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
                ExplosionDebris(event);
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
                for (unsigned int index = 0; index < 6; ++index) {
                    Add(WeaponVisualEffectKind::Spark,
                        event.position,
                        glm::vec2(RandomVelocity(), RandomVelocity()) * (index < 2 ? 2.0F : 3.0F),
                        1.0F,
                        35);
                    effects_.back().variant = 1;
                    effects_.back().tint =
                      index == 5 ? glm::vec4(0.67F, 0.67F, 0.67F, 1.0F)
                                 : glm::vec4(1.0F, 254.0F / 255.0F, 53.0F / 255.0F, 1.0F);
                }
                break;
            case ProjectileEventKind::ProjectileImpact: {
                auto velocity = event.velocity_before * -0.06F - glm::vec2(0.0F, 1.0F);
                for (int lifetime : { 60, 65, 50 }) {
                    velocity *=
                      glm::vec2(0.9F + RandomVelocity() * 0.3F, 0.9F + RandomVelocity() * 0.2F);
                    Add(WeaponVisualEffectKind::Spark, event.position, velocity, 1.0F, lifetime);
                }
                Add(WeaponVisualEffectKind::Smoke, event.position, velocity * 0.5F, 1.0F, 60);
                Add(WeaponVisualEffectKind::Puff, event.position, {}, 1.0F, 22);
                break;
            }
            case ProjectileEventKind::ThingHitByProjectile:
                Add(WeaponVisualEffectKind::Smoke,
                    event.position,
                    event.velocity_before * -0.01F,
                    1.0F,
                    70);
                break;
            default:
                break;
        }
    }
    std::array<std::optional<std::uint32_t>, 32> spawned_lives_{};
    std::uint32_t random_{ 0xA341316CU };
    unsigned int last_observed_tick_{};
    unsigned int cigar_smoke_sound_count_{};
    std::array<int, 128> m2_heat_{};
    std::optional<unsigned int> last_tick_;
    std::array<Shooter, 32> shooters_{};
    std::deque<EventKey> seen_events_;
    std::vector<WeaponVisualEffect> effects_;
};
} // namespace Soldank
