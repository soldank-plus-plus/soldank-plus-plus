#include "core/CoreEventHandler.hpp"

#include "core/math/Calc.hpp"

#include "core/entities/Bullet.hpp"
#include "core/entities/WeaponParametersFactory.hpp"

#include "core/physics/SoldierPhysics.hpp"

#include "core/types/ItemType.hpp"
#include "spdlog/spdlog.h"
#include <utility>

namespace Soldank
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

void CoreEventHandler::ObserveAll(IWorld* world)
{
    ObserveAllWorldEvents(world);
    ObserveAllPhysicsEvents(world);
}

void CoreEventHandler::ObserveAllWorldEvents(IWorld* world)
{
    world->GetWorldEvents().after_soldier_spawns.AddObserver([](const Soldier& soldier) {
        spdlog::debug("soldier {} spawned at ({}, {})",
                      soldier.id,
                      soldier.particle.position.x,
                      soldier.particle.position.y);
    });
}

void CoreEventHandler::ObserveAllPhysicsEvents(IWorld* world)
{
    world->GetPhysicsEvents().soldier_hit_by_bullet.AddObserver(
      [world](Soldier& soldier, float damage) {
          soldier.health -= damage;
          spdlog::debug("soldier {} hit by {} damage", soldier.id, damage);
          if (soldier.health <= 0.0F) {
              world->KillSoldier(soldier.id);
          }
      });
    world->GetPhysicsEvents().soldier_switches_weapon.AddObserver([world](Soldier& soldier) {
        spdlog::debug(
          "soldier {} switched weapon from {} to {}",
          soldier.id,
          std::to_underlying(soldier.weapons[soldier.active_weapon].GetWeaponParameters().kind),
          std::to_underlying(
            soldier.weapons[(soldier.active_weapon + 1) % 2].GetWeaponParameters().kind));
        world->GetStateManager()->SwitchSoldierWeapon(soldier.id);
    });
    world->GetPhysicsEvents().soldier_throws_active_weapon.AddObserver([world](
                                                                         const Soldier& soldier) {
        if (soldier.weapons[soldier.active_weapon].GetWeaponParameters().kind ==
            WeaponType::Knife) {
            spdlog::debug("soldier {} throws knife", soldier.id);
            Weapon weapon{ WeaponParametersFactory::GetParameters(WeaponType::ThrownKnife, false) };
            auto aim_x = (float)soldier.control.mouse_aim_x;
            auto aim_y = (float)soldier.control.mouse_aim_y;
            auto dir = Calc::Vec2Normalize(glm::vec2(aim_x, aim_y) - soldier.skeleton->GetPos(15));
            auto frame = (float)soldier.body_animation->GetFrame();
            auto thrown_mul = 1.5F * std::min(16.0F, std::max(8.0F, frame)) / 16.0F;
            auto bullet_vel = dir * weapon.GetWeaponParameters().speed * thrown_mul;
            auto inherited_vel =
              soldier.particle.GetVelocity() * weapon.GetWeaponParameters().inherited_velocity;
            auto velocity = bullet_vel + inherited_vel;

            BulletParams params{
                weapon.GetWeaponParameters().bullet_style,
                weapon.GetWeaponParameters().kind,
                soldier.skeleton->GetPos(16) + velocity,
                velocity,
                (std::int16_t)weapon.GetWeaponParameters().timeout,
                weapon.GetWeaponParameters().hit_multiply,
                TeamType::None,
                soldier.id,
                0.0F // TODO: Add push
            };
            world->GetStateManager()->EnqueueNewProjectile(params);
            world->GetStateManager()->ChangeSoldierPrimaryWeapon(soldier.id, WeaponType::NoWeapon);
        } else {
            spdlog::debug("soldier {} throws a weapon", soldier.id);
            WeaponType current_soldier_weapon_type =
              soldier.weapons[soldier.active_weapon].GetWeaponParameters().kind;
            Item& new_item = world->GetStateManager()->CreateItem(
              soldier.skeleton->GetPos(16),
              soldier.id,
              WeaponTypeToItemType(current_soldier_weapon_type));
            world->GetStateManager()->ChangeSoldierPrimaryWeapon(soldier.id, WeaponType::NoWeapon);
            new_item.skeleton->SetPos(
              1, new_item.skeleton->GetPos(1) + soldier.particle.GetVelocity());
            new_item.skeleton->SetPos(
              2, new_item.skeleton->GetPos(2) + soldier.particle.GetVelocity());

            glm::vec2 aim_direction = { soldier.control.mouse_aim_x, soldier.control.mouse_aim_y };
            aim_direction -= soldier.skeleton->GetPos(15);
            aim_direction = Calc::Vec2Normalize(aim_direction);

            float weapon_throw_speed_pos1 = 0.01F;
            float weapon_throw_speed_pos2 = 3.0F;
            if (soldier.dead_meat) {
                weapon_throw_speed_pos1 = 0.02F;
                weapon_throw_speed_pos2 = 0.64F;
            }

            glm::vec2 weapon_throw_velocity =
              Calc::Vec2Scale(aim_direction, weapon_throw_speed_pos1);
            new_item.skeleton->SetPos(1, new_item.skeleton->GetPos(1) + weapon_throw_velocity);
            weapon_throw_velocity = Calc::Vec2Scale(aim_direction, weapon_throw_speed_pos2);
            new_item.skeleton->SetPos(2, new_item.skeleton->GetPos(2) + weapon_throw_velocity);
        }
    });
    world->GetPhysicsEvents().soldier_fires_primary_weapon.AddObserver([world](Soldier& soldier) {
        // std::vector<BulletParams> bullet_emitter;
        // SoldierPhysics::Fire(soldier, bullet_emitter);
        // for (auto& bullet_params : bullet_emitter) {
        //     world->GetStateManager()->EnqueueNewProjectile(bullet_params);
        // }
    });
    world->GetPhysicsEvents().soldier_collides_with_item.AddObserver(
      [world](Soldier& soldier, Item& item) {
          if (IsItemTypeFlag(item.style)) {
              if (item.holding_soldier_id == 0) {
                  item.static_type = false;
                  item.holding_soldier_id = soldier.id;
                  soldier.is_holding_flags = true;
              }
          } else if (IsItemTypeWeapon(item.style)) {
              if (soldier.weapons[soldier.active_weapon].GetWeaponParameters().kind ==
                  WeaponType::NoWeapon) {
                  world->GetStateManager()->SoldierPickupWeapon(soldier.id, item);
                  item.active = false;
              }
          } else if (IsItemTypeKit(item.style)) {
              world->GetStateManager()->SoldierPickupKit(soldier.id, item.id);
          }
      });
    world->GetPhysicsEvents().soldier_throws_flags.AddObserver([world](Soldier& soldier) {
        spdlog::info("PLAYER {} THROWS FLAGS", soldier.id);
        world->GetStateManager()->ThrowSoldierFlags(soldier.id);
    });
    world->GetPhysicsEvents().bullet_collides_with_polygon.AddObserver(
      [world](const Bullet& bullet, const glm::vec2& collision_position) {
          if (bullet.style == BulletType::ThrownKnife) {
              glm::vec2 knife_as_item_position = collision_position;
              knife_as_item_position -= bullet.particle.GetVelocity() * 2.0F;

              world->GetStateManager()->CreateItem(knife_as_item_position, 0, ItemType::Knife);
          }
      });
}
} // namespace Soldank
