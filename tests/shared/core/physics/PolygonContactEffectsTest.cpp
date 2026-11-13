#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <stdexcept>

import Extern.Glm;
import Shared.Core.World;
import Shared.Core.Entities.Soldier;
import Shared.Core.Entities.Weapon;
import Shared.Core.Map.PMSEnums;
import Shared.Core.Physics.Polygons.PolygonContact;
import Shared.Core.Physics.Polygons.PolygonContactEffects;
import Shared.Core.Combat.HealthEffects;
import Shared.Core.Types.WeaponType;
import Shared.Core.Types.TeamType;
import Testing.Framework.Shared.MapBuilder;

using namespace Soldank;
namespace
{
std::unique_ptr<World> MakeWorld(bool realistic = false)
{
    auto world = std::make_unique<World>();
    world->GetStateManager()->OverrideMap(*SoldankTesting::MapBuilder::Empty()->Build());
    world->GetStateManager()->SetRealistic(realistic);
    world->CreateSoldier(1);
    world->SpawnSoldier(1, glm::vec2{});
    return world;
}
void Apply(World& world,
           PMSPolygonType type,
           std::vector<unsigned int> draws = {},
           PolygonProbe probe = PolygonProbe::Body)
{
    std::size_t index = 0;
    PolygonContactEffects::Apply(
      *world.GetStateManager(),
      world.GetStateManager()->GetSoldier(1),
      { .polygon = 0, .type = type, .position = { 10, 20 }, .velocity = { 2, 4 }, .probe = probe },
      0,
      world.GetPhysicsEvents(),
      [&](unsigned int bound) {
          const auto value = draws.at(index++);
          EXPECT_LT(value, bound);
          return value;
      });
    EXPECT_EQ(index, draws.size());
}
} // namespace
TEST(PolygonContactEffectsTest, LethalTypesUseHealthHitAndArmorInsteadOfTeleportation)
{
    for (bool realistic : { false, true }) {
        for (auto type : { PMSPolygonType::Deadly, PMSPolygonType::BloodyDeadly }) {
            for (bool armored : { false, true }) {
                for (bool protected_actor : { false, true }) {
                    auto world = MakeWorld(realistic);
                    auto& soldier = world->GetStateManager()->GetSoldier(1);
                    const auto initial = soldier.health;
                    soldier.vest = armored ? 100.0F : 0.0F;
                    soldier.combat_bonus =
                      protected_actor ? CombatBonus::FlameGod : CombatBonus::None;
                    Apply(*world, type);
                    EXPECT_EQ(soldier.particle.position, glm::vec2{});
                    if (protected_actor) {
                        EXPECT_EQ(soldier.health, initial);
                        EXPECT_FALSE(soldier.dead_meat);
                    } else if (!armored) {
                        EXPECT_TRUE(soldier.dead_meat);
                        EXPECT_EQ(soldier.health,
                                  type == PMSPolygonType::Deadly ? -50.0F : -400.0F);
                    } else {
                        const float amount =
                          (type == PMSPolygonType::Deadly ? 50.0F : 450.0F) + initial;
                        EXPECT_EQ(soldier.health,
                                  initial - HealthEffects::RoundHealth(amount * 0.25F));
                    }
                }
            }
        }
    }
}
TEST(PolygonContactEffectsTest, HurtBranchesAndCarrierRestrictionsAreExact)
{
    for (auto type : { PMSPolygonType::Hurts, PMSPolygonType::HurtFlaggers }) {
        for (bool carrier : { false, true }) {
            for (unsigned int draw : { 0U, 1U, 9U }) {
                auto world = MakeWorld();
                auto& soldier = world->GetStateManager()->GetSoldier(1);
                soldier.is_holding_flags = carrier;
                soldier.vest = 100;
                soldier.combat_bonus = CombatBonus::FlameGod;
                Apply(*world,
                      type,
                      type == PMSPolygonType::HurtFlaggers && !carrier
                        ? std::vector<unsigned int>{}
                        : std::vector<unsigned int>{ draw });
                const float loss = draw == 0 && (type != PMSPolygonType::HurtFlaggers || carrier)
                                     ? (type == PMSPolygonType::Hurts ? 5.0F : 10.0F)
                                     : 0.0F;
                EXPECT_EQ(soldier.health, 150 - loss);
                EXPECT_EQ(soldier.vest, 100);
            }
        }
    }
    auto world = MakeWorld();
    auto& soldier = world->GetStateManager()->GetSoldier(1);
    soldier.health = 1;
    soldier.vest = 100;
    Apply(*world, PMSPolygonType::Hurts, { 0 });
    EXPECT_TRUE(soldier.dead_meat);
    EXPECT_EQ(soldier.health, -6); // direct -5 then Pascal Round(10*0.25)=2
    EXPECT_EQ(soldier.vest, 97);
}
TEST(PolygonContactEffectsTest, RegenerationUsesGlobalTwelveTickBoundaryAndNeverManufacturesVest)
{
    for (bool realistic : { false, true }) {
        for (unsigned int tick : { 11U, 12U, 13U, 23U, 24U }) {
            auto world = MakeWorld(realistic);
            auto state = world->GetStateManager();
            auto& soldier = state->GetSoldier(1);
            soldier.health = state->GetMaximumHealth() - 1;
            soldier.vest = 10;
            state->SetGameTick(tick);
            Apply(*world, PMSPolygonType::Regenerates);
            EXPECT_EQ(soldier.health, state->GetMaximumHealth() - (tick % 12 == 0 ? 0 : 1));
            EXPECT_EQ(soldier.vest, 10);
        }
    }
}
TEST(PolygonContactEffectsTest, LavaDrawOrderProjectileMetadataAndAuthority)
{
    for (bool realistic : { false, true }) {
        for (auto draws : { std::vector<unsigned int>{ 1, 1 },
                            std::vector<unsigned int>{ 0, 1 },
                            std::vector<unsigned int>{ 1, 0, 0 },
                            std::vector<unsigned int>{ 1, 0, 1 },
                            std::vector<unsigned int>{ 0, 0, 1 },
                            std::vector<unsigned int>{ 0, 0, 0 } }) {
            auto world = MakeWorld(realistic);
            auto state = world->GetStateManager();
            const auto before = state->GetSoldier(1).GetActiveWeapon().GetState();
            Apply(*world, PMSPolygonType::Lava, draws);
            EXPECT_EQ(state->GetBulletEmitter().size(),
                      draws.size() == 3 && draws.back() == 0 ? 1U : 0U);
            if (!state->GetBulletEmitter().empty()) {
                const auto& flame = state->GetBulletEmitter().front();
                EXPECT_EQ(flame.weapon, WeaponType::Flamer);
                EXPECT_EQ(flame.position, glm::vec2(10, 17));
                EXPECT_EQ(flame.velocity, glm::vec2(-2, -4));
                EXPECT_EQ(flame.owner_id, 1);
                EXPECT_EQ(flame.realistic, realistic);
                EXPECT_TRUE(flame.derived);
            }
            EXPECT_EQ(state->GetSoldier(1).GetActiveWeapon().GetState(), before);
        }
    }
    auto world = MakeWorld();
    world->GetStateManager()->SetAuthoritativeProjectiles(false);
    Apply(*world, PMSPolygonType::Lava);
    EXPECT_EQ(world->GetSoldier(1).health, 150);
    EXPECT_TRUE(world->GetStateManager()->GetBulletEmitter().empty());
}
TEST(PolygonContactEffectsTest, ExplosiveCommitsSevereDeathAndOneRealM79)
{
    auto world = MakeWorld(true);
    auto state = world->GetStateManager();
    auto& soldier = state->GetSoldier(1);
    soldier.combat_bonus = CombatBonus::FlameGod;
    std::vector<PolygonEffectEvent> events;
    world->GetPhysicsEvents().polygon_event.AddObserver([&](const auto& event) {
        events.push_back(event);
        EXPECT_LE(soldier.health, 0);
    });
    Apply(*world, PMSPolygonType::Explosive);
    EXPECT_EQ(soldier.health, -600);
    EXPECT_EQ(soldier.death_severity, DeathSeverity::Brutal);
    ASSERT_EQ(state->GetBulletEmitter().size(), 1U);
    EXPECT_EQ(state->GetBulletEmitter().front().weapon, WeaponType::M79);
    EXPECT_EQ(state->GetBulletEmitter().front().velocity, glm::vec2{});
    Apply(*world, PMSPolygonType::Explosive);
    EXPECT_EQ(state->GetBulletEmitter().size(), 1U);
    EXPECT_EQ(events.back().kind, PolygonEventKind::Death);
}
TEST(PolygonContactEffectsTest, LandingAndRandomStreamsAreDeterministic)
{
    auto world = MakeWorld(true);
    auto state = world->GetStateManager();
    Apply(*world, PMSPolygonType::Normal, {}, PolygonProbe::Landing);
    EXPECT_EQ(world->GetSoldier(1).health, 45);
    state->GetSoldier(1).health = 65;
    Apply(*world, PMSPolygonType::Bouncy, {}, PolygonProbe::Landing);
    EXPECT_EQ(world->GetSoldier(1).health, 65);
    PolygonContactEffects::HazardRandom first(42), second(42), other(999);
    for (int i = 0; i < 100; ++i) {
        other.Draw(3);
        EXPECT_EQ(first.Draw(10), second.Draw(10));
    }
}

TEST(PolygonContactEffectsTest, NoEffectsForDeadSpectatorInactiveOrPredictedActors)
{
    for (auto type : { PMSPolygonType::Deadly,
                       PMSPolygonType::BloodyDeadly,
                       PMSPolygonType::Hurts,
                       PMSPolygonType::Regenerates,
                       PMSPolygonType::Lava,
                       PMSPolygonType::Explosive,
                       PMSPolygonType::HurtFlaggers }) {
        for (unsigned int blocked = 0; blocked < 4; ++blocked) {
            auto world = MakeWorld();
            auto state = world->GetStateManager();
            auto& soldier = state->GetSoldier(1);
            soldier.health = 100;
            if (blocked == 0) {
                soldier.dead_meat = true;
            }
            if (blocked == 1) {
                soldier.SetTeam(TeamType::Spectator);
            }
            if (blocked == 2) {
                soldier.active = false;
            }
            if (blocked == 3) {
                state->SetAuthoritativeProjectiles(false);
            }
            PolygonContactEffects::Apply(*state,
                                         soldier,
                                         { .type = type },
                                         0,
                                         world->GetPhysicsEvents(),
                                         [](unsigned int) -> unsigned int {
                                             ADD_FAILURE()
                                               << "A blocked effect must not consume RNG";
                                             return 0;
                                         });
            EXPECT_EQ(soldier.health, 100);
            EXPECT_TRUE(state->GetBulletEmitter().empty());
        }
    }
}

TEST(PolygonContactEffectsTest, PascalRoundingAndProtectedHealingAreExplicit)
{
    EXPECT_EQ(HealthEffects::RoundHealth(2.5F), 2);
    EXPECT_EQ(HealthEffects::RoundHealth(3.5F), 4);
    EXPECT_EQ(HealthEffects::RoundHealth(-2.5F), -2);
    auto world = MakeWorld();
    auto& soldier = world->GetStateManager()->GetSoldier(1);
    soldier.health = 100;
    soldier.combat_bonus = CombatBonus::FlameGod;
    Apply(*world, PMSPolygonType::Regenerates);
    EXPECT_EQ(soldier.health, 100);
    PolygonContactEffects::HazardRandom random(1);
    EXPECT_THROW(random.Draw(0), std::invalid_argument);
}

TEST(PolygonContactEffectsTest, LandingThresholdsModeArmorAndProtectionAreIndependent)
{
    for (bool realistic : { false, true }) {
        for (bool armored : { false, true }) {
            for (bool protected_actor : { false, true }) {
                for (float speed : { -10.0F, 0.0F, 3.49F, 3.5F, 3.51F, 10.0F }) {
                    for (auto type : { PMSPolygonType::Normal, PMSPolygonType::Bouncy }) {
                        auto world = MakeWorld(realistic);
                        auto state = world->GetStateManager();
                        auto& soldier = state->GetSoldier(1);
                        const float initial = soldier.health;
                        soldier.vest = armored ? 100.0F : 0.0F;
                        soldier.combat_bonus =
                          protected_actor ? CombatBonus::FlameGod : CombatBonus::None;
                        PolygonContactEffects::Apply(*state,
                                                     soldier,
                                                     { .type = type,
                                                       .velocity = { 0, speed },
                                                       .probe = PolygonProbe::Landing },
                                                     0,
                                                     world->GetPhysicsEvents(),
                                                     [](unsigned int) {
                                                         ADD_FAILURE()
                                                           << "Landing cannot consume hazard RNG";
                                                         return 0U;
                                                     });
                        const bool damage = realistic && speed > 3.5F &&
                                            type != PMSPolygonType::Bouncy && !protected_actor;
                        const float amount = damage ? speed * 5 : 0;
                        EXPECT_FLOAT_EQ(soldier.health,
                                        initial - (armored
                                                     ? HealthEffects::RoundHealth(amount * 0.25F)
                                                     : amount));
                        EXPECT_FLOAT_EQ(soldier.vest,
                                        armored ? 100 - HealthEffects::RoundHealth(amount * 0.33F)
                                                : 0);
                    }
                }
            }
        }
    }
}
