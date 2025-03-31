#ifndef __SOLDIER_HPP__
#define __SOLDIER_HPP__

#include "core/animations/AnimationData.hpp"
#include "core/animations/AnimationState.hpp"
#include "core/physics/Particles.hpp"
#include "core/state/Control.hpp"
#include "core/entities/Weapon.hpp"
#include "core/entities/Bullet.hpp"
#include "core/math/Glm.hpp"

#include <cstdint>
#include <vector>
#include <memory>
#include <array>

namespace Soldank
{
struct Soldier
{
    Soldier() = default;
    Soldier(std::uint8_t soldier_id,
            const AnimationDataManager& animation_data_manager,
            std::shared_ptr<ParticleSystem> skeleton,
            const std::vector<Weapon>& initial_weapons);

    void SetDefaultValues();

    std::uint8_t id{};

    glm::vec2 mouse{};
    float game_width{};
    float game_height{};

    bool active{};
    bool dead_meat{ true };
    std::uint8_t style{};
    std::uint32_t num{};
    std::uint8_t visible{};
    bool on_ground{};
    bool on_ground_for_law{};
    bool on_ground_last_frame{};
    bool on_ground_permanent{};
    std::int8_t direction{};
    std::int8_t old_direction{};
    float health{};
    std::uint8_t alpha{};
    std::int32_t jets_count{};
    std::int32_t jets_count_prev{};
    std::uint8_t wear_helmet{};
    std::uint8_t has_cigar{};
    float vest{};
    std::int32_t idle_time{};
    std::int8_t idle_random{};
    std::uint8_t stance{};
    std::uint8_t on_fire{};
    std::uint8_t collider_distance{};
    bool half_dead{};
    std::shared_ptr<ParticleSystem> skeleton;
    std::shared_ptr<AnimationState> legs_animation;
    std::shared_ptr<AnimationState> body_animation;
    Control control{};
    std::uint8_t active_weapon{};
    std::vector<Weapon> weapons;
    std::array<WeaponType, 2> weapon_choices{};
    std::uint8_t fired{};
    Particle particle;
    bool is_shooting{};
    bool is_holding_flags{};

    int ticks_to_respawn{};

    bool grenade_can_throw{};
};

class SoldierSnapshot
{
public:
    SoldierSnapshot(const Soldier& soldier);

    void CompareAndLog(const Soldier& other_soldier);

    bool IsShooting() const { return is_shooting_; }

private:
    AnimationType body_animation_type_;
    AnimationType legs_animation_type_;

    std::uint32_t body_animation_frame_;
    std::uint32_t legs_animation_frame_;

    std::int32_t body_animation_speed_;
    std::int32_t legs_animation_speed_;

    std::int32_t body_animation_count_;
    std::int32_t legs_animation_count_;

    bool on_ground_;

    Control control_;

    glm::vec2 position_;
    glm::vec2 old_position_;

    std::int32_t jets_count_;
    std::int32_t jets_count_prev_;

    std::uint8_t stance_;

    glm::vec2 velocity_;
    glm::vec2 force_;

    bool is_shooting_;
};
} // namespace Soldank

#endif
