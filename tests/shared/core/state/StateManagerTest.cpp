#include <gtest/gtest.h>

#include <expected>
#include <ios>
#include <string>

#include "core/utility/Expected.hpp"

import Shared.Core.Animations;
import Shared.Core.Data.IFileReader;
import Shared.Core.Entities.Item;
import Shared.Core.Entities.Weapon;
import Shared.Core.Entities.WeaponParametersFactory;
import Shared.Core.Types.WeaponType;
import Shared.Core.Physics.Particles;
import Shared.Core.State.StateManager;
import Shared.Core.Types.ItemType;

namespace
{
class AnimationDataReader final : public Soldank::IFileReader
{
public:
    std::expected<std::string, Soldank::FileReaderError> Read(
      const std::string& file_path,
      std::ios_base::openmode /*mode*/) const override
    {
        if (file_path != "stand.poa") {
            return std::unexpected(Soldank::FileReaderError::FileNotFound);
        }

        return "1\n0\n0\n0\nENDFILE\n";
    }
};
} // namespace

TEST(StateManagerTest, CreateItemReturnsTheInitializedItemSlot)
{
    AnimationDataReader animation_data_reader;
    Soldank::AnimationDataManager animation_data_manager;
    animation_data_manager.LoadAnimationData(
      Soldank::AnimationType::Stand, "stand.poa", true, 1, animation_data_reader);
    Soldank::StateManager state_manager(
      animation_data_manager, Soldank::ParticleSystem::Load(Soldank::ParticleSystemType::Soldier));

    Soldank::Item& item = state_manager.CreateItem({ 100.0F, 200.0F }, 0, Soldank::ItemType::Ak74);

    EXPECT_TRUE(item.active);
    EXPECT_EQ(item.id, 0);
    EXPECT_EQ(item.style, Soldank::ItemType::Ak74);
    ASSERT_NE(item.skeleton, nullptr);
    EXPECT_EQ(item.skeleton->GetParticles().size(), 2U);
}

TEST(StateManagerTest, RandomNumbersAreReproducibleAfterResettingTheSeed)
{
    AnimationDataReader animation_data_reader;
    Soldank::AnimationDataManager animation_data_manager;
    animation_data_manager.LoadAnimationData(
      Soldank::AnimationType::Stand, "stand.poa", true, 1, animation_data_reader);
    Soldank::StateManager state_manager(
      animation_data_manager, Soldank::ParticleSystem::Load(Soldank::ParticleSystemType::Soldier));

    state_manager.SetRandomSeed(42U);
    const unsigned int first_value = state_manager.GetRandomInt(0, 1000);
    state_manager.SetRandomSeed(42U);

    EXPECT_EQ(state_manager.GetRandomInt(0, 1000), first_value);
}

TEST(StateManagerTest, PartiallyLoadedPickupKeepsItsOriginalCapacity)
{
    AnimationDataReader reader;
    Soldank::AnimationDataManager animations;
    animations.LoadAnimationData(Soldank::AnimationType::Stand, "stand.poa", true, 1, reader);
    Soldank::StateManager state(
      animations, Soldank::ParticleSystem::Load(Soldank::ParticleSystemType::Soldier));
    auto& item = state.CreateItem({}, 0, Soldank::ItemType::M79);
    EXPECT_EQ(item.ammo_count, 1);
    state.SoldierPickupWeapon(0, item);
    EXPECT_EQ(state.GetSoldier(0).GetActiveWeapon().GetAmmoCount(), 1);
    item.style = Soldank::ItemType::Ak74;
    item.ammo_count = 2;
    state.SoldierPickupWeapon(0, item);
    const auto& weapon = state.GetSoldier(0).GetActiveWeapon();
    EXPECT_EQ(weapon.GetAmmoCount(), 2);
    EXPECT_EQ(
      weapon.GetWeaponParameters().ammo,
      Soldank::WeaponParametersFactory::GetParameters(Soldank::WeaponType::Ak74, false).ammo);
}
