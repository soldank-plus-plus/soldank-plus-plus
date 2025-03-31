#ifndef __NETWORK_PACKETS_HPP__
#define __NETWORK_PACKETS_HPP__

#include "core/state/Control.hpp"
#include "core/animations/AnimationData.hpp"
#include "core/types/BulletType.hpp"
#include "core/types/TeamType.hpp"
#include "core/types/WeaponType.hpp"

#include <cstdint>

namespace Soldank
{
#pragma pack(push, 1)
struct SoldierInputPacket
{
    std::uint32_t input_sequence_id;
    std::uint32_t game_tick;
    float position_x;
    float position_y;
    float mouse_map_position_x;
    float mouse_map_position_y;
    Control control;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct SoldierStatePacket
{
    std::uint32_t game_tick;
    std::uint8_t player_id;
    float position_x;
    float position_y;
    float old_position_x;
    float old_position_y;
    AnimationType body_animation_type;
    std::uint32_t body_animation_frame;
    std::int32_t body_animation_speed;
    std::int32_t body_animation_count;
    AnimationType legs_animation_type;
    std::uint32_t legs_animation_frame;
    std::int32_t legs_animation_speed;
    std::int32_t legs_animation_count;
    float velocity_x;
    float velocity_y;
    float force_x;
    float force_y;
    bool on_ground;
    bool on_ground_for_law;
    bool on_ground_last_frame;
    bool on_ground_permanent;
    std::uint8_t stance;
    float mouse_map_position_x;
    float mouse_map_position_y;
    bool using_jets;
    std::int32_t jets_count;
    std::uint8_t active_weapon;
    std::uint32_t last_processed_input_id;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct SoldierInfoPacket
{
    std::uint8_t soldier_id;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct ProjectileSpawnPacket
{
    std::uint16_t projectile_id;
    BulletType style;
    WeaponType weapon;
    float position_x;
    float position_y;
    float velocity_x;
    float velocity_y;
    std::int16_t timeout;
    float hit_multiply;
    TeamType team;
    std::uint8_t owner_id;
    std::uint32_t last_processed_input_id;
    std::int32_t creation_order;
};
#pragma pack(pop)
} // namespace Soldank

#endif
