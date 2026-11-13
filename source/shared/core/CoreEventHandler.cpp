module;

#include "spdlog/spdlog.h"

#include <cstdint>
#include <utility>
#include <vector>

export module Shared.CoreEventHandler;

import Extern.Glm;

import Shared.Core.IWorld;
import Shared.Core.WorldEvents;
import Shared.Core.Physics.SoldierPhysics;
import Shared.Core.Physics.PhysicsEvents;
import Shared.Core.Math.Calc;
import Shared.Core.Entities.Bullet;
import Shared.Core.Entities.Item;
import Shared.Core.Entities.Soldier;
import Shared.Core.Entities.Weapon;
import Shared.Core.Animations;
import Shared.Core.Entities.WeaponParametersFactory;
import Shared.Core.Types.ItemType;
import Shared.Core.Types.TeamType;
import Shared.Core.Types.WeaponType;

namespace Soldank
{
export class CoreEventHandler
{
public:
    static void ObserveAll(IWorld* world)
    {
        ObserveAllWorldEvents(world);
        ObserveAllPhysicsEvents(world);
    }

private:
    static void ObserveAllWorldEvents(IWorld* world)
    {
        world->GetWorldEvents().after_soldier_spawns.AddObserver([](const Soldier& soldier) {
            spdlog::debug("soldier {} spawned at ({}, {})",
                          soldier.id,
                          soldier.particle.position.x,
                          soldier.particle.position.y);
        });
    }

    static void ObserveAllPhysicsEvents(IWorld* world)
    {
        world->GetPhysicsEvents().soldier_switches_weapon.AddObserver([world](Soldier& soldier) {
            spdlog::debug(
              "soldier {} switched weapon from {} to {}",
              soldier.id,
              std::to_underlying(soldier.weapons[soldier.active_weapon].GetWeaponParameters().kind),
              std::to_underlying(
                soldier.weapons[(soldier.active_weapon + 1) % 2].GetWeaponParameters().kind));
        });
        world->GetPhysicsEvents().soldier_collides_with_item.AddObserver([world](Soldier& soldier,
                                                                                 Item& item) {
            if (!world->GetStateManager()->HasAuthoritativeProjectiles()) {
                return;
            }
            if (IsItemTypeFlag(item.style)) {
                if (item.holding_soldier_id == 0) {
                    item.static_type = false;
                    item.holding_soldier_id = soldier.id;
                    soldier.is_holding_flags = true;
                }
            } else if (IsItemTypeWeapon(item.style)) {
                const auto cooldown = item.style == ItemType::Bow ? 1500 - 100 : 1200 - 30;
                if (soldier.GetActiveWeapon().GetWeaponParameters().kind == WeaponType::NoWeapon &&
                    soldier.body_animation->GetType() != AnimationType::Change &&
                    item.time_out < cooldown && !soldier.IsSpectator()) {
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
    }
};
} // namespace Soldank
