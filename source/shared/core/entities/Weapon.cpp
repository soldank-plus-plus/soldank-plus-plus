module;

#include <algorithm>
#include <cstdint>
#include <optional>
#include <initializer_list>

export module Shared.Core.Entities.Weapon;

import Shared.Core.Types.WeaponType;
import Shared.Core.Entities.WeaponParameters;

export namespace Soldank
{
enum class CombatBonus : std::uint8_t
{
    None,
    FlameGod,
    Berserk,
};

// Value state shared by authoritative snapshots and prediction history.
struct WeaponState
{
    WeaponType kind{};
    std::uint32_t instance_id{};
    bool realistic{};
    std::uint8_t ammo{};
    std::uint8_t capacity{};
    std::uint16_t fire_interval{};
    std::uint16_t startup{};
    std::uint16_t reload{};
    bool reloading{};
    bool pending_reload{};
    bool can_auto_reload{ true };
    bool action_consumed{};
    bool trigger_latched{};
    std::uint32_t shot_sequence{};
    std::uint8_t burst_count{};

    bool operator==(const WeaponState&) const = default;
};

enum class WeaponActionKind : std::uint8_t
{
    Fired,
    ReloadStarted,
    ReloadCompleted,
    ShellInserted,
    StartupStarted,
    Switched,
    Dropped,
    GrenadeThrown,
};

struct WeaponActionEvent
{
    WeaponActionKind kind{};
    std::uint8_t soldier_id{};
    unsigned int tick{};
    WeaponState weapon{};
    std::uint8_t slot{};
};

class Weapon
{
public:
    Weapon(const WeaponParameters& parameters,
           std::optional<std::uint8_t> ammo = std::nullopt,
           std::optional<std::uint8_t> capacity = std::nullopt)
        : weapon_parameters_(parameters)
        , state_{ .kind = parameters.kind,
                  .realistic = parameters.realistic,
                  .ammo = ammo.value_or(parameters.kind == WeaponType::M79 ? 0 : parameters.ammo),
                  .capacity = capacity.value_or(parameters.ammo),
                  .fire_interval = parameters.fire_interval,
                  .startup = parameters.start_up_time,
                  .reload = parameters.reload_time }
    {
        state_.ammo = std::min(state_.ammo, state_.capacity);
    }

    bool IsAny(std::initializer_list<WeaponType> weapons) const
    {
        return std::ranges::find(weapons, state_.kind) != weapons.end();
    }

    const WeaponParameters& GetWeaponParameters() const { return weapon_parameters_; }
    void SetInstanceId(std::uint32_t id) { state_.instance_id = id; }
    const WeaponState& GetState() const { return state_; }
    bool RestoreState(const WeaponState& state)
    {
        if (state.kind != state_.kind || state.realistic != state_.realistic ||
            state.ammo > state.capacity ||
            (!IsAny({ WeaponType::FragGrenade, WeaponType::ClusterGrenade }) &&
             state.capacity != weapon_parameters_.ammo) ||
            state.fire_interval > weapon_parameters_.fire_interval ||
            state.startup > weapon_parameters_.start_up_time ||
            state.reload > weapon_parameters_.reload_time) {
            return false;
        }
        state_ = state;
        return true;
    }

    bool IsMelee() const
    {
        return state_.kind == WeaponType::NoWeapon || state_.kind == WeaponType::Knife;
    }
    bool IsReloading() const { return state_.reloading; }
    void ResetBurst() { state_.burst_count = 0; }
    void ReleaseTrigger() { state_.trigger_latched = false; }
    void ResetStartUpTimeCount() { state_.startup = weapon_parameters_.start_up_time; }
    std::uint16_t GetReloadTimeCount() const { return state_.reload; }
    std::uint16_t GetFireIntervalCount() const { return state_.fire_interval; }
    std::uint16_t GetStartUpTimeCount() const { return state_.startup; }
    std::uint8_t GetCapacity() const { return state_.capacity; }
    std::uint8_t GetAmmoCount() const { return state_.ammo; }
    std::uint16_t GetClipInTime() const
    {
        return weapon_parameters_.clip_reload ? weapon_parameters_.reload_time * 3U / 10U : 0;
    }
    std::uint16_t GetClipOutTime() const
    {
        return weapon_parameters_.clip_reload ? weapon_parameters_.reload_time * 8U / 10U : 0;
    }

    bool BeginReload()
    {
        if (IsMelee() || state_.reloading || state_.ammo >= state_.capacity) {
            return false;
        }
        if (state_.kind == WeaponType::Spas12 && state_.fire_interval != 0) {
            return false;
        }
        ResetBurst();
        state_.pending_reload = false;
        state_.reloading = true;
        state_.reload = weapon_parameters_.reload_time;
        if (state_.kind != WeaponType::Spas12) {
            state_.ammo = 0;
        }
        ResetStartUpTimeCount();
        return true;
    }

    void RequestReload()
    {
        state_.pending_reload = state_.ammo < state_.capacity && !IsMelee() && !state_.reloading;
    }
    bool CanAutoReload() const { return state_.can_auto_reload; }
    void ResetThrow() { state_.action_consumed = false; }
    bool ConsumeThrow()
    {
        if (state_.action_consumed || !ConsumeRound()) {
            return false;
        }
        state_.action_consumed = true;
        return true;
    }

    void Advance(bool fire_held, bool allow_reload_progress = true)
    {
        if (state_.fire_interval > 0) {
            --state_.fire_interval;
        }
        if (!fire_held) {
            ReleaseTrigger();
            ResetStartUpTimeCount();
            state_.can_auto_reload = true;
            ResetBurst();
        }
        if (state_.pending_reload && allow_reload_progress) {
            BeginReload();
        }
        if (!allow_reload_progress || !state_.reloading || state_.kind == WeaponType::Spas12) {
            return;
        }
        if (state_.reload > 0) {
            --state_.reload;
        }
        if (state_.reload == 0) {
            state_.ammo = state_.capacity;
            state_.reload = weapon_parameters_.reload_time;
            state_.fire_interval = weapon_parameters_.fire_interval;
            state_.reloading = false;
            ResetStartUpTimeCount();
        }
    }

    void InsertShell()
    {
        if (state_.kind == WeaponType::Spas12 && state_.reloading) {
            state_.ammo = std::min<int>(state_.ammo + 1, state_.capacity);
            if (state_.ammo == state_.capacity) {
                state_.reloading = false;
            }
        }
    }

    // Called once per eligible simulation tick. Rejected shots never spend ammo.
    bool TryFire()
    {
        // Weapons.pas FireMode=2: one accepted shot until the trigger is released.
        const bool single_shot = IsAny({ WeaponType::DesertEagles,
                                         WeaponType::Spas12,
                                         WeaponType::Ruger77,
                                         WeaponType::Barrett,
                                         WeaponType::USSOCOM });
        if (single_shot && state_.trigger_latched) {
            return false;
        }
        if (state_.fire_interval != 0 || (!IsMelee() && state_.ammo == 0)) {
            return false;
        }
        if (state_.reloading && state_.kind != WeaponType::Spas12) {
            return false;
        }
        if (state_.startup > 0) {
            --state_.startup;
            return false;
        }
        if (!IsMelee()) {
            --state_.ammo;
        }
        state_.reloading = false;
        state_.pending_reload = false;
        state_.trigger_latched = single_shot;
        state_.can_auto_reload = state_.kind != WeaponType::Spas12;
        state_.fire_interval = weapon_parameters_.fire_interval;
        ++state_.shot_sequence;
        state_.burst_count = std::min<int>(255, state_.burst_count + 1);
        return true;
    }

    bool ConsumeRound()
    {
        if (state_.ammo == 0) {
            return false;
        }
        --state_.ammo;
        ++state_.shot_sequence;
        return true;
    }

private:
    WeaponParameters weapon_parameters_;
    WeaponState state_;
};
} // namespace Soldank
