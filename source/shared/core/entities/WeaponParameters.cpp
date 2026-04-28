module;

#include <string>
#include <cstdint>

export module Shared.Core.Entities.WeaponParameters;

import Shared.Core.Types.WeaponType;
import Shared.Core.Types.BulletType;

export namespace Soldank
{
struct WeaponParameters
{
    WeaponType kind;
    std::string name;
    std::string ini_name;
    std::uint8_t ammo;
    float movement_acc;
    std::int16_t bink;
    std::uint16_t recoil;
    std::uint16_t fire_interval;
    float hit_multiply;
    float bullet_spread;
    BulletType bullet_style;
    float modifier_legs;
    float modifier_chest;
    float modifier_head;
    float inherited_velocity;
    float push;
    float speed;
    std::uint16_t start_up_time;
    std::uint16_t reload_time;
    bool clip_reload;
    std::uint16_t timeout;
};
} // namespace Soldank
