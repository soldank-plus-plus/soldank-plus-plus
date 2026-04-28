#include <gtest/gtest.h>

import Shared.Core.Entities.Weapon;
import Shared.Core.Entities.WeaponParameters;

TEST(WeaponTests, TestWeaponsLoadedCorrectly)
{
    Soldank::WeaponParameters weapon_parameters;
    weapon_parameters.ammo = 50;
    Soldank::Weapon weapon(weapon_parameters);
    ASSERT_EQ(weapon.GetAmmoCount(), 50);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
