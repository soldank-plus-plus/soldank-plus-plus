#include <gtest/gtest.h>

#include <expected>
#include <ios>
#include <string>

#include "core/utility/Expected.hpp"

import Shared.Core.Animations;
import Shared.Core.Data.IFileReader;
import Shared.Core.Entities.Item;
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
