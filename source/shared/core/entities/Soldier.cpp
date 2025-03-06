
#include "core/entities/Soldier.hpp"

#include "core/entities/WeaponParametersFactory.hpp"
#include "core/physics/Particles.hpp"
#include "core/types/WeaponType.hpp"
#include "core/animations/states/LegsStandAnimationState.hpp"
#include "core/animations/states/BodyStandAnimationState.hpp"

#include "spdlog/spdlog.h"

#include <utility>

const float GRAV = 0.06F;

namespace Soldank
{
Soldier::Soldier(std::uint8_t soldier_id,
                 const AnimationDataManager& animation_data_manager,
                 std::shared_ptr<ParticleSystem> skeleton,
                 const std::vector<Weapon>& initial_weapons)
    : id(soldier_id)
    , num(1)
    , visible(1)
    , direction(1)
    , old_direction(1)
    , health(150.0)
    , alpha(255.0)
    , has_cigar(1)
    , collider_distance(255)
    , skeleton(std::move(skeleton))
    , legs_animation(std::make_shared<LegsStandAnimationState>(animation_data_manager))
    , body_animation(std::make_shared<BodyStandAnimationState>(animation_data_manager))
    , control()
    , weapons{ initial_weapons }
    , weapon_choices{ WeaponType::DesertEagles, WeaponType::Knife }
    , particle(false,
               { 0.0F, 0.0F },
               { 0.0F, 0.0F },
               glm::vec2(0.0F, 0.0F),
               glm::vec2(0.0F, 0.0F),
               1.0F,
               1.0F,
               GRAV,
               0.99,
               0.0F)
{
}

void Soldier::SetDefaultValues()
{
    active = false;
    dead_meat = true;
    style = 0;
    num = 1;
    visible = 1;
    on_ground = false;
    on_ground_for_law = false;
    on_ground_last_frame = false;
    on_ground_permanent = false;
    direction = 1;
    old_direction = 1;
    health = 150.0;
    alpha = 255.0;
    jets_count = 0;
    jets_count_prev = 0;
    wear_helmet = 0;
    has_cigar = 1;
    vest = 0.0;
    idle_time = 0;
    idle_random = 0;
    stance = 0;
    on_fire = 0;
    collider_distance = 255;
    half_dead = false;
    is_shooting = false;
}

SoldierSnapshot::SoldierSnapshot(const Soldier& soldier)
    : body_animation_type_(soldier.body_animation->GetType())
    , legs_animation_type_(soldier.legs_animation->GetType())
    , body_animation_frame_(soldier.body_animation->GetFrame())
    , legs_animation_frame_(soldier.legs_animation->GetFrame())
    , body_animation_speed_(soldier.body_animation->GetSpeed())
    , legs_animation_speed_(soldier.legs_animation->GetSpeed())
    , body_animation_count_(soldier.body_animation->GetCount())
    , legs_animation_count_(soldier.legs_animation->GetCount())
    , on_ground_(soldier.on_ground)
    , control_(soldier.control)
    , position_(soldier.particle.position)
    , old_position_(soldier.particle.old_position)
    , jets_count_(soldier.jets_count)
    , jets_count_prev_(soldier.jets_count_prev)
    , stance_(soldier.stance)
    , velocity_(soldier.particle.GetVelocity())
    , force_(soldier.particle.GetForce())
{
}
void SoldierSnapshot::CompareAndLog(const Soldier& other_soldier)
{
    if (body_animation_type_ != other_soldier.body_animation->GetType()) {
        spdlog::debug("body_animation type difference: {} != {}",
                      std::to_underlying(body_animation_type_),
                      std::to_underlying(other_soldier.body_animation->GetType()));
    }
    if (legs_animation_type_ != other_soldier.legs_animation->GetType()) {
        spdlog::debug("legs_animation type difference: {} != {}",
                      std::to_underlying(legs_animation_type_),
                      std::to_underlying(other_soldier.legs_animation->GetType()));
    }

    if (body_animation_frame_ != other_soldier.body_animation->GetFrame()) {
        spdlog::debug("body_animation_frame difference: {} != {}",
                      body_animation_frame_,
                      other_soldier.body_animation->GetFrame());
    }
    if (legs_animation_frame_ != other_soldier.legs_animation->GetFrame()) {
        spdlog::debug("legs_animation_frame difference: {} != {}",
                      legs_animation_frame_,
                      other_soldier.legs_animation->GetFrame());
    }

    if (body_animation_speed_ != other_soldier.body_animation->GetSpeed()) {
        spdlog::debug("body_animation_speed difference: {} != {}",
                      body_animation_speed_,
                      other_soldier.body_animation->GetSpeed());
    }
    if (legs_animation_speed_ != other_soldier.legs_animation->GetSpeed()) {
        spdlog::debug("legs_animation_speed difference: {} != {}",
                      legs_animation_speed_,
                      other_soldier.legs_animation->GetSpeed());
    }

    if (body_animation_count_ != other_soldier.body_animation->GetCount()) {
        spdlog::debug("body_animation_count difference: {} != {}",
                      body_animation_count_,
                      other_soldier.body_animation->GetCount());
    }
    if (legs_animation_count_ != other_soldier.legs_animation->GetCount()) {
        spdlog::debug("legs_animation_count difference: {} != {}",
                      legs_animation_count_,
                      other_soldier.legs_animation->GetCount());
    }

    if (on_ground_ != other_soldier.on_ground) {
        spdlog::debug("on_ground difference: {} != {}", on_ground_, other_soldier.on_ground);
    }

    if (control_.jets != other_soldier.control.jets) {
        spdlog::debug(
          "control.jets difference: {} != {}", control_.jets, other_soldier.control.jets);
    }
    if (control_.left != other_soldier.control.left) {
        spdlog::debug(
          "control.left difference: {} != {}", control_.left, other_soldier.control.left);
    }
    if (control_.right != other_soldier.control.right) {
        spdlog::debug(
          "control.right difference: {} != {}", control_.right, other_soldier.control.right);
    }
    if (control_.down != other_soldier.control.down) {
        spdlog::debug(
          "control.down difference: {} != {}", control_.down, other_soldier.control.down);
    }
    if (control_.up != other_soldier.control.up) {
        spdlog::debug("control.up difference: {} != {}", control_.up, other_soldier.control.up);
    }
    if (control_.prone != other_soldier.control.prone) {
        spdlog::debug(
          "control.prone difference: {} != {}", control_.prone, other_soldier.control.prone);
    }

    if ((std::abs(position_.x - other_soldier.particle.position.x) > 0.001F) ||
        (std::abs(position_.y - other_soldier.particle.position.y) > 0.001F)) {
        spdlog::debug("position difference: ({}, {}) != ({}, {})",
                      position_.x,
                      position_.y,
                      other_soldier.particle.position.x,
                      other_soldier.particle.position.y);
    }
    if ((std::abs(old_position_.x - other_soldier.particle.old_position.x) > 0.001F) ||
        (std::abs(old_position_.y - other_soldier.particle.old_position.y) > 0.001F)) {
        spdlog::debug("old_position difference: ({}, {}) != ({}, {})",
                      old_position_.x,
                      old_position_.y,
                      other_soldier.particle.old_position.x,
                      other_soldier.particle.old_position.y);
    }

    if (jets_count_ != other_soldier.jets_count) {
        spdlog::debug("jets_count difference: {} != {}", jets_count_, other_soldier.jets_count);
    }
    if (jets_count_prev_ != other_soldier.jets_count_prev) {
        spdlog::debug(
          "jets_count_prev difference: {} != {}", jets_count_prev_, other_soldier.jets_count_prev);
    }

    if (stance_ != other_soldier.stance) {
        spdlog::debug("stance difference: {} != {}", stance_, other_soldier.stance);
    }

    if ((std::abs(velocity_.x - other_soldier.particle.GetVelocity().x) > 0.001F) ||
        (std::abs(velocity_.y - other_soldier.particle.GetVelocity().y) > 0.001F)) {
        spdlog::debug("velocity difference: ({}, {}) != ({}, {})",
                      velocity_.x,
                      velocity_.y,
                      other_soldier.particle.GetVelocity().x,
                      other_soldier.particle.GetVelocity().y);
    }

    if ((std::abs(force_.x - other_soldier.particle.GetForce().x) > 0.001F) ||
        (std::abs(force_.y - other_soldier.particle.GetForce().y) > 0.001F)) {
        spdlog::debug("velocity difference: ({}, {}) != ({}, {})",
                      force_.x,
                      force_.y,
                      other_soldier.particle.GetForce().x,
                      other_soldier.particle.GetForce().y);
    }
}
} // namespace Soldank
