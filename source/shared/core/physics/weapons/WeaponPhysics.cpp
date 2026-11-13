module;

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
#include <numbers>

export module Shared.Core.Physics.Weapons.WeaponPhysics;

import Extern.Glm;
import Shared.Core.Entities.Soldier;
import Shared.Core.Entities.Weapon;
import Shared.Core.Entities.WeaponParameters;
import Shared.Core.Entities.Bullet;
import Shared.Core.Animations;
import Shared.Core.Math.Calc;
import Shared.Core.Types.WeaponType;
import Shared.Core.Types.BulletType;
import Shared.Core.Map.Map;
import Shared.Core.State.StateManager;
import Shared.Core.Entities.Item;
import Shared.Core.Entities.WeaponParametersFactory;
import Shared.Core.Types.ItemType;

namespace Soldank::WeaponPhysics
{
ItemType WeaponTypeToItemType(WeaponType weapon_type)
{
    switch (weapon_type) {
        case WeaponType::DesertEagles:
            return ItemType::DesertEagles;
        case WeaponType::MP5:
            return ItemType::MP5;
        case WeaponType::Ak74:
            return ItemType::Ak74;
        case WeaponType::SteyrAUG:
            return ItemType::SteyrAUG;
        case WeaponType::Spas12:
            return ItemType::Spas12;
        case WeaponType::Ruger77:
            return ItemType::Ruger77;
        case WeaponType::M79:
            return ItemType::M79;
        case WeaponType::Barrett:
            return ItemType::Barrett;
        case WeaponType::Minimi:
            return ItemType::Minimi;
        case WeaponType::Minigun:
            return ItemType::Minigun;
        case WeaponType::USSOCOM:
            return ItemType::USSOCOM;
        case WeaponType::Knife:
            return ItemType::Knife;
        case WeaponType::Chainsaw:
            return ItemType::Chainsaw;
        case WeaponType::LAW:
            return ItemType::LAW;
        case WeaponType::FlameBow:
        case WeaponType::Bow:
            return ItemType::Bow;
        case WeaponType::Flamer:
        case WeaponType::M2:
        case WeaponType::NoWeapon:
        case WeaponType::FragGrenade:
        case WeaponType::ClusterGrenade:
        case WeaponType::Cluster:
        case WeaponType::ThrownKnife:
            std::unreachable();
    }
}

float RandomOffset(std::uint32_t& seed)
{
    seed = seed * 1664525U + 1013904223U;
    return static_cast<float>(seed >> 8U) / 8388608.0F - 1.0F;
}

glm::vec2 Aim(const Soldier& soldier)
{
    const auto target = glm::vec2(soldier.control.mouse_aim_x, soldier.control.mouse_aim_y);
    const auto delta = target - soldier.skeleton->GetPos(15);
    return glm::dot(delta, delta) > 0.000001F ? Calc::Vec2Normalize(delta)
                                              : glm::vec2(soldier.direction, 0.0F);
}

} // namespace Soldank::WeaponPhysics

export namespace Soldank::WeaponPhysics
{
void ThrowWeapon(StateManager& state, Soldier& soldier)
{
    if (soldier.GetActiveWeapon().IsAny({ WeaponType::NoWeapon,
                                          WeaponType::M2,
                                          WeaponType::FragGrenade,
                                          WeaponType::ClusterGrenade,
                                          WeaponType::Cluster,
                                          WeaponType::ThrownKnife })) {
        return;
    }
    if (soldier.GetActiveWeapon().GetWeaponParameters().kind == WeaponType::Flamer) {
        if (soldier.dead_meat || soldier.combat_bonus != CombatBonus::FlameGod) {
            state.ChangeSoldierPrimaryWeapon(soldier.id, WeaponType::NoWeapon);
        }
        return;
    }
    if (!soldier.dead_meat &&
        soldier.GetActiveWeapon().IsAny({ WeaponType::Bow, WeaponType::FlameBow })) {
        return;
    }
    if (!soldier.dead_meat &&
        soldier.GetActiveWeapon().GetWeaponParameters().kind == WeaponType::Knife) {
        const Weapon weapon{ WeaponParametersFactory::GetParameters(WeaponType::ThrownKnife,
                                                                    state.IsRealistic()) };
        const auto aim_x = static_cast<float>(soldier.control.mouse_aim_x);
        const auto aim_y = static_cast<float>(soldier.control.mouse_aim_y);
        const auto dir =
          Calc::Vec2Normalize(glm::vec2(aim_x, aim_y) - soldier.skeleton->GetPos(15));
        const auto frame = static_cast<float>(soldier.body_animation->GetFrame());
        const auto thrown_mul = 1.5F * std::min(16.0F, std::max(8.0F, frame)) / 16.0F;
        const auto bullet_vel = dir * weapon.GetWeaponParameters().speed * thrown_mul;
        const auto inherited_vel =
          soldier.particle.GetVelocity() * weapon.GetWeaponParameters().inherited_velocity;
        const auto velocity = bullet_vel + inherited_vel;

        BulletParams params{
            weapon.GetWeaponParameters().bullet_style,
            weapon.GetWeaponParameters().kind,
            soldier.skeleton->GetPos(16) + velocity,
            velocity,
            static_cast<std::int16_t>(weapon.GetWeaponParameters().timeout),
            weapon.GetWeaponParameters().hit_multiply,
            soldier.GetTeam(),
            soldier.id,
            weapon.GetWeaponParameters().push,
            soldier.GetPingTicks(),
        };
        params.realistic = weapon.GetWeaponParameters().realistic;
        if (state.HasAuthoritativeProjectiles()) {
            state.EnqueueNewProjectile(params);
        }
        state.ChangeSoldierPrimaryWeapon(soldier.id, WeaponType::NoWeapon);
    } else {
        if (!state.HasAuthoritativeProjectiles()) {
            state.ChangeSoldierPrimaryWeapon(soldier.id, WeaponType::NoWeapon);
            return;
        }
        const WeaponType current_soldier_weapon_type =
          soldier.GetActiveWeapon().GetWeaponParameters().kind;
        Item& new_item = state.CreateItem(soldier.skeleton->GetPos(16),
                                          soldier.id,
                                          WeaponTypeToItemType(current_soldier_weapon_type));
        new_item.ammo_count = soldier.GetActiveWeapon().GetAmmoCount();
        new_item.polygon_background = soldier.polygon_background;
        state.ChangeSoldierPrimaryWeapon(soldier.id, WeaponType::NoWeapon);
        new_item.skeleton->SetPos(1, new_item.skeleton->GetPos(1) + soldier.particle.GetVelocity());
        new_item.skeleton->SetPos(2, new_item.skeleton->GetPos(2) + soldier.particle.GetVelocity());

        glm::vec2 aim_direction = { soldier.control.mouse_aim_x, soldier.control.mouse_aim_y };
        aim_direction -= soldier.skeleton->GetPos(15);
        aim_direction = Calc::Vec2Normalize(aim_direction);

        float weapon_throw_speed_pos1 = 0.01F;
        float weapon_throw_speed_pos2 = 3.0F;
        if (soldier.dead_meat) {
            weapon_throw_speed_pos1 = 0.02F;
            weapon_throw_speed_pos2 = 0.64F;
        }

        glm::vec2 weapon_throw_velocity = Calc::Vec2Scale(aim_direction, weapon_throw_speed_pos1);
        new_item.skeleton->SetPos(1, new_item.skeleton->GetPos(1) + weapon_throw_velocity);
        weapon_throw_velocity = Calc::Vec2Scale(aim_direction, weapon_throw_speed_pos2);
        new_item.skeleton->SetPos(2, new_item.skeleton->GetPos(2) + weapon_throw_velocity);
    }
}

bool CanFireLaw(const Soldier& soldier)
{
    const auto animation = soldier.legs_animation->GetType();
    const auto frame = soldier.legs_animation->GetFrame();
    return (soldier.on_ground || soldier.on_ground_permanent || soldier.on_ground_for_law) &&
           ((animation == AnimationType::Crouch && frame > 13) ||
            animation == AnimationType::CrouchRun || animation == AnimationType::CrouchRunBack ||
            (animation == AnimationType::Prone && frame > 23));
}

namespace Detail
{
bool IsProne(const Soldier& soldier)
{
    const auto legs = soldier.legs_animation->GetType();
    return legs == AnimationType::ProneMove ||
           (legs == AnimationType::Prone && soldier.legs_animation->GetFrame() > 23);
}

bool IsCrouched(const Soldier& soldier)
{
    const auto legs = soldier.legs_animation->GetType();
    return legs == AnimationType::CrouchRun || legs == AnimationType::CrouchRunBack ||
           (legs == AnimationType::Crouch && soldier.legs_animation->GetFrame() > 13);
}

float MovementInaccuracy(const Soldier& soldier, const WeaponParameters& parameters)
{
    const auto legs = soldier.legs_animation->GetType();
    const bool moving = soldier.control.jets || legs == AnimationType::Run ||
                        legs == AnimationType::RunBack || legs == AnimationType::Jump ||
                        legs == AnimationType::JumpSide || legs == AnimationType::Roll ||
                        legs == AnimationType::RollBack;
    if (moving) {
        return parameters.movement_acc * 7.0F;
    }
    if (!soldier.on_ground_permanent && !IsProne(soldier) && !IsCrouched(soldier)) {
        return parameters.movement_acc * 3.0F;
    }
    return 0.0F;
}

float Inaccuracy(const Soldier& soldier, const WeaponParameters& parameters)
{
    float result =
      static_cast<float>(soldier.hit_spray) * 0.01F + MovementInaccuracy(soldier, parameters);
    if (parameters.kind == WeaponType::DesertEagles ||
        parameters.bullet_style == BulletType::GaugeBullet) {
        return result * 0.25F;
    }
    float stance_divisor = 1.0F;
    if (IsProne(soldier)) {
        stance_divisor = 1.625F;
    } else if (IsCrouched(soldier)) {
        stance_divisor = 1.3F;
    }
    result += parameters.bullet_spread / stance_divisor;
    return result * 0.25F;
}

struct ShotVectors
{
    glm::vec2 position;
    glm::vec2 direction;
    glm::vec2 velocity;
};

ShotVectors BuildVectors(const Soldier& soldier,
                         const WeaponParameters& parameters,
                         std::uint32_t& seed)
{
    auto direction = Aim(soldier);
    if (parameters.bullet_style == BulletType::Blade) {
        const auto hands = soldier.skeleton->GetPos(15) - soldier.skeleton->GetPos(16);
        if (glm::dot(hands, hands) > 0.000001F) {
            direction = Calc::Vec2Normalize(hands);
        }
    }
    auto position = soldier.skeleton->GetPos(15) - direction * 4.0F - glm::vec2(0.0F, 2.0F);
    const float deviation =
      0.5F * std::sin(std::min(Inaccuracy(soldier, parameters), 0.5F) * std::numbers::pi_v<float>);
    const float deviation_x = RandomOffset(seed);
    const float deviation_y = RandomOffset(seed);
    direction += glm::vec2(deviation_x, deviation_y) * deviation;
    direction = Calc::Vec2Normalize(direction);
    auto velocity =
      direction * parameters.speed + soldier.particle.GetVelocity() * parameters.inherited_velocity;
    if (parameters.kind == WeaponType::Flamer || parameters.kind == WeaponType::Chainsaw) {
        position += velocity * 2.0F;
    }
    if (parameters.kind == WeaponType::NoWeapon || parameters.kind == WeaponType::Knife) {
        const float facing = static_cast<float>(soldier.direction);
        position = soldier.skeleton->GetPos(16) + glm::vec2(2.0F * facing, 3.0F);
        velocity = glm::vec2(0.1F * facing, 0.0F);
    }
    return { .position = position, .direction = direction, .velocity = velocity };
}

bool FireButtstock(const Soldier& soldier, std::vector<BulletParams>& emitter)
{
    if (soldier.body_animation->GetFrame() != 12 || soldier.body_animation->GetCount() != 0) {
        return false;
    }
    const auto& fists = WeaponParametersFactory::GetParameters(
      WeaponType::NoWeapon, soldier.GetActiveWeapon().GetWeaponParameters().realistic);
    const float facing = static_cast<float>(soldier.direction);
    emitter.push_back({ .style = fists.bullet_style,
                        .weapon = fists.kind,
                        .position = soldier.skeleton->GetPos(16) + glm::vec2(2.0F * facing, 3.0F),
                        .velocity = glm::vec2(0.1F * facing, 0.0F),
                        .timeout = static_cast<std::int16_t>(fists.timeout),
                        .hit_multiply = fists.hit_multiply,
                        .team = soldier.GetTeam(),
                        .owner_id = soldier.id,
                        .push = fists.push,
                        .owner_ping_ticks = soldier.GetPingTicks(),
                        .realistic = fists.realistic });
    return true;
}

void EmitPattern(Soldier& soldier,
                 const WeaponParameters& parameters,
                 BulletParams params,
                 glm::vec2 velocity,
                 glm::vec2 direction,
                 std::uint32_t& seed,
                 std::vector<BulletParams>& emitter)
{
    const auto spread_velocity = [&] {
        const auto x = RandomOffset(seed);
        const auto y = RandomOffset(seed);
        return velocity + glm::vec2(x, y) * parameters.bullet_spread;
    };
    if (parameters.kind == WeaponType::DesertEagles) {
        params.velocity = spread_velocity();
        emitter.push_back(params);
        const auto normal =
          glm::dot(velocity, velocity) > 0.000001F ? Calc::Vec2Normalize(velocity) : direction;
        params.position += glm::vec2(-std::copysign(std::abs(normal.y), velocity.x),
                                     std::copysign(std::abs(normal.x), velocity.y)) *
                           3.0F;
        params.velocity = spread_velocity();
        emitter.push_back(params);
    } else if (parameters.bullet_style == BulletType::GaugeBullet) {
        for (int pellet = 0; pellet < 6; ++pellet) {
            params.velocity = spread_velocity();
            emitter.push_back(params);
        }
        soldier.particle.SetVelocity(soldier.particle.GetVelocity() -
                                     velocity * glm::vec2(0.0412F, 0.041F));
    } else {
        emitter.push_back(params);
    }
    if (parameters.kind == WeaponType::Minigun) {
        auto impulse =
          velocity * (soldier.control.jets && soldier.jets_count > 0 ? glm::vec2(0.0012F, 0.0009F)
                                                                     : glm::vec2(0.0082F, 0.0078F));
        if (soldier.is_holding_flags) {
            impulse *= glm::vec2(0.5F, 0.7F);
        }
        impulse.x *= 0.6F;
        soldier.particle.SetVelocity(soldier.particle.GetVelocity() - impulse);
    }
}
} // namespace Detail

bool Fire(Soldier& soldier, std::vector<BulletParams>& emitter)
{
    auto& weapon = soldier.GetActiveWeapon();
    const auto& parameters = weapon.GetWeaponParameters();
    if (!soldier.IsAlive() || !soldier.CanReceiveCombatImpact() || soldier.mounted_weapon >= 0 ||
        weapon.IsAny({ WeaponType::Cluster,
                       WeaponType::ThrownKnife,
                       WeaponType::FragGrenade,
                       WeaponType::ClusterGrenade,
                       WeaponType::M2 })) {
        return false;
    }
    if (soldier.body_animation->GetType() == AnimationType::Melee) {
        return Detail::FireButtstock(soldier, emitter);
    }
    if (parameters.kind == WeaponType::LAW && !CanFireLaw(soldier)) {
        return false;
    }
    if (weapon.IsMelee() &&
        (soldier.body_animation->GetType() != AnimationType::Punch ||
         soldier.body_animation->GetFrame() != 11 || soldier.body_animation->GetCount() != 0)) {
        return false;
    }
    if (parameters.kind == WeaponType::LAW && weapon.GetStartUpTimeCount() > 0 &&
        !(soldier.on_ground || soldier.on_ground_permanent)) {
        return false;
    }
    if (!weapon.TryFire()) {
        return false;
    }
    std::uint32_t seed = weapon.GetState().instance_id * 277803737U +
                         weapon.GetState().shot_sequence * 747796405U +
                         static_cast<std::uint32_t>(soldier.id) * 2891336453U +
                         static_cast<std::uint32_t>(parameters.kind) + 1U;
    const auto shot_seed = seed;
    const auto vectors = Detail::BuildVectors(soldier, parameters, seed);
    const BulletParams params{ .style = parameters.bullet_style,
                               .weapon = parameters.kind,
                               .position = vectors.position,
                               .velocity = vectors.velocity,
                               .timeout = static_cast<std::int16_t>(parameters.timeout),
                               .hit_multiply = parameters.hit_multiply,
                               .team = soldier.GetTeam(),
                               .owner_id = soldier.id,
                               .push = parameters.push,
                               .owner_ping_ticks = soldier.GetPingTicks(),
                               .seed = shot_seed,
                               .realistic = parameters.realistic };
    Detail::EmitPattern(
      soldier, parameters, params, vectors.velocity, vectors.direction, seed, emitter);
    if (parameters.bink < 0) {
        const auto bink = static_cast<float>(-static_cast<int>(parameters.bink));
        const bool steady = Detail::IsProne(soldier) || Detail::IsCrouched(soldier);
        soldier.AddBink(static_cast<std::uint16_t>(std::lround(bink / (steady ? 2.0F : 1.0F))));
    }
    soldier.fired = 3;
    soldier.is_shooting = true;
    return true;
}

// Advance only the active slot: holstered reloads pause as in the reference.
void Advance(Soldier& soldier)
{
    if (!soldier.control.fire) {
        for (auto& weapon : soldier.weapons) {
            weapon.ReleaseTrigger();
        }
    }
    if (soldier.GetCeaseFireCounter() >= 0) {
        soldier.SetCeaseFireCounter(static_cast<std::int16_t>(soldier.GetCeaseFireCounter() - 1));
    }
    if (soldier.fired > 0) {
        --soldier.fired;
    }
    if (soldier.bonus_ticks > 0) {
        --soldier.bonus_ticks;
    }
    if (soldier.bonus_ticks == 0) {
        soldier.combat_bonus = CombatBonus::None;
    }
    if (soldier.hit_spray > 0) {
        --soldier.hit_spray;
    }
    auto& weapon = soldier.GetActiveWeapon();
    const auto animation = soldier.body_animation->GetType();
    if (animation != AnimationType::Throw && soldier.weapons.size() > 2) {
        soldier.weapons.at(2).ResetThrow();
    }
    const bool interruptible =
      animation != AnimationType::Roll && animation != AnimationType::RollBack &&
      animation != AnimationType::Change && animation != AnimationType::ThrowWeapon &&
      animation != AnimationType::Throw && animation != AnimationType::Melee;
    const bool reload_allowed =
      interruptible || weapon.GetWeaponParameters().kind == WeaponType::Chainsaw;
    if (reload_allowed && weapon.GetWeaponParameters().kind != WeaponType::Spas12 &&
        weapon.GetAmmoCount() == 0 && weapon.CanAutoReload()) {
        weapon.BeginReload();
    }
    weapon.Advance(soldier.control.fire, reload_allowed);
    if (reload_allowed && soldier.control.reload) {
        weapon.RequestReload();
        weapon.BeginReload();
    } else if (reload_allowed && weapon.GetAmmoCount() == 0 && weapon.CanAutoReload()) {
        weapon.BeginReload();
    }
    if (weapon.IsMelee()) {
        weapon.ResetStartUpTimeCount();
    }
}

bool ThrowGrenade(Soldier& soldier, const Map& map, std::vector<BulletParams>& emitter)
{
    if (!soldier.IsAlive() || !soldier.CanReceiveCombatImpact() || soldier.weapons.size() < 3 ||
        soldier.body_animation->GetType() != AnimationType::Throw) {
        return false;
    }
    const auto frame = soldier.body_animation->GetFrame();
    if (frame <= 14 || frame > 36 || (soldier.control.throw_grenade && frame != 36)) {
        return false;
    }
    auto& grenade = soldier.weapons.at(2);
    if (grenade.GetAmmoCount() == 0) {
        return false;
    }
    const auto& parameters = grenade.GetWeaponParameters();
    if (parameters.kind != WeaponType::FragGrenade &&
        parameters.kind != WeaponType::ClusterGrenade) {
        return false;
    }
    auto direction = Aim(soldier);
    const float arc = (direction.x > 0.0F ? 1.0F : -1.0F) / 8.0F * (1.0F - std::abs(direction.y));
    direction += glm::vec2(std::sin(direction.y * std::numbers::pi_v<float> / 2.0F) * arc,
                           -std::sin(direction.x * std::numbers::pi_v<float> / 2.0F) * arc);
    direction = Calc::Vec2Normalize(direction);
    if (parameters.speed <= 0.0F) {
        return false;
    }
    auto velocity =
      direction * (static_cast<float>(frame) / parameters.speed) * (frame < 24 ? 0.65F : 1.0F) +
      soldier.particle.GetVelocity() * parameters.inherited_velocity;
    const auto position = soldier.skeleton->GetPos(15) + velocity * 3.0F - glm::vec2(0.0F, 2.0F);
    glm::vec2 perpendicular{};
    float distance = 0.0F;
    if (map.CollisionTest(position, perpendicular) ||
        map.RayCast(soldier.particle.position - glm::vec2(0.0F, 12.0F),
                    position,
                    distance,
                    50.0F,
                    false,
                    false,
                    true,
                    false,
                    static_cast<int>(soldier.GetTeam()))) {
        return false;
    }
    if (!grenade.ConsumeThrow()) {
        return false;
    }
    if (parameters.bink < 0) {
        soldier.AddBink(static_cast<std::uint16_t>(-static_cast<int>(parameters.bink)));
    }
    emitter.push_back({ parameters.bullet_style,
                        parameters.kind,
                        position,
                        velocity,
                        static_cast<std::int16_t>(parameters.timeout),
                        parameters.hit_multiply,
                        soldier.GetTeam(),
                        soldier.id,
                        parameters.push,
                        soldier.GetPingTicks() });
    emitter.back().realistic = parameters.realistic;
    return true;
}
} // namespace Soldank::WeaponPhysics
