module;

#include <cstdint>
#include <array>

export module Shared.Networking.NetworkPackets;

import Shared.Core.Entities.Weapon;
import Shared.Core.State.Control;
import Shared.Core.Animations;
import Shared.Core.Types.BulletType;
import Shared.Core.Types.TeamType;
import Shared.Core.Types.WeaponType;
import Shared.Core.Types.ItemType;
import Shared.Core.Physics.Bullets.BulletEvents;
import Shared.Core.Physics.Bullets.BulletTypes;

export namespace Soldank
{
#pragma pack(push, 1)
struct SoldierInputPacket
{
    std::uint32_t input_sequence_id;
    // Client simulation tick at which this control state was predicted.
    std::uint32_t client_tick;
    // Authoritative server tick at which the control state must be simulated.
    std::uint32_t apply_server_tick;
    float position_x;
    float position_y;
    float mouse_map_position_x;
    float mouse_map_position_y;
    Control control;
    bool has_weapon_choice{};
    WeaponType primary_choice{ WeaponType::DesertEagles };
    WeaponType secondary_choice{ WeaponType::Knife };
};
#pragma pack(pop)

#pragma pack(push, 1)
struct SoldierStatePacket
{
    // Authoritative server simulation tick represented by this snapshot.
    std::uint32_t server_tick;
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
    std::int8_t old_direction;
    std::uint8_t stance;
    float mouse_map_position_x;
    float mouse_map_position_y;
    bool using_jets;
    std::int32_t jets_count;
    std::uint8_t active_weapon;
    std::uint8_t weapon_count{};
    std::uint32_t weapon_generation{};
    std::uint64_t weapon_config_hash{};
    std::array<WeaponState, 3> weapons{};
    bool grenade_can_throw{};
    std::uint16_t hit_spray{};
    std::int16_t mounted_weapon{ -1 };
    CombatBonus combat_bonus{};
    std::uint16_t bonus_ticks{};
    std::uint8_t fired{};
    Control control{};
    float health{ 150.0F };
    float vest{};
    bool dead_meat{};
    std::int32_t ticks_to_respawn{};
    std::uint32_t last_applied_input_id;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct SoldierInfoPacket
{
    std::uint8_t soldier_id;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct ItemParticlePacket
{
    float x{};
    float y{};
    float old_x{};
    float old_y{};
    bool operator==(const ItemParticlePacket&) const = default;
};
struct ItemStatePacket
{
    std::uint32_t server_tick{};
    std::uint8_t id{};
    bool active{};
    ItemType style{ ItemType::USSOCOM };
    std::uint8_t owner{};
    std::uint8_t ammo{};
    float radius{};
    std::int32_t timeout{};
    bool is_static{};
    std::int32_t heat{};
    std::int16_t mounted_soldier{ -1 };
    std::uint8_t particle_count{};
    std::array<ItemParticlePacket, 4> particles{};
    bool operator==(const ItemStatePacket&) const = default;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct ProjectileSpawnPacket
{
    std::uint32_t projectile_id;
    BulletType style{ BulletType::Bullet };
    WeaponType weapon;
    float position_x;
    float position_y;
    float velocity_x;
    float velocity_y;
    std::int16_t timeout;
    float hit_multiply;
    TeamType team;
    std::uint8_t owner_id;
    float push{};
    std::uint16_t owner_ping_ticks{};
    std::uint32_t seed{};
    unsigned int spawn_tick{};
    bool derived{};
    bool realistic{};
};
#pragma pack(pop)
#pragma pack(push, 1)
struct ProjectilePhysicsPacket
{
    ProjectileEventKind kind{};
    std::uint32_t projectile_id{};
    BulletType style{ BulletType::Bullet };
    WeaponType weapon{};
    std::uint8_t owner_id{};
    std::uint32_t seed{};
    unsigned int tick{};
    float position_x{};
    float position_y{};
    float impulse_x{};
    float impulse_y{};
    float radius{};
    unsigned int apply_tick{};
    std::uint8_t soldier_id{ 255 };
    std::uint8_t item_id{ 255 };
    unsigned int particle_id{};
    ItemType item_style{};
    float velocity_before_x{};
    float velocity_before_y{};
    float velocity_after_x{};
    float velocity_after_y{};
    float damage{};
    float distance{};
    float normal_x{};
    float normal_y{};
    bool has_collision{};
    BulletCollisionKind collision_kind{};
    unsigned int polygon_id{ 0xffffffffU };
    unsigned int collider_id{ 0xffffffffU };
    std::uint32_t target_projectile_id{};
    ProjectileDestructionReason reason{};
};
#pragma pack(pop)
} // namespace Soldank
