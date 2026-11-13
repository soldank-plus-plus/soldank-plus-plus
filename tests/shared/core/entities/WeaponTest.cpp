#include <gtest/gtest.h>

#include <cstdint>

import Shared.Core.Entities.Weapon;
import Shared.Core.Entities.WeaponParameters;
import Shared.Core.Entities.Soldier;
import Shared.Core.Types.TeamType;
import Shared.Core.Types.WeaponType;

TEST(WeaponTests, TestWeaponsLoadedCorrectly)
{
    Soldank::WeaponParameters weapon_parameters{};
    weapon_parameters.ammo = 50;
    Soldank::Weapon weapon(weapon_parameters);
    ASSERT_EQ(weapon.GetAmmoCount(), 50);
}

TEST(WeaponTests, ExposesOpenSoldatNoCollisionMasks)
{
    constexpr std::uint8_t COLLISION_MASK =
      static_cast<std::uint8_t>(Soldank::WeaponNoCollision::Enemy) |
      static_cast<std::uint8_t>(Soldank::WeaponNoCollision::ExplosionSelf);

    EXPECT_TRUE(Soldank::HasWeaponNoCollision(COLLISION_MASK, Soldank::WeaponNoCollision::Enemy));
    EXPECT_FALSE(Soldank::HasWeaponNoCollision(COLLISION_MASK, Soldank::WeaponNoCollision::Team));
    EXPECT_TRUE(
      Soldank::HasWeaponNoCollision(COLLISION_MASK, Soldank::WeaponNoCollision::ExplosionSelf));
}

TEST(WeaponTests, ExposesCombatStateThroughSoldierApis)
{
    Soldank::WeaponParameters weapon_parameters{};
    weapon_parameters.ammo = 10;
    Soldank::Soldier soldier;
    soldier.active = true;
    soldier.dead_meat = false;
    soldier.weapons.emplace_back(weapon_parameters);

    soldier.SetTeam(Soldank::TeamType::Alpha);
    soldier.SetCeaseFireCounter(-1);
    soldier.SetPingTicks(12);

    EXPECT_EQ(soldier.GetTeam(), Soldank::TeamType::Alpha);
    EXPECT_FALSE(soldier.IsSpectator());
    EXPECT_TRUE(soldier.IsAlive());
    EXPECT_FALSE(soldier.IsRagdoll());
    EXPECT_TRUE(soldier.CanReceiveCombatImpact());
    EXPECT_EQ(soldier.GetPingTicks(), 12);
    EXPECT_EQ(soldier.GetActiveWeapon().GetAmmoCount(), 10);

    soldier.SetTeam(Soldank::TeamType::Spectator);
    EXPECT_TRUE(soldier.IsSpectator());
    EXPECT_FALSE(soldier.CanReceiveCombatImpact());
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

namespace
{
Soldank::WeaponParameters RuntimeParameters(Soldank::WeaponType kind = Soldank::WeaponType::MP5)
{
    Soldank::WeaponParameters parameters{};
    parameters.kind = kind;
    parameters.ammo = 3;
    parameters.fire_interval = 2;
    parameters.reload_time = 4;
    parameters.start_up_time = 2;
    return parameters;
}
} // namespace

TEST(WeaponRuntimeTest, CadenceAndStartupHaveExactTickBoundaries)
{
    Soldank::Weapon weapon(RuntimeParameters());
    EXPECT_FALSE(weapon.TryFire());
    weapon.Advance(true);
    EXPECT_FALSE(weapon.TryFire());
    weapon.Advance(true);
    EXPECT_FALSE(weapon.TryFire()); // startup 2 -> 1
    EXPECT_FALSE(weapon.TryFire()); // startup 1 -> 0
    EXPECT_EQ(weapon.GetAmmoCount(), 3);
    ASSERT_TRUE(weapon.TryFire());
    EXPECT_EQ(weapon.GetAmmoCount(), 2);
    EXPECT_EQ(weapon.GetState().shot_sequence, 1U);
    EXPECT_FALSE(weapon.TryFire());
    weapon.Advance(true);
    EXPECT_FALSE(weapon.TryFire());
    weapon.Advance(true);
    EXPECT_TRUE(weapon.TryFire());
    EXPECT_EQ(weapon.GetAmmoCount(), 1);
}

TEST(WeaponRuntimeTest, ReleaseResetsPartiallyCompletedStartup)
{
    Soldank::Weapon weapon(RuntimeParameters());
    weapon.Advance(true);
    weapon.Advance(true);
    EXPECT_FALSE(weapon.TryFire());
    EXPECT_EQ(weapon.GetStartUpTimeCount(), 1);
    weapon.Advance(false);
    EXPECT_EQ(weapon.GetStartUpTimeCount(), 2);
    EXPECT_EQ(weapon.GetAmmoCount(), 3);
}

TEST(WeaponRuntimeTest, MagazineReloadRefillsCapacityAndRestoresCadence)
{
    Soldank::Weapon weapon(RuntimeParameters(), 1);
    ASSERT_TRUE(weapon.BeginReload());
    EXPECT_EQ(weapon.GetAmmoCount(), 0);
    for (int tick = 0; tick < 3; ++tick) {
        weapon.Advance(true);
        EXPECT_TRUE(weapon.IsReloading());
        EXPECT_FALSE(weapon.TryFire());
    }
    weapon.Advance(true);
    EXPECT_FALSE(weapon.IsReloading());
    EXPECT_EQ(weapon.GetAmmoCount(), 3);
    EXPECT_EQ(weapon.GetFireIntervalCount(), 2);
    EXPECT_EQ(weapon.GetStartUpTimeCount(), 2);
}

TEST(WeaponRuntimeTest, PickupAmmoDoesNotChangeMagazineCapacity)
{
    Soldank::Weapon weapon(RuntimeParameters(), 1);
    EXPECT_EQ(weapon.GetAmmoCount(), 1);
    EXPECT_EQ(weapon.GetWeaponParameters().ammo, 3);
    weapon.BeginReload();
    for (int tick = 0; tick < 4; ++tick) {
        weapon.Advance(false);
    }
    EXPECT_EQ(weapon.GetAmmoCount(), 3);
}

TEST(WeaponRuntimeTest, M79StartsEmptyButLoadedPickupsStayLoaded)
{
    auto parameters = RuntimeParameters(Soldank::WeaponType::M79);
    parameters.ammo = 1;
    EXPECT_EQ(Soldank::Weapon(parameters).GetAmmoCount(), 0);
    EXPECT_EQ(Soldank::Weapon(parameters, 1).GetAmmoCount(), 1);
}

TEST(WeaponRuntimeTest, SnapshotRoundTripPreservesNextShotAndRejectsInvalidState)
{
    Soldank::Weapon original(RuntimeParameters());
    original.Advance(true);
    original.Advance(true);
    original.TryFire();
    Soldank::Weapon restored(RuntimeParameters());
    ASSERT_TRUE(restored.RestoreState(original.GetState()));
    EXPECT_EQ(restored.GetState(), original.GetState());
    EXPECT_EQ(restored.TryFire(), original.TryFire());
    EXPECT_EQ(restored.TryFire(), original.TryFire());
    EXPECT_EQ(restored.GetState(), original.GetState());
    auto invalid = original.GetState();
    invalid.ammo = 255;
    const auto before = restored.GetState();
    EXPECT_FALSE(restored.RestoreState(invalid));
    EXPECT_EQ(restored.GetState(), before);
}

TEST(WeaponRuntimeTest, SpasLoadsIndividualShellsAndCanInterruptToShoot)
{
    auto parameters = RuntimeParameters(Soldank::WeaponType::Spas12);
    parameters.start_up_time = 0;
    Soldank::Weapon weapon(parameters, 0);
    EXPECT_FALSE(weapon.BeginReload());
    weapon.Advance(false);
    weapon.Advance(false);
    ASSERT_TRUE(weapon.BeginReload());
    weapon.Advance(false);
    EXPECT_EQ(weapon.GetAmmoCount(), 0);
    weapon.InsertShell();
    EXPECT_EQ(weapon.GetAmmoCount(), 1);
    ASSERT_TRUE(weapon.TryFire());
    EXPECT_FALSE(weapon.IsReloading());
    EXPECT_EQ(weapon.GetAmmoCount(), 0);
}

TEST(WeaponRuntimeTest, MeleeDoesNotConsumeItsWeaponOrReload)
{
    for (auto kind : { Soldank::WeaponType::Knife, Soldank::WeaponType::NoWeapon }) {
        auto parameters = RuntimeParameters(kind);
        parameters.start_up_time = 0;
        Soldank::Weapon weapon(parameters);
        weapon.Advance(false);
        weapon.Advance(false);
        ASSERT_TRUE(weapon.TryFire());
        EXPECT_EQ(weapon.GetAmmoCount(), 3);
        EXPECT_FALSE(weapon.BeginReload());
    }
}
