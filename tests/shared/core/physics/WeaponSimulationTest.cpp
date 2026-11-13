#include "core/math/Glm.hpp"
#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <vector>

import Shared.Core.World;
import Shared.Core.Entities.Soldier;
import Shared.Core.Entities.Item;
import Shared.Core.Entities.Weapon;
import Shared.Core.Entities.WeaponParametersFactory;
import Shared.Core.Physics.SoldierPhysics;
import Shared.Core.Physics.PhysicsEvents;
import Shared.Core.Physics.Weapons.WeaponPhysics;
import Shared.Core.Physics.Weapons.StationaryWeaponPhysics;
import Shared.Core.Types.WeaponType;
import Shared.Core.Types.ItemType;
import Shared.Core.Map.PMSEnums;
import Shared.Core.Animations;
import Testing.Framework.Shared.MapBuilder;

using namespace Soldank;

namespace
{
std::unique_ptr<World> CreateWeaponWorld(bool authoritative = true, bool realistic = false)
{
    auto world = std::make_unique<World>();
    const auto map = SoldankTesting::MapBuilder::Empty()->Build();
    world->GetStateManager()->OverrideMap(*map);
    world->GetStateManager()->SetAuthoritativeProjectiles(authoritative);
    world->GetStateManager()->SetRealistic(realistic);
    world->CreateSoldier(1);
    world->SpawnSoldier(1, glm::vec2{});
    return world;
}

void Step(World& world, unsigned int tick)
{
    world.GetStateManager()->SetGameTick(tick);
    world.UpdateSoldier(1);
}
} // namespace

TEST(WeaponSimulationTest, GunsFireThroughTheSharedTickWithoutGameplayObservers)
{
    for (const bool realistic : { false, true }) {
        for (const auto kind : { WeaponType::DesertEagles,
                                 WeaponType::MP5,
                                 WeaponType::Ak74,
                                 WeaponType::SteyrAUG,
                                 WeaponType::Spas12,
                                 WeaponType::Ruger77,
                                 WeaponType::M79,
                                 WeaponType::Barrett,
                                 WeaponType::Minimi,
                                 WeaponType::Minigun,
                                 WeaponType::USSOCOM,
                                 WeaponType::Chainsaw,
                                 WeaponType::Bow,
                                 WeaponType::FlameBow,
                                 WeaponType::Flamer }) {
            SCOPED_TRACE(static_cast<int>(kind));
            SCOPED_TRACE(realistic);
            auto world = CreateWeaponWorld(true, realistic);
            auto state = world->GetStateManager();
            state->ChangeSoldierPrimaryWeapon(1, kind);
            auto& soldier = state->GetSoldier(1);
            soldier.control.fire = true;
            soldier.control.mouse_aim_x = 1000;
            const auto& parameters = soldier.GetActiveWeapon().GetWeaponParameters();
            const unsigned int limit =
              parameters.reload_time + parameters.fire_interval + parameters.start_up_time + 30U;
            for (unsigned int tick = 0; tick < limit && state->GetBulletEmitter().empty(); ++tick) {
                Step(*world, tick);
            }
            const auto expected =
              kind == WeaponType::DesertEagles ? 2U : (kind == WeaponType::Spas12 ? 6U : 1U);
            ASSERT_EQ(state->GetBulletEmitter().size(), expected);
            EXPECT_EQ(soldier.GetActiveWeapon().GetState().shot_sequence, 1U);
            EXPECT_EQ(soldier.GetActiveWeapon().GetAmmoCount(), parameters.ammo - 1);
            for (const auto& bullet : state->GetBulletEmitter()) {
                EXPECT_EQ(bullet.realistic, realistic);
            }
        }
    }
}

TEST(WeaponSimulationTest, PredictionSpendsAmmoWithoutCreatingAuthoritativeProjectiles)
{
    auto world = CreateWeaponWorld(false);
    auto state = world->GetStateManager();
    state->ChangeSoldierPrimaryWeapon(1, WeaponType::MP5);
    auto& soldier = state->GetSoldier(1);
    soldier.control.fire = true;
    soldier.control.mouse_aim_x = 1000;
    const auto capacity = soldier.GetActiveWeapon().GetAmmoCount();
    const auto interval = soldier.GetActiveWeapon().GetWeaponParameters().fire_interval;
    for (unsigned int tick = 0; tick < interval; ++tick) {
        Step(*world, tick);
    }
    EXPECT_EQ(soldier.GetActiveWeapon().GetAmmoCount(), capacity - 1);
    EXPECT_TRUE(state->GetBulletEmitter().empty());
}

TEST(WeaponSimulationTest, ManualReloadRestoresFullCapacityThroughAnimationPhases)
{
    auto world = CreateWeaponWorld();
    auto state = world->GetStateManager();
    state->ChangeSoldierPrimaryWeapon(1, WeaponType::MP5);
    auto& soldier = state->GetSoldier(1);
    auto partial = soldier.GetActiveWeapon().GetState();
    partial.ammo = 2;
    soldier.GetActiveWeapon().RestoreState(partial);
    soldier.control.reload = true;
    Step(*world, 0);
    EXPECT_TRUE(soldier.GetActiveWeapon().IsReloading());
    EXPECT_EQ(soldier.body_animation->GetType(), AnimationType::ClipOut);
    soldier.control.reload = false;
    const auto reload_time = soldier.GetActiveWeapon().GetWeaponParameters().reload_time;
    for (unsigned int tick = 1; tick <= reload_time; ++tick) {
        Step(*world, tick);
    }
    EXPECT_FALSE(soldier.GetActiveWeapon().IsReloading());
    EXPECT_EQ(soldier.GetActiveWeapon().GetAmmoCount(),
              soldier.GetActiveWeapon().GetWeaponParameters().ammo);
}

TEST(WeaponSimulationTest, KnifeContactsOnlyOnceAtPunchFrameEleven)
{
    auto world = CreateWeaponWorld();
    auto state = world->GetStateManager();
    state->ChangeSoldierPrimaryWeapon(1, WeaponType::Knife);
    auto& soldier = state->GetSoldier(1);
    soldier.control.fire = true;
    soldier.control.mouse_aim_x = 100;
    Step(*world, 0);
    EXPECT_TRUE(state->GetBulletEmitter().empty());
    soldier.control.fire = false;
    for (unsigned int tick = 1; tick < 20; ++tick) {
        Step(*world, tick);
    }
    EXPECT_EQ(state->GetBulletEmitter().size(), 1U);
    EXPECT_EQ(soldier.GetActiveWeapon().GetWeaponParameters().kind, WeaponType::Knife);
}

TEST(WeaponSimulationTest, FlamerBonusPreservesPreviousPrimaryInSecondaryAndExpires)
{
    auto world = CreateWeaponWorld();
    auto state = world->GetStateManager();
    state->ChangeSoldierPrimaryWeapon(1, WeaponType::MP5);
    const auto item_id = state->CreateItem({}, 255, ItemType::FlamerKit).id;
    state->SoldierPickupKit(1, item_id);
    auto& soldier = state->GetSoldier(1);
    EXPECT_EQ(soldier.GetActiveWeapon().GetWeaponParameters().kind, WeaponType::Flamer);
    EXPECT_EQ(soldier.weapons[1].GetWeaponParameters().kind, WeaponType::MP5);
    state->SwitchSoldierWeapon(1);
    EXPECT_EQ(soldier.active_weapon, 0);
    for (unsigned int tick = 0; tick < 600; ++tick) {
        Step(*world, tick);
    }
    EXPECT_EQ(soldier.combat_bonus, CombatBonus::None);
    state->SwitchSoldierWeapon(1);
    EXPECT_EQ(soldier.GetActiveWeapon().GetWeaponParameters().kind, WeaponType::MP5);
}

TEST(WeaponSimulationTest, HeldFireUsesExactReferenceIntervalAfterStartup)
{
    for (const bool realistic : { false, true }) {
        for (const auto kind : { WeaponType::MP5, WeaponType::Minigun }) {
            auto world = CreateWeaponWorld();
            auto state = world->GetStateManager();
            const auto& parameters = WeaponParametersFactory::GetParameters(kind, realistic);
            state->GetSoldier(1).GetActiveWeapon() = parameters;
            auto& soldier = state->GetSoldier(1);
            soldier.control.fire = true;
            soldier.control.mouse_aim_x = 1000;
            const unsigned int first_tick =
              parameters.fire_interval - 1U + parameters.start_up_time;
            for (unsigned int tick = 0; tick < first_tick; ++tick) {
                Step(*world, tick);
                EXPECT_EQ(soldier.GetActiveWeapon().GetState().shot_sequence, 0U);
            }
            Step(*world, first_tick);
            EXPECT_EQ(soldier.GetActiveWeapon().GetState().shot_sequence, 1U);
            for (unsigned int tick = first_tick + 1; tick < first_tick + parameters.fire_interval;
                 ++tick) {
                Step(*world, tick);
                EXPECT_EQ(soldier.GetActiveWeapon().GetState().shot_sequence, 1U);
            }
            Step(*world, first_tick + parameters.fire_interval);
            EXPECT_EQ(soldier.GetActiveWeapon().GetState().shot_sequence, 2U);
        }
    }
}

TEST(WeaponSimulationTest, SingleShotWeaponsRequireReleaseAndPreserveLatchAcrossRestore)
{
    for (const bool realistic : { false, true }) {
        for (const auto kind : { WeaponType::DesertEagles,
                                 WeaponType::USSOCOM,
                                 WeaponType::Spas12,
                                 WeaponType::Ruger77,
                                 WeaponType::Barrett }) {
            SCOPED_TRACE(static_cast<int>(kind));
            SCOPED_TRACE(realistic);
            auto world = CreateWeaponWorld(true, realistic);
            auto state = world->GetStateManager();
            state->ChangeSoldierPrimaryWeapon(1, kind);
            auto& soldier = state->GetSoldier(1);
            soldier.control.fire = true;
            soldier.control.mouse_aim_x = 1000;
            const auto parameters = soldier.GetActiveWeapon().GetWeaponParameters();
            const unsigned int first_tick =
              parameters.fire_interval - 1U + parameters.start_up_time;
            unsigned int tick = 0;
            for (; tick <= first_tick + parameters.fire_interval * 3U; ++tick) {
                Step(*world, tick);
            }
            ASSERT_EQ(soldier.GetActiveWeapon().GetState().shot_sequence, 1U);
            EXPECT_EQ(soldier.GetActiveWeapon().GetAmmoCount(), parameters.ammo - 1);
            Weapon restored(parameters);
            ASSERT_TRUE(restored.RestoreState(soldier.GetActiveWeapon().GetState()));
            restored.Advance(true);
            EXPECT_FALSE(restored.TryFire());
            // A completed reload must not turn a held trigger into a new click.
            ASSERT_TRUE(restored.BeginReload());
            for (unsigned int frame = 0; frame < parameters.reload_time; ++frame) {
                restored.Advance(true);
                restored.InsertShell();
            }
            for (unsigned int frame = 0; frame <= parameters.fire_interval; ++frame) {
                restored.Advance(true);
            }
            EXPECT_FALSE(restored.TryFire());
            soldier.control.fire = false;
            Step(*world, tick++);
            soldier.control.fire = true;
            for (unsigned int frame = 0; frame <= parameters.start_up_time; ++frame) {
                Step(*world, tick++);
            }
            EXPECT_EQ(soldier.GetActiveWeapon().GetState().shot_sequence, 2U);
            EXPECT_EQ(soldier.GetActiveWeapon().GetAmmoCount(), parameters.ammo - 2);
        }
    }
}

TEST(WeaponSimulationTest, TriggerReleaseAlsoRearmsHolsteredWeapons)
{
    auto world = CreateWeaponWorld();
    auto state = world->GetStateManager();
    state->ChangeSoldierPrimaryWeapon(1, WeaponType::USSOCOM);
    auto& soldier = state->GetSoldier(1);
    auto ready = soldier.GetActiveWeapon().GetState();
    ready.fire_interval = 0;
    ASSERT_TRUE(soldier.GetActiveWeapon().RestoreState(ready));
    soldier.control.fire = true;
    soldier.control.mouse_aim_x = 1000;
    Step(*world, 0);
    ASSERT_TRUE(soldier.weapons[0].GetState().trigger_latched);
    state->SwitchSoldierWeapon(1);
    ASSERT_EQ(soldier.active_weapon, 1);
    soldier.control.fire = false;
    Step(*world, 1);
    EXPECT_FALSE(soldier.weapons[0].GetState().trigger_latched);
}

TEST(WeaponSimulationTest, SpasWaitsForReleaseBeforeReloadingAnEmptyMagazine)
{
    auto world = CreateWeaponWorld();
    auto state = world->GetStateManager();
    state->ChangeSoldierPrimaryWeapon(1, WeaponType::Spas12);
    auto& soldier = state->GetSoldier(1);
    auto ready = soldier.GetActiveWeapon().GetState();
    ready.ammo = 1;
    ready.fire_interval = 0;
    soldier.GetActiveWeapon().RestoreState(ready);
    soldier.control.fire = true;
    soldier.control.mouse_aim_x = 1000;
    for (unsigned int tick = 0; tick < 100; ++tick) {
        Step(*world, tick);
    }
    EXPECT_EQ(soldier.GetActiveWeapon().GetAmmoCount(), 0);
    EXPECT_FALSE(soldier.GetActiveWeapon().IsReloading());
    soldier.control.fire = false;
    for (unsigned int tick = 100; tick < 140; ++tick) {
        Step(*world, tick);
    }
    EXPECT_GE(soldier.GetActiveWeapon().GetAmmoCount(), 1);
}

TEST(WeaponSimulationTest, GroundedCrouchingLawCompletesStartupAndFiresOneMissile)
{
    auto world = CreateWeaponWorld();
    auto state = world->GetStateManager();
    const auto floor =
      SoldankTesting::MapBuilder::Empty()
        ->AddPolygon(
          { -4000.0F, 0.0F }, { 4000.0F, 0.0F }, { 4000.0F, 60.0F }, PMSPolygonType::Normal)
        ->AddPolygon(
          { -4000.0F, 0.0F }, { 4000.0F, 60.0F }, { -4000.0F, 60.0F }, PMSPolygonType::Normal)
        ->Build();
    state->OverrideMap(*floor);
    state->SetSoldierPosition(1, { 0.0F, -34.0F });
    state->ChangeSoldierPrimaryWeapon(1, WeaponType::LAW);
    auto& soldier = state->GetSoldier(1);
    soldier.control.down = true;
    soldier.control.fire = true;
    soldier.control.mouse_aim_x = 1000;
    for (unsigned int tick = 0; tick < 200 && state->GetBulletEmitter().empty(); ++tick) {
        Step(*world, tick);
    }
    ASSERT_EQ(state->GetBulletEmitter().size(), 1U);
    EXPECT_EQ(state->GetBulletEmitter().front().weapon, WeaponType::LAW);
    EXPECT_EQ(soldier.GetActiveWeapon().GetAmmoCount(), 0);
}

TEST(WeaponSimulationTest, AcceptedShotEventsAreImmutableValueSnapshots)
{
    auto world = CreateWeaponWorld();
    auto state = world->GetStateManager();
    state->ChangeSoldierPrimaryWeapon(1, WeaponType::MP5);
    auto& soldier = state->GetSoldier(1);
    soldier.control.fire = true;
    soldier.control.mouse_aim_x = 1000;
    std::vector<WeaponActionEvent> accepted;
    world->GetPhysicsEvents().weapon_action_event.AddObserver([&](const WeaponActionEvent& event) {
        if (event.kind == WeaponActionKind::Fired) {
            accepted.push_back(event);
        }
    });
    const auto interval = soldier.GetActiveWeapon().GetWeaponParameters().fire_interval;
    for (unsigned int tick = 0; tick < interval * 2; ++tick) {
        Step(*world, tick);
    }
    ASSERT_EQ(accepted.size(), 2U);
    EXPECT_EQ(accepted[0].weapon.shot_sequence, 1U);
    EXPECT_EQ(accepted[1].weapon.shot_sequence, 2U);
    EXPECT_EQ(accepted[0].weapon.ammo, soldier.GetActiveWeapon().GetCapacity() - 1);
    EXPECT_EQ(accepted[1].tick - accepted[0].tick, interval);
}

TEST(WeaponParametersTest, PinnedNormalAndRealisticTimingFixtures)
{
    struct Fixture
    {
        WeaponType kind;
        int normal_ammo;
        int realistic_ammo;
        int normal_interval;
        int realistic_interval;
        int normal_startup;
        int realistic_startup;
        int normal_reload;
        int realistic_reload;
    };
    // Fixtures from the pinned soldatbase INIs, in OpenSoldat's 60 Hz tick units.
    const std::vector<Fixture> fixtures{
        { WeaponType::DesertEagles, 7, 7, 24, 27, 0, 0, 87, 106 },
        { WeaponType::MP5, 30, 30, 6, 6, 0, 0, 105, 110 },
        { WeaponType::Ak74, 35, 35, 10, 11, 0, 0, 165, 158 },
        { WeaponType::SteyrAUG, 25, 30, 7, 7, 0, 0, 125, 126 },
        { WeaponType::Spas12, 7, 7, 32, 35, 0, 0, 175, 175 },
        { WeaponType::Ruger77, 4, 4, 45, 52, 0, 0, 78, 104 },
        { WeaponType::M79, 1, 1, 6, 6, 0, 0, 178, 173 },
        { WeaponType::Barrett, 10, 10, 225, 200, 19, 16, 70, 170 },
        { WeaponType::Minimi, 50, 50, 9, 10, 0, 0, 250, 261 },
        { WeaponType::Minigun, 100, 100, 3, 4, 25, 33, 480, 320 },
        { WeaponType::USSOCOM, 14, 12, 10, 12, 0, 0, 60, 72 },
        { WeaponType::Knife, 1, 1, 6, 6, 0, 0, 3, 3 },
        { WeaponType::Chainsaw, 200, 200, 2, 2, 0, 0, 110, 110 },
        { WeaponType::LAW, 1, 1, 6, 30, 13, 12, 300, 495 },
        { WeaponType::NoWeapon, 1, 1, 6, 6, 0, 0, 3, 3 },
        { WeaponType::FragGrenade, 1, 1, 80, 80, 0, 0, 20, 20 },
        { WeaponType::ClusterGrenade, 1, 1, 80, 80, 0, 0, 20, 20 },
        { WeaponType::Cluster, 1, 1, 80, 80, 0, 0, 20, 20 },
        { WeaponType::ThrownKnife, 1, 1, 6, 6, 0, 0, 3, 3 },
        { WeaponType::M2, 100, 100, 10, 14, 0, 21, 366, 366 },
        { WeaponType::Flamer, 200, 200, 6, 6, 0, 0, 5, 5 },
        { WeaponType::Bow, 1, 1, 10, 10, 0, 0, 25, 25 },
        { WeaponType::FlameBow, 1, 1, 10, 10, 0, 0, 39, 39 },
    };
    for (const auto& fixture : fixtures) {
        for (const bool realistic : { false, true }) {
            SCOPED_TRACE(static_cast<int>(fixture.kind));
            SCOPED_TRACE(realistic);
            const auto& parameters =
              WeaponParametersFactory::GetParameters(fixture.kind, realistic);
            EXPECT_EQ(parameters.ammo, realistic ? fixture.realistic_ammo : fixture.normal_ammo);
            EXPECT_EQ(parameters.fire_interval,
                      realistic ? fixture.realistic_interval : fixture.normal_interval);
            EXPECT_EQ(parameters.start_up_time,
                      realistic ? fixture.realistic_startup : fixture.normal_startup);
            EXPECT_EQ(parameters.reload_time,
                      realistic ? fixture.realistic_reload : fixture.normal_reload);
            EXPECT_EQ(parameters.realistic, realistic);
        }
    }
    EXPECT_FLOAT_EQ(WeaponParametersFactory::GetParameters(WeaponType::M79, false).speed, 10.7F);
    EXPECT_FLOAT_EQ(WeaponParametersFactory::GetParameters(WeaponType::M79, true).speed, 11.4F);
}

TEST(WeaponSimulationTest, NearbyRifleButtstockContactsWithoutSpendingGunAmmo)
{
    auto world = CreateWeaponWorld();
    auto state = world->GetStateManager();
    world->CreateSoldier(2);
    world->SpawnSoldier(2, glm::vec2(5.0F, 0.0F));
    state->ChangeSoldierPrimaryWeapon(1, WeaponType::MP5);
    auto& soldier = state->GetSoldier(1);
    const auto ammo = soldier.GetActiveWeapon().GetAmmoCount();
    Step(*world, 0);
    world->UpdateSoldier(2);
    soldier.control.fire = true;
    Step(*world, 1);
    soldier.control.fire = false;
    for (unsigned int tick = 2; tick < 22; ++tick) {
        Step(*world, tick);
    }
    ASSERT_EQ(state->GetBulletEmitter().size(), 1U);
    EXPECT_EQ(state->GetBulletEmitter().front().weapon, WeaponType::NoWeapon);
    EXPECT_EQ(soldier.GetActiveWeapon().GetAmmoCount(), ammo);
}

TEST(WeaponSimulationTest, BowRegenerationIsAuthoritativeAndDoesNotRepeatDuringPrediction)
{
    for (const bool authoritative : { false, true }) {
        auto world = CreateWeaponWorld(authoritative);
        auto state = world->GetStateManager();
        state->ChangeSoldierPrimaryWeapon(1, WeaponType::Bow);
        state->GetSoldier(1).health = 140.0F;
        for (unsigned int tick = 0; tick < 30; ++tick) {
            Step(*world, tick);
        }
        EXPECT_FLOAT_EQ(state->GetSoldier(1).health, authoritative ? 150.0F : 140.0F);
    }
}

TEST(WeaponSimulationTest, GrenadeAnimationPausesAndResumesMagazineReload)
{
    auto world = CreateWeaponWorld();
    auto state = world->GetStateManager();
    state->ChangeSoldierPrimaryWeapon(1, WeaponType::MP5);
    auto& soldier = state->GetSoldier(1);
    auto partial = soldier.GetActiveWeapon().GetState();
    partial.ammo = 2;
    soldier.GetActiveWeapon().RestoreState(partial);
    soldier.control.reload = true;
    Step(*world, 0);
    soldier.control.reload = false;
    soldier.control.throw_grenade = true;
    Step(*world, 1);
    const auto paused = soldier.GetActiveWeapon().GetReloadTimeCount();
    for (unsigned int tick = 2; tick < 10; ++tick) {
        Step(*world, tick);
    }
    EXPECT_EQ(soldier.GetActiveWeapon().GetReloadTimeCount(), paused);
    soldier.control.throw_grenade = false;
    Step(*world, 10);
    Step(*world, 11);
    EXPECT_EQ(soldier.GetActiveWeapon().GetReloadTimeCount(), paused - 1);
    EXPECT_TRUE(soldier.GetActiveWeapon().IsReloading());
}

TEST(WeaponParametersTest, ConfigurationFingerprintIsStableAndDistinguishesGameModes)
{
    const auto normal = WeaponParametersFactory::GetConfigurationHash(false);
    EXPECT_NE(normal, 0U);
    EXPECT_EQ(normal, WeaponParametersFactory::GetConfigurationHash(false));
    EXPECT_NE(normal, WeaponParametersFactory::GetConfigurationHash(true));
}

TEST(WeaponSimulationTest, RespawnCancelsOldThrowAndGivesFreshWeaponInstances)
{
    auto world = CreateWeaponWorld();
    auto state = world->GetStateManager();
    world->UpdateWeaponChoices(1, WeaponType::MP5, WeaponType::Knife);
    state->ChangeSoldierPrimaryWeapon(1, WeaponType::MP5);
    auto& soldier = state->GetSoldier(1);
    const auto previous_instance = soldier.GetActiveWeapon().GetState().instance_id;
    soldier.body_animation = world->GetBodyAnimationState(AnimationType::ThrowWeapon);
    soldier.body_animation->SetFrame(soldier.body_animation->GetFramesCount());
    soldier.control.drop = true;
    soldier.vest = 75.0F;
    soldier.fired = 3;
    world->SpawnSoldier(1, glm::vec2{});
    EXPECT_EQ(soldier.body_animation->GetType(), AnimationType::Stand);
    EXPECT_NE(soldier.GetActiveWeapon().GetState().instance_id, previous_instance);
    EXPECT_EQ(soldier.GetActiveWeapon().GetWeaponParameters().kind, WeaponType::MP5);
    EXPECT_FLOAT_EQ(soldier.vest, 0.0F);
    EXPECT_EQ(soldier.fired, 0);
    Step(*world, 1);
    EXPECT_EQ(soldier.GetActiveWeapon().GetWeaponParameters().kind, WeaponType::MP5);
}

TEST(WeaponSimulationTest, CeasefireExpiresBeforeFiringBecomesEligible)
{
    auto world = CreateWeaponWorld();
    auto state = world->GetStateManager();
    state->ChangeSoldierPrimaryWeapon(1, WeaponType::MP5);
    auto& soldier = state->GetSoldier(1);
    auto ready = soldier.GetActiveWeapon().GetState();
    ready.fire_interval = 0;
    ready.startup = 0;
    soldier.GetActiveWeapon().RestoreState(ready);
    soldier.SetCeaseFireCounter(2);
    soldier.control.fire = true;
    soldier.control.mouse_aim_x = 1000;
    Step(*world, 0);
    Step(*world, 1);
    EXPECT_TRUE(state->GetBulletEmitter().empty());
    Step(*world, 2);
    EXPECT_EQ(state->GetBulletEmitter().size(), 1U);
}

TEST(WeaponSimulationTest, MountedM2IsExclusiveAndStopsAtOverheatThenCoolsOnRelease)
{
    auto world = CreateWeaponWorld();
    auto state = world->GetStateManager();
    world->CreateSoldier(2);
    world->SpawnSoldier(2, glm::vec2{});
    auto& first = state->GetSoldier(1);
    auto& second = state->GetSoldier(2);
    auto& item = state->CreateItem(glm::vec2{}, 255, ItemType::M2);
    item.time_out = 0;
    first.control.fire = true;
    first.control.mouse_aim_x = 1000;
    state->SetGameTick(0);
    StationaryWeaponPhysics::Update(*state, item);
    ASSERT_EQ(first.mounted_weapon, item.id);
    EXPECT_EQ(second.mounted_weapon, -1);
    ASSERT_EQ(state->GetBulletEmitter().size(), 1U);
    EXPECT_EQ(state->GetBulletEmitter().front().weapon, WeaponType::M2);
    state->ClearBulletEmitter();
    item.interest = 19;
    StationaryWeaponPhysics::Update(*state, item);
    EXPECT_TRUE(state->GetBulletEmitter().empty());
    first.control.fire = false;
    state->SetGameTick(8);
    StationaryWeaponPhysics::Update(*state, item);
    EXPECT_EQ(item.interest, 18);
    first.control.up = true;
    StationaryWeaponPhysics::Update(*state, item);
    EXPECT_EQ(first.mounted_weapon, -1);
    EXPECT_EQ(second.mounted_weapon, item.id);
    second.dead_meat = true;
    StationaryWeaponPhysics::Update(*state, item);
    EXPECT_EQ(second.mounted_weapon, -1);
    EXPECT_EQ(item.mounted_soldier, -1);
}

TEST(WeaponSimulationTest, MountedM2DoesNotCreateProjectilesDuringPrediction)
{
    auto world = CreateWeaponWorld(false);
    auto state = world->GetStateManager();
    auto& soldier = state->GetSoldier(1);
    soldier.control.fire = true;
    soldier.control.mouse_aim_x = 1000;
    auto& item = state->CreateItem(glm::vec2{}, 255, ItemType::M2);
    item.time_out = 0;
    StationaryWeaponPhysics::Update(*state, item);
    EXPECT_TRUE(state->GetBulletEmitter().empty());
    EXPECT_EQ(item.interest, 0);
}
