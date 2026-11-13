module;

#include <cmath>
#include <cstdint>

export module Shared.Core.Physics.Weapons.StationaryWeaponPhysics;

import Extern.Glm;
import Shared.Core.Entities.Item;
import Shared.Core.Entities.Soldier;
import Shared.Core.Entities.WeaponParametersFactory;
import Shared.Core.Entities.Bullet;
import Shared.Core.State.StateManager;
import Shared.Core.Types.WeaponType;
import Shared.Core.Animations;
import Shared.Core.Math.Calc;

export namespace Soldank::StationaryWeaponPhysics
{
void Update(StateManager& state, Item& item)
{
    if (!state.HasAuthoritativeProjectiles() || item.time_out > 0) {
        return;
    }
    Soldier* operator_soldier = nullptr;
    state.TransformSoldiers([&](Soldier& soldier) {
        const bool in_range =
          soldier.IsAlive() && !soldier.IsSpectator() &&
          glm::distance(soldier.particle.position, item.skeleton->GetPos(1)) < item.radius;
        if (soldier.mounted_weapon == item.id) {
            if (!in_range || soldier.control.up || soldier.control.jets) {
                soldier.mounted_weapon = -1;
            } else {
                operator_soldier = &soldier;
            }
        }
    });
    if (operator_soldier == nullptr) {
        item.mounted_soldier = -1;
        state.TransformSoldiers([&](Soldier& soldier) {
            if (operator_soldier == nullptr && soldier.IsAlive() && !soldier.IsSpectator() &&
                soldier.mounted_weapon < 0 && !soldier.control.up && !soldier.control.jets &&
                soldier.legs_animation->GetType() == AnimationType::Stand &&
                glm::distance(soldier.particle.position, item.skeleton->GetPos(1)) < item.radius) {
                soldier.mounted_weapon = item.id;
                operator_soldier = &soldier;
            }
        });
    }
    if ((operator_soldier == nullptr || !operator_soldier->control.fire) && item.interest > 0 &&
        state.GetGameTick() % 8U == 0) {
        --item.interest;
    }
    if (operator_soldier == nullptr) {
        return;
    }
    auto& soldier = *operator_soldier;
    item.mounted_soldier = soldier.id;
    item.static_type = true;
    auto direction = glm::vec2(soldier.control.mouse_aim_x, soldier.control.mouse_aim_y) -
                     soldier.skeleton->GetPos(15);
    if (glm::dot(direction, direction) < 0.000001F) {
        return;
    }
    direction = Calc::Vec2Normalize(direction);
    const auto muzzle = item.skeleton->GetPos(1) + glm::vec2(-direction.x, direction.y) * 3.0F;
    item.skeleton->SetOldPos(4, item.skeleton->GetPos(4));
    item.skeleton->SetPos(4, muzzle);
    const auto& parameters =
      WeaponParametersFactory::GetParameters(WeaponType::M2, state.IsRealistic());
    if (!soldier.control.fire || !soldier.CanReceiveCombatImpact() ||
        soldier.legs_animation->GetType() != AnimationType::Stand ||
        parameters.fire_interval == 0 || state.GetGameTick() % parameters.fire_interval != 0 ||
        item.interest > 18) {
        return;
    }
    glm::vec2 velocity = direction * parameters.speed;
    if (item.interest > 4) {
        const int deviation = item.interest / 11;
        const float offset = deviation == 0
                               ? 0.0F
                               : static_cast<float>(state.GetRandomInt(0, 2 * deviation - 1)) -
                                   static_cast<float>(deviation);
        velocity += glm::vec2(offset);
    }
    state.EnqueueNewProjectile({ parameters.bullet_style,
                                 parameters.kind,
                                 muzzle + glm::vec2(4.0F, -10.0F),
                                 velocity,
                                 static_cast<std::int16_t>(parameters.timeout),
                                 parameters.hit_multiply,
                                 soldier.GetTeam(),
                                 soldier.id,
                                 parameters.push,
                                 soldier.GetPingTicks(),
                                 0,
                                 0,
                                 0,
                                 false,
                                 parameters.realistic });
    ++item.interest;
}
} // namespace Soldank::StationaryWeaponPhysics
