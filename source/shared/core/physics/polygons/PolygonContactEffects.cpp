module;

#include <cstdint>
#include <functional>
#include <stdexcept>
#include <algorithm>
#include <cmath>

export module Shared.Core.Physics.Polygons.PolygonContactEffects;

import Shared.Core.Physics.Polygons.PolygonContact;
import Shared.Core.Map.Polygons.PolygonTraits;
import Shared.Core.Combat.HealthEffects;
import Shared.Core.Entities.Soldier;
import Shared.Core.Entities.WeaponParametersFactory;
import Shared.Core.Entities.Bullet;
import Shared.Core.State.StateManager;
import Shared.Core.Physics.PhysicsEvents;
import Shared.Core.Types.WeaponType;
import Shared.Core.Map.PMSEnums;
import Extern.Glm;

export namespace Soldank::PolygonContactEffects
{
class HazardRandom
{
public:
    explicit HazardRandom(std::uint32_t seed)
        : state_(seed)
    {
    }
    unsigned int Draw(unsigned int bound)
    {
        if (bound == 0) {
            throw std::invalid_argument("Hazard RNG bound must be positive");
        }
        state_ = (state_ * 1664525U) + 1013904223U;
        return (state_ >> 8U) % bound;
    }

private:
    std::uint32_t state_;
};
void Apply(StateManager& state,
           Soldier& soldier,
           const PolygonContact& contact,
           unsigned int ordinal,
           const PhysicsEvents& events,
           const std::function<unsigned int(unsigned int)>& draw)
{
    if (!state.HasAuthoritativeProjectiles() || !soldier.IsAlive() || soldier.IsSpectator()) {
        return;
    }
    const auto emit = [&](PolygonEventKind kind, float amount = 0.0F) {
        events.polygon_event.Notify({
          .kind = kind,
          .soldier = soldier.id,
          .tick = state.GetGameTick(),
          .life = soldier.life_sequence,
          .polygon = contact.polygon,
          .ordinal = ordinal,
          .position = contact.position,
          .amount = amount,
          .severity = soldier.death_severity,
          .map_fingerprint = state.GetConstMap().GetPolygonFingerprint(),
        });
    };
    const auto spawn = [&](WeaponType kind, glm::vec2 velocity) {
        const auto& weapon = WeaponParametersFactory::GetParameters(kind, state.IsRealistic());
        state.EnqueueNewProjectile({ .style = weapon.bullet_style,
                                     .weapon = kind,
                                     .position = contact.position - glm::vec2(0.0F, 3.0F),
                                     .velocity = velocity,
                                     .timeout = static_cast<std::int16_t>(weapon.timeout),
                                     .hit_multiply = weapon.hit_multiply,
                                     .team = soldier.GetTeam(),
                                     .owner_id = soldier.id,
                                     .push = weapon.push,
                                     .seed = soldier.life_sequence * 747796405U +
                                             state.GetGameTick() * 2891336453U + contact.polygon +
                                             ordinal + 1U,
                                     .derived = true,
                                     .realistic = state.IsRealistic() });
    };
    if (contact.probe == PolygonProbe::Bounce) {
        emit(PolygonEventKind::Bounce, glm::length(contact.velocity));
        return;
    }
    const auto direct_loss = [&](float amount) {
        HealthEffects::Apply(soldier, amount, state.GetMaximumHealth(), true, false);
        if (soldier.health < 1.0F) {
            HealthEffects::Apply(soldier, 10.0F, state.GetMaximumHealth());
            // Direct environmental loss cannot leave an active negative-health actor.
            HealthEffects::Finalize(soldier);
        }
    };
    const auto before = soldier.health;
    const auto maximum = state.GetMaximumHealth();
    if (contact.probe == PolygonProbe::Landing) {
        if (state.IsRealistic() && contact.type != PMSPolygonType::Bouncy &&
            contact.velocity.y > 3.5F) {
            HealthEffects::Apply(soldier, contact.velocity.y * 5.0F, maximum);
        }
        emit(PolygonEventKind::Landing, std::abs(contact.velocity.y));
    } else {
        switch (GetPolygonTraits(contact.type).effect) {
            case PolygonEffect::Deadly:
                HealthEffects::Apply(soldier, 50.0F + soldier.health, maximum);
                break;
            case PolygonEffect::BloodyDeadly:
                HealthEffects::Apply(soldier, 450.0F + soldier.health, maximum);
                break;
            case PolygonEffect::Hurts:
            case PolygonEffect::Lava:
                if (draw(10) == 0) {
                    direct_loss(5.0F);
                }
                if (contact.type == PMSPolygonType::Lava && draw(3) == 0) {
                    emit(PolygonEventKind::Lava);
                    if (draw(3) == 0) {
                        spawn(WeaponType::Flamer, -contact.velocity);
                    }
                }
                break;
            case PolygonEffect::Regenerates:
                if (soldier.health < maximum && state.GetGameTick() % 12U == 0) {
                    HealthEffects::Apply(soldier, -2.0F, maximum);
                }
                break;
            case PolygonEffect::Explosive:
                spawn(WeaponType::M79, {});
                HealthEffects::Apply(soldier, 4000.0F, maximum);
                soldier.health = -600.0F;
                soldier.dead_meat = true;
                soldier.ticks_to_respawn = 180;
                soldier.death_severity = DeathSeverity::Brutal;
                break;
            case PolygonEffect::HurtFlaggers:
                if (soldier.is_holding_flags && draw(10) == 0) {
                    direct_loss(10.0F);
                }
                break;
            default:
                break;
        }
    }
    if (soldier.health < before) {
        emit(PolygonEventKind::Hurt, before - soldier.health);
    }
    if (soldier.health > before) {
        emit(PolygonEventKind::Heal, soldier.health - before);
    }
    if (soldier.dead_meat) {
        emit(PolygonEventKind::Death);
    }
}
void ApplyAll(StateManager& state, Soldier& soldier, const PhysicsEvents& events)
{
    unsigned int ordinal = 0;
    for (const auto& contact : soldier.polygon_contacts) {
        HazardRandom random(soldier.life_sequence * 277803737U + soldier.id * 747796405U +
                            state.GetGameTick() * 2891336453U + contact.polygon * 31U + ordinal);
        Apply(state, soldier, contact, ordinal++, events, [&](unsigned int bound) {
            return random.Draw(bound);
        });
    }
    soldier.polygon_contacts.clear();
}
} // namespace Soldank::PolygonContactEffects
