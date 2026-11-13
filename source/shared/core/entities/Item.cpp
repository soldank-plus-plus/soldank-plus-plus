module;

#include <cstdint>
#include <memory>
#include <array>

export module Shared.Core.Entities.Item;

import Shared.Core.Types.ItemType;
import Shared.Core.Physics.Particles;
import Shared.Core.Map.Polygons.PolygonBackgroundState;

export namespace Soldank
{

struct Item
{
    PolygonBackgroundState polygon_background{};
    bool background_touched{};
    bool has_collision_owner{};
    std::uint8_t collision_owner_team{};
    std::int16_t mounted_soldier{ -1 };
    bool active{};
    ItemType style{ ItemType::USSOCOM };
    std::uint8_t id;
    std::uint8_t owner;
    std::int16_t holding_soldier_id{ -1 };
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
