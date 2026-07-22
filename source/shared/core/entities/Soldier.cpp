module;

#include "spdlog/spdlog.h"

#include <utility>
#include <cstdint>
#include <vector>
#include <memory>
#include <array>

export module Shared.Core.Entities.Soldier;

import Extern.Glm;

import Shared.Core.Physics.Particles;
import Shared.Core.Entities.WeaponParametersFactory;
import Shared.Core.Types.WeaponType;
import Shared.Core.Animations.States;
import Shared.Core.Animations;
import Shared.Core.State.Control;
import Shared.Core.Entities.Weapon;
import Shared.Core.Entities.Bullet;

const float GRAV = 0.06F;

export namespace Soldank
{
struct Soldier
{
    Soldier() = default;
    Soldier(std::uint8_t soldier_id,
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

    void SetDefaultValues()
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
    SoldierSnapshot(const Soldier& soldier)
        : body_animation_type_(soldier.body_animation->GetType())
        , legs_animation_type_(soldier.legs_animation->GetType())
        , body_animation_frame_(soldier.body_animation->GetFrame())
        , legs_animation_frame_(soldier.legs_animation->GetFrame())
        , body_animation_speed_(soldier.body_animation->GetSpeed())
        , legs_animation_speed_(soldier.legs_animation->GetSpeed())
        , body_animation_count_(soldier.body_animation->GetCount())
        , legs_animation_count_(soldier.legs_animation->GetCount())
        , on_ground_(soldier.on_ground)
        , on_ground_for_law_(soldier.on_ground_for_law)
        , on_ground_last_frame_(soldier.on_ground_last_frame)
        , on_ground_permanent_(soldier.on_ground_permanent)
        , direction_(soldier.direction)
        , old_direction_(soldier.old_direction)
        , control_(soldier.control)
        , position_(soldier.particle.position)
        , old_position_(soldier.particle.old_position)
        , jets_count_(soldier.jets_count)
        , jets_count_prev_(soldier.jets_count_prev)
        , stance_(soldier.stance)
        , velocity_(soldier.particle.GetVelocity())
        , force_(soldier.particle.GetForce())
        , grenade_can_throw_(soldier.grenade_can_throw)
    {
    }

    AnimationType GetBodyAnimationType() const { return body_animation_type_; }
    AnimationType GetLegsAnimationType() const { return legs_animation_type_; }
    std::uint32_t GetBodyAnimationFrame() const { return body_animation_frame_; }
    std::uint32_t GetLegsAnimationFrame() const { return legs_animation_frame_; }
    std::int32_t GetBodyAnimationSpeed() const { return body_animation_speed_; }
    std::int32_t GetLegsAnimationSpeed() const { return legs_animation_speed_; }
    std::int32_t GetBodyAnimationCount() const { return body_animation_count_; }
    std::int32_t GetLegsAnimationCount() const { return legs_animation_count_; }
    const glm::vec2& GetPosition() const { return position_; }
    const glm::vec2& GetOldPosition() const { return old_position_; }
    const glm::vec2& GetVelocity() const { return velocity_; }
    const glm::vec2& GetForce() const { return force_; }
    bool IsOnGround() const { return on_ground_; }
    bool IsOnGroundForLaw() const { return on_ground_for_law_; }
    bool WasOnGroundLastFrame() const { return on_ground_last_frame_; }
    bool IsOnGroundPermanent() const { return on_ground_permanent_; }
    std::int8_t GetDirection() const { return direction_; }
    std::int8_t GetOldDirection() const { return old_direction_; }
    std::uint8_t GetStance() const { return stance_; }
    std::int32_t GetJetsCount() const { return jets_count_; }
    std::int32_t GetPreviousJetsCount() const { return jets_count_prev_; }
    bool IsUsingJets() const { return control_.jets; }
    const Control& GetControl() const { return control_; }
    bool CanThrowGrenade() const { return grenade_can_throw_; }

    void CompareAndLog(const Soldier& other_soldier)
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
            spdlog::debug("jets_count_prev difference: {} != {}",
                          jets_count_prev_,
                          other_soldier.jets_count_prev);
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
    bool on_ground_for_law_;
    bool on_ground_last_frame_;
    bool on_ground_permanent_;

    std::int8_t direction_;
    std::int8_t old_direction_;

    Control control_;

    glm::vec2 position_;
    glm::vec2 old_position_;

    std::int32_t jets_count_;
    std::int32_t jets_count_prev_;

    std::uint8_t stance_;

    glm::vec2 velocity_;
    glm::vec2 force_;

    bool grenade_can_throw_;
};
} // namespace Soldank
