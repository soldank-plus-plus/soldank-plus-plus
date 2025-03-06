#ifndef __ITEM_HPP__
#define __ITEM_HPP__

#include "core/types/ItemType.hpp"

#include <cstdint>
#include <memory>
#include <array>

namespace Soldank
{
class ParticleSystem;

struct Item
{
    bool active{};
    ItemType style;
    std::uint8_t id;
    std::uint8_t owner;
    std::uint8_t holding_soldier_id;
    std::uint8_t ammo_count;
    float radius;
    std::int32_t time_out;
    bool static_type;
    std::int32_t interest;
    bool collide_with_bullets;
    bool in_base;
    std::uint8_t last_spawn;
    std::uint8_t team;
    bool flipped;
    std::shared_ptr<ParticleSystem> skeleton;
    std::array<std::uint8_t, 4> collide_count;
};
} // namespace Soldank

#endif
