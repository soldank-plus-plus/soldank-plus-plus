module;

#include <algorithm>
#include <utility>
#include <vector>
#include <cstdint>

export module Shared.Core.Entities.Weapon;

import Shared.Core.Types.WeaponType;
import Shared.Core.Entities.WeaponParameters;

export namespace Soldank
{
class Weapon
{
public:
    Weapon(const WeaponParameters& weapon_parameters)
        : weapon_parameters_(weapon_parameters)
        , ammo_count_(weapon_parameters.ammo)
        , fire_interval_prev_(weapon_parameters.fire_interval)
        , fire_interval_count_(weapon_parameters.fire_interval)
        , fire_interval_real_((float)weapon_parameters.fire_interval)
        , start_up_time_count_(weapon_parameters.start_up_time)
        , reload_time_prev_(weapon_parameters.reload_time)
        , reload_time_count_(weapon_parameters.reload_time)
        , reload_time_real_((float)weapon_parameters.reload_time)
    {

        if (weapon_parameters.clip_reload) {
            clip_out_time_ = (std::uint16_t)((float)weapon_parameters.reload_time * 0.8);
            clip_in_time_ = (std::uint16_t)((float)weapon_parameters.reload_time * 0.3);
        } else {
            clip_out_time_ = 0;
            clip_in_time_ = 0;
        }

        if (weapon_parameters.kind == WeaponType::M79) {
            ammo_count_ = 0;
        }
    }

    bool IsAny(const std::vector<WeaponType>& weapons) const
    {
        return std::ranges::any_of(weapons, [this](WeaponType weapon_type) {
            return weapon_type == GetWeaponParameters().kind;
        });
    }

    const WeaponParameters& GetWeaponParameters() const { return weapon_parameters_; }

    void ResetStartUpTimeCount() { start_up_time_count_ = weapon_parameters_.start_up_time; };
    void SetReloadTimePrev(std::uint16_t reload_time_prev)
    {
        reload_time_prev_ = reload_time_prev;
    };
    std::uint16_t GetReloadTimeCount() const { return reload_time_count_; };

    std::uint8_t GetAmmoCount() const { return ammo_count_; }
    std::uint16_t GetClipInTime() const { return clip_in_time_; }
    std::uint16_t GetClipOutTime() const { return clip_out_time_; }

private:
    WeaponParameters weapon_parameters_;

    std::uint8_t ammo_count_;
    std::uint16_t fire_interval_prev_;
    std::uint16_t fire_interval_count_;
    float fire_interval_real_;
    std::uint16_t start_up_time_count_;
    std::uint16_t reload_time_prev_;
    std::uint16_t reload_time_count_;
    float reload_time_real_;
    std::uint16_t clip_in_time_;
    std::uint16_t clip_out_time_;
};
} // namespace Soldank
