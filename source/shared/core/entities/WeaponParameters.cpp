module;

#include <string>
#include <cstdint>

export module Shared.Core.Entities.WeaponParameters;

import Shared.Core.Types.WeaponType;
import Shared.Core.Types.BulletType;

export namespace Soldank
{
enum class WeaponNoCollision : std::uint8_t
{
    Enemy = 1U << 0U,
    Team = 1U << 1U,
    Self = 1U << 2U,
    ExplosionEnemy = 1U << 3U,
    ExplosionTeam = 1U << 4U,
    ExplosionSelf = 1U << 5U,
};

constexpr bool HasWeaponNoCollision(std::uint8_t collision_mask, WeaponNoCollision collision)
{
    return (collision_mask & static_cast<std::uint8_t>(collision)) != 0U;
}

constexpr bool AllowsWeaponCollision(std::uint8_t mask, bool self, bool same_team, bool explosion)
{
    const auto category =
      explosion
        ? (self
             ? WeaponNoCollision::ExplosionSelf
             : (same_team ? WeaponNoCollision::ExplosionTeam : WeaponNoCollision::ExplosionEnemy))
        : (self ? WeaponNoCollision::Self
                : (same_team ? WeaponNoCollision::Team : WeaponNoCollision::Enemy));
    return !HasWeaponNoCollision(mask, category);
}

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
    std::uint8_t no_collision{};
    float inherited_velocity;
    float push;
    float speed;
    std::uint16_t start_up_time;
    std::uint16_t reload_time;
    bool clip_reload;
    std::uint16_t timeout;
};
} // namespace Soldank
