module;

#include <cstdint>
#include <vector>
#include <optional>

export module Shared.Core.Entities.Bullet;

import Extern.Glm;

import Shared.Core.Physics.Particles;
import Shared.Core.Types.BulletType;
import Shared.Core.Types.TeamType;
import Shared.Core.Types.WeaponType;

export namespace Soldank
{
struct BulletParams
{
    BulletType style;
    WeaponType weapon;
    glm::vec2 position;
    glm::vec2 velocity;
    std::int16_t timeout;
    float hit_multiply;
    TeamType team;
    std::uint8_t owner_id;
    float push;
    std::uint16_t owner_ping_ticks{};
    std::uint32_t id{};
    std::uint32_t seed{};
    unsigned int spawn_tick{};
    bool derived{};
};

struct ItemCollisionCooldown
{
    std::uint8_t item_id;
    std::uint32_t cooldown_end_tick;
};

struct Bullet
{
    Bullet() = default;

    Bullet(BulletParams params)
        : active(true)
        , style(params.style)
        , weapon(params.weapon)
        , team(params.team)
        , owner_id(params.owner_id)
        , particle(true,
                   params.position,
                   params.position,
                   params.velocity,
                   glm::vec2(0.0F, 0.0F),
                   1.0F,
                   1.0F,
                   0.06 * 2.25,
                   0.99,
                   0.0F)
        , initial_position(params.position)
        , velocity_prev(params.velocity)
        , timeout(params.timeout)
        , timeout_prev(params.timeout)
        , timeout_real((float)params.timeout)
        , hit_multiply(params.hit_multiply)
        , hit_multiply_prev(params.hit_multiply)
        , push(params.push)
        , owner_ping_ticks(params.owner_ping_ticks)
        , id(params.id)
        , seed(params.seed)
        , spawn_tick(params.spawn_tick)
    {
    }

    bool active = false;
    BulletType style{};
    WeaponType weapon{};
    TeamType team{};
    std::uint8_t owner_id{};
    Particle particle{};
    glm::vec2 initial_position{};
    glm::vec2 velocity_prev{};
    std::int16_t timeout{};
    std::int16_t timeout_prev{};
    float timeout_real{};
    float hit_multiply{};
    float hit_multiply_prev{};
    glm::vec2 hit_spot{};
    std::int32_t ricochet_count{};
    std::uint32_t degrade_count = 0;
    float push{};
    std::uint16_t owner_ping_ticks{};
    std::uint32_t id{};
    std::uint32_t seed{};
    unsigned int spawn_tick{};
    bool stuck{};
    std::optional<std::uint8_t> attached_soldier;
    std::optional<unsigned int> attached_part;
    std::vector<std::uint8_t> penetrated_soldiers;
    std::vector<ItemCollisionCooldown> item_collision_cooldowns;
};
} // namespace Soldank
