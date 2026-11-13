module;

#include <algorithm>
#include <cmath>

export module Shared.Core.Combat.HealthEffects;

import Shared.Core.Entities.Soldier;
import Shared.Core.Entities.Weapon;
import Shared.Core.Physics.Polygons.PolygonContact;

export namespace Soldank::HealthEffects
{
void Finalize(Soldier& soldier)
{
    if (soldier.health < 1.0F) {
        soldier.health = std::max(-400.0F, soldier.health);
        soldier.dead_meat = true;
        soldier.ticks_to_respawn = 180;
        soldier.death_severity = DeathSeverity::Normal;
        if (soldier.health <= -400.0F) {
            soldier.death_severity = DeathSeverity::Brutal;
        } else if (soldier.health <= -90.0F) {
            soldier.death_severity = DeathSeverity::Head;
        }
    }
}
float RoundHealth(float value)
{
    const float lower = std::floor(value);
    const float fraction = value - lower;
    return lower + static_cast<float>(fraction > 0.5F ||
                                      (fraction == 0.5F && std::fmod(lower, 2.0F) != 0.0F));
}
// Environmental self hits do not inherit projectile ceasefire, team or self-berserk rules.
float Apply(Soldier& soldier,
            float amount,
            float maximum_health,
            bool direct = false,
            bool finalize = true)
{
    if (!soldier.IsAlive() || soldier.IsSpectator() || !std::isfinite(amount)) {
        return 0.0F;
    }
    if (!direct && soldier.combat_bonus == CombatBonus::FlameGod) {
        return 0.0F;
    }
    const auto before = soldier.health;
    if (amount < 0.0F) {
        // Intentional safety adaptation: healing never manufactures vest or consumes armor.
        soldier.health = std::min(maximum_health, soldier.health - amount);
    } else {
        if (!direct && soldier.vest > 0.0F) {
            soldier.vest = std::max(0.0F, soldier.vest - RoundHealth(amount * 0.33F));
            amount = RoundHealth(amount * 0.25F);
        }
        soldier.health -= amount;
    }
    if (finalize) {
        Finalize(soldier);
    }
    return before - soldier.health;
}
} // namespace Soldank::HealthEffects
