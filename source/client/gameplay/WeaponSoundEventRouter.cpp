module;

#include <array>
#include <algorithm>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <string_view>
#include <utility>

export module Gameplay.WeaponSoundEventRouter;

import Audio.AudioEvents;
import Shared.Core.Utility.SerialNumber;
import Shared.Core.Entities.Soldier;
import Shared.Core.Entities.Item;
import Shared.Core.Entities.Weapon;
import Shared.Core.State.StateManager;
import Shared.Core.Animations;
import Shared.Core.Types.WeaponType;
import Shared.Core.Types.ItemType;
import Shared.Core.Types.BulletType;
import Shared.Core.Physics.PhysicsEvents;
import Shared.Core.Physics.Bullets.BulletEvents;
import Shared.Core.WorldEvents;

export namespace Soldank
{
struct WeaponSounds
{
    std::string_view fire;
    std::string_view reload;
    std::string_view startup;
    bool continuous{};
};

WeaponSounds SoundsFor(WeaponType kind)
{
    constexpr std::array<WeaponSounds, 23> SOUNDS{ {
      { "deserteagle-fire", "deserteagle-reload", "" },
      { "mp5-fire", "mp5-reload", "" },
      { "ak74-fire", "ak74-reload", "" },
      { "steyraug-fire", "steyraug-reload", "" },
      { "spas12-fire", "spas12-reload", "" },
      { "ruger77-fire", "ruger77-reload", "" },
      { "m79-fire", "m79-reload", "" },
      { "barretm82-fire", "barretm82-reload", "law-start" },
      { "m249-fire", "m249-reload", "" },
      { "minigun-fire", "minigun-reload", "minigun-start", true },
      { "colt1911-fire", "colt1911-reload", "" },
      { "slash", "", "" },
      { "chainsaw-r", "chainsaw-o", "", true },
      { "law", "law-end", "law-start" },
      { "bow-fire", "bow-reload", "" },
      { "bow-fire", "bow-reload", "" },
      { "flamer", "", "", true },
      { "m2fire", "", "" },
      { "", "", "" },
      { "grenade-throw", "", "" },
      { "grenade-throw", "", "" },
      { "", "", "" },
      { "knife", "", "" },
    } };
    return SOUNDS.at(static_cast<std::size_t>(kind));
}
} // namespace Soldank

export namespace Soldank
{
// Translates simulation actions and committed snapshots to playback requests.
// No renderer or audio device is required, and this router never steps gameplay.
class WeaponSoundEventRouter
{
public:
    static std::shared_ptr<WeaponSoundEventRouter> Create(
      PhysicsEvents& physics,
      WorldEvents& world,
      StateManager& state,
      std::shared_ptr<AudioEvents> events,
      std::function<bool()> should_present = [] { return true; })
    {
        auto router = std::shared_ptr<WeaponSoundEventRouter>(
          new WeaponSoundEventRouter(std::move(events), std::move(should_present)));
        router->paused_ = state.IsGamePaused();
        for (std::size_t id = 0; id < router->entries_.size(); ++id) {
            router->entries_.at(id).channel = static_cast<std::uint8_t>(id);
        }
        // Capture weak ownership because Observable subscriptions can outlive the subscriber.
        const auto weak = std::weak_ptr<WeaponSoundEventRouter>(router);
        physics.weapon_action_event.AddObserver([weak](const WeaponActionEvent& event) {
            if (const auto self = weak.lock(); self && self->CanPresent()) {
                self->ObserveAction(event);
            }
        });
        physics.projectile_event.AddObserver([weak](const ProjectilePhysicsEvent& event) {
            if (const auto self = weak.lock(); self && self->CanPresent()) {
                self->ObserveProjectile(event);
            }
        });
        world.after_soldier_updates.AddObserver([weak](const Soldier& soldier, unsigned int tick) {
            if (const auto self = weak.lock();
                self && self->CanPresent() && soldier.id < self->entries_.size()) {
                self->Observe(soldier, tick, self->entries_.at(soldier.id));
            }
        });
        world.after_item_updates.AddObserver([weak](const Item& item, unsigned int) {
            if (const auto self = weak.lock(); self && self->CanPresent()) {
                self->ObserveItem(item);
            }
        });
        world.after_soldier_spawns.AddObserver([weak](const Soldier& soldier) {
            if (const auto self = weak.lock(); self && self->CanPresent()) {
                self->RemoveSoldier(soldier.id);
                self->Observe(soldier, 0, self->entries_.at(soldier.id));
            }
        });
        world.soldier_died.AddObserver([weak](const Soldier& soldier) {
            if (const auto self = weak.lock(); self && self->should_present_()) {
                self->RemoveSoldier(soldier.id);
            }
        });
        state.soldier_removed.AddObserver([weak](std::uint8_t id) {
            if (const auto self = weak.lock()) {
                self->RemoveSoldier(id);
            }
        });
        state.game_pause_changed.AddObserver([weak](bool paused) {
            if (const auto self = weak.lock()) {
                self->paused_ = paused;
                if (paused) {
                    self->events_->sound_requested.Notify({ AudioCommand::StopAll });
                    for (auto& entry : self->entries_) {
                        entry.loop_initialized = false;
                    }
                }
            }
        });
        // Seed existing entities once. There is no per-frame state scan.
        state.ForEachSoldier([&](const Soldier& soldier) {
            if (soldier.id < router->entries_.size()) {
                router->Observe(soldier, state.GetGameTick(), router->entries_.at(soldier.id));
            }
        });
        return router;
    }

private:
    WeaponSoundEventRouter(std::shared_ptr<AudioEvents> events,
                           std::function<bool()> should_present)
        : events_(std::move(events))
        , should_present_(std::move(should_present))
    {
    }
    bool CanPresent() const { return !paused_ && should_present_(); }
    void RemoveSoldier(std::uint8_t id)
    {
        if (id < entries_.size()) {
            auto& entry = entries_.at(id);
            StopLoop(entry);
            entry.initialized = false;
            entry.grenade_pin_active = false;
            entry.mounted = -1;
        }
    }
    void ObserveItem(const Item& item)
    {
        if (item.style != ItemType::M2 || item.mounted_soldier < 0 ||
            static_cast<std::size_t>(item.mounted_soldier) >= entries_.size()) {
            return;
        }
        auto& entry = entries_.at(item.mounted_soldier);
        if (entry.mounted != item.id) {
            Play("m2use");
            entry.heat = item.interest;
        } else if (item.interest > entry.heat) {
            Play("m2fire");
        }
        if (item.interest == 19 && entry.heat < 19) {
            Play("m2overheat");
        }
        entry.mounted = item.id;
        entry.heat = item.interest;
    }
    void ObserveAction(const WeaponActionEvent& event)
    {
        if (event.soldier_id >= entries_.size() || event.slot >= 3) {
            return;
        }
        auto& entry = entries_.at(event.soldier_id);
        auto& previous = entry.weapons.at(event.slot);
        const auto& current = event.weapon;
        const auto sounds = SoundsFor(current.kind);
        if (event.slot < 2) {
            if (entry.active != event.slot || previous.kind != current.kind ||
                previous.instance_id != current.instance_id) {
                StopLoop(entry);
            }
            entry.active = event.slot;
        }
        const bool same = entry.initialized && previous.kind == current.kind &&
                          previous.instance_id == current.instance_id &&
                          previous.realistic == current.realistic;
        if (event.kind == WeaponActionKind::Dropped || event.kind == WeaponActionKind::Switched) {
            StopLoop(entry);
            return;
        }
        switch (event.kind) {
            case WeaponActionKind::Fired:
            case WeaponActionKind::GrenadeThrown:
                if (!same || IsSerialNumberNewer(current.shot_sequence, previous.shot_sequence)) {
                    entry.last_shot_tick = event.tick;
                    if (sounds.continuous) {
                        StartLoop(entry, sounds.fire);
                    } else {
                        Play(sounds.fire);
                    }
                }
                break;
            case WeaponActionKind::ReloadStarted:
                StopLoop(entry);
                if (current.kind != WeaponType::Spas12 && (!same || !previous.reloading)) {
                    Play(sounds.reload);
                }
                break;
            case WeaponActionKind::ShellInserted:
                if (!same || current.ammo > previous.ammo) {
                    Play(sounds.reload);
                }
                break;
            case WeaponActionKind::StartupStarted:
                Play(sounds.startup);
                break;
            default:
                break;
        }
        const auto sequence =
          same && IsSerialNumberOlder(current.shot_sequence, previous.shot_sequence)
            ? previous.shot_sequence
            : current.shot_sequence;
        previous = current;
        previous.shot_sequence = sequence;
        entry.initialized = true;
    }
    struct Entry
    {
        bool initialized{};
        std::array<WeaponState, 3> weapons{};
        std::uint8_t active{};
        std::uint32_t last_shot_tick{};
        bool loop_initialized{};
        std::uint8_t channel{};
        std::int16_t mounted{ -1 };
        int heat{};
        bool grenade_pin_active{};
    };
    struct ProjectileSoundKey
    {
        std::uint32_t id;
        unsigned int tick;
        ProjectileEventKind kind;
        bool operator==(const ProjectileSoundKey&) const = default;
    };
    void ObserveProjectile(const ProjectilePhysicsEvent& event)
    {
        if (event.kind != ProjectileEventKind::ProjectileExploded &&
            event.kind != ProjectileEventKind::ProjectileRicocheted &&
            event.kind != ProjectileEventKind::ProjectileBounced &&
            event.kind != ProjectileEventKind::ProjectileFragmented) {
            return;
        }
        const ProjectileSoundKey key{ event.projectile_id, event.tick, event.kind };
        if (std::ranges::find(projectile_sounds_, key) != projectile_sounds_.end()) {
            return;
        }
        projectile_sounds_.push_back(key);
        if (projectile_sounds_.size() > 512) {
            projectile_sounds_.pop_front();
        }
        if (event.kind == ProjectileEventKind::ProjectileFragmented) {
            Play("clustergrenade");
        } else if (event.kind == ProjectileEventKind::ProjectileExploded) {
            switch (event.style) {
                case BulletType::M2Bullet:
                    Play("m2explode");
                    break;
                case BulletType::M79Grenade:
                case BulletType::LAWMissile:
                    Play("m79-explosion");
                    break;
                case BulletType::Cluster:
                    Play("cluster-explosion");
                    break;
                default:
                    Play("grenade-explosion");
                    break;
            }
        } else if (event.kind == ProjectileEventKind::ProjectileRicocheted) {
            Play("ric");
        } else if (event.style == BulletType::FragGrenade ||
                   event.style == BulletType::ClusterGrenade) {
            Play("grenade-bounce");
        }
    }
    void Play(std::string_view name)
    {
        if (!name.empty()) {
            events_->sound_requested.Notify({ AudioCommand::Play, name });
        }
    }
    void StopLoop(Entry& entry)
    {
        if (entry.loop_initialized) {
            events_->sound_requested.Notify({ AudioCommand::StopLoop, {}, entry.channel });
            entry.loop_initialized = false;
        }
    }
    void StartLoop(Entry& entry, std::string_view name)
    {
        if (!entry.loop_initialized && !name.empty()) {
            events_->sound_requested.Notify({ AudioCommand::StartLoop, name, entry.channel });
            entry.loop_initialized = true;
        }
    }
    void ObserveWeapon(const Weapon& weapon,
                       const WeaponState& current,
                       std::uint32_t tick,
                       Entry& entry,
                       WeaponSounds sounds,
                       std::uint8_t slot)
    {
        if (entry.initialized && entry.weapons.at(slot).kind == current.kind &&
            entry.weapons.at(slot).instance_id == current.instance_id &&
            entry.weapons.at(slot).realistic == current.realistic) {
            const auto previous = entry.weapons.at(slot);
            const bool shot = IsSerialNumberNewer(current.shot_sequence, previous.shot_sequence);
            if (shot) {
                entry.last_shot_tick = tick;
                if (sounds.continuous) {
                    StartLoop(entry, sounds.fire);
                } else {
                    Play(sounds.fire);
                }
            }
            if ((current.reloading && !previous.reloading && current.kind != WeaponType::Spas12) ||
                (current.kind == WeaponType::Spas12 && current.ammo > previous.ammo)) {
                Play(sounds.reload);
            }
            if (current.startup < previous.startup &&
                previous.startup == weapon.GetWeaponParameters().start_up_time) {
                Play(sounds.startup);
            }
            if (current.kind == WeaponType::Minigun && current.startup > previous.startup) {
                Play("minigun-end");
            }
            if (IsSerialNumberOlder(current.shot_sequence, previous.shot_sequence)) {
                // Preserve the high-water sequence across a correction/replay.
                entry.weapons.at(slot) = current;
                entry.weapons.at(slot).shot_sequence = previous.shot_sequence;
            } else {
                entry.weapons.at(slot) = current;
            }
        } else {
            entry.weapons.at(slot) = current;
        }
    }

    void Observe(const Soldier& soldier, std::uint32_t tick, Entry& entry)
    {
        if (!soldier.IsAlive() || soldier.weapons.size() != 3) {
            StopLoop(entry);
            entry.initialized = false;
            entry.grenade_pin_active = false;
            return;
        }
        if (soldier.mounted_weapon < 0) {
            entry.mounted = -1;
        }
        const auto& weapon = soldier.GetActiveWeapon();
        const auto current = weapon.GetState();
        const auto slot = soldier.active_weapon;
        const auto sounds = SoundsFor(current.kind);
        if (entry.active != slot || entry.weapons.at(slot).kind != current.kind ||
            entry.weapons.at(slot).instance_id != current.instance_id ||
            entry.weapons.at(slot).realistic != current.realistic) {
            StopLoop(entry);
        }
        if (entry.initialized && entry.weapons.at(slot).kind == WeaponType::Knife &&
            current.kind == WeaponType::NoWeapon) {
            Play("knife");
        }
        ObserveWeapon(weapon, current, tick, entry, sounds, slot);
        if (current.reloading || current.ammo == 0 || !sounds.continuous || !soldier.control.fire ||
            soldier.mounted_weapon >= 0 ||
            soldier.body_animation->GetType() == AnimationType::Throw ||
            soldier.body_animation->GetType() == AnimationType::ThrowWeapon ||
            tick - entry.last_shot_tick > weapon.GetWeaponParameters().fire_interval + 1U) {
            StopLoop(entry);
        }
        const auto grenade = soldier.weapons.at(2).GetState();
        const bool pulling_pin = soldier.body_animation->GetType() == AnimationType::Throw &&
                                 !grenade.action_consumed && grenade.ammo > 0;
        if (entry.initialized && pulling_pin && !entry.grenade_pin_active) {
            Play("grenade-pullout");
        }
        entry.grenade_pin_active = pulling_pin;
        const auto& previous_grenade = entry.weapons.at(2);
        const bool same_grenade = previous_grenade.kind == grenade.kind &&
                                  previous_grenade.instance_id == grenade.instance_id;
        if (entry.initialized && same_grenade &&
            IsSerialNumberNewer(grenade.shot_sequence, previous_grenade.shot_sequence)) {
            Play("grenade-throw");
        }
        if (!same_grenade ||
            !IsSerialNumberOlder(grenade.shot_sequence, previous_grenade.shot_sequence)) {
            entry.weapons.at(2) = grenade;
        }
        entry.active = slot;
        entry.initialized = true;
    }

    std::deque<ProjectileSoundKey> projectile_sounds_;
    std::shared_ptr<AudioEvents> events_;
    std::function<bool()> should_present_;
    bool paused_{};
    std::array<Entry, 32> entries_{};
};
} // namespace Soldank
