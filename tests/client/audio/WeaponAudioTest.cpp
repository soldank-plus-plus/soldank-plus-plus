#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

import Extern.Glm;
import Audio.AudioEvents;
import Audio.WeaponAudio;
import Gameplay.WeaponSoundEventRouter;
import Shared.Core.World;
import Shared.Core.Entities.Soldier;
import Shared.Core.Entities.Weapon;
import Shared.Core.Entities.Item;
import Shared.Core.Types.WeaponType;
import Shared.Core.Types.ItemType;
import Shared.Core.Types.BulletType;
import Shared.Core.Physics.Bullets.BulletEvents;
import Testing.Framework.Shared.MapBuilder;

using namespace Soldank;

namespace
{
class RecordingAudioPlayer final : public IAudioPlayer
{
public:
    void Handle(const AudioEvent& event) override { events.push_back(event); }
    std::vector<AudioEvent> events;
};

class WeaponSoundEventsTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        world_ = std::make_unique<World>();
        world_->GetStateManager()->OverrideMap(*SoldankTesting::MapBuilder::Empty()->Build());
        world_->CreateSoldier(1);
        world_->SpawnSoldier(1, glm::vec2{});
        world_->GetStateManager()->ChangeSoldierPrimaryWeapon(1, WeaponType::MP5);
        events_ = std::make_shared<AudioEvents>();
        events_->sound_requested.AddObserver(
          [this](const AudioEvent& event) { requests_.push_back(event); });
        router_ = WeaponSoundEventRouter::Create(world_->GetPhysicsEvents(),
                                                 world_->GetWorldEvents(),
                                                 *world_->GetStateManager(),
                                                 events_,
                                                 [this] { return !replaying_; });
    }

    void Step(unsigned int tick)
    {
        world_->GetStateManager()->SetGameTick(tick);
        world_->UpdateSoldier(1);
    }

    void Publish(unsigned int tick)
    {
        world_->GetWorldEvents().after_soldier_updates.Notify(world_->GetSoldier(1), tick);
    }

    std::size_t Count(AudioCommand command, std::string_view sound = {}) const
    {
        return std::ranges::count_if(requests_, [&](const AudioEvent& event) {
            return event.command == command && (sound.empty() || event.sound == sound);
        });
    }

    void ChangeWeapon(WeaponType kind)
    {
        world_->GetStateManager()->ChangeSoldierPrimaryWeapon(1, kind);
        Publish(0);
        requests_.clear();
    }

    std::unique_ptr<World> world_;
    std::shared_ptr<AudioEvents> events_;
    std::vector<AudioEvent> requests_;
    bool replaying_{};
    std::shared_ptr<WeaponSoundEventRouter> router_;
};
} // namespace

TEST(WeaponAudioTest, SubscribesWithoutGameStateAndStopsReceivingAfterDestruction)
{
    AudioEvents events;
    auto player = std::make_shared<RecordingAudioPlayer>();
    {
        WeaponAudio audio(events, player);
        events.sound_requested.Notify({ AudioCommand::Play, "mp5-fire" });
        events.sound_requested.Notify({ AudioCommand::StartLoop, "flamer", 7 });
        events.sound_requested.Notify({ AudioCommand::StopLoop, {}, 7 });
        ASSERT_EQ(player->events.size(), 3U);
        EXPECT_EQ(player->events.at(1).channel, 7);
        EXPECT_EQ(player->events.at(1).sound, "flamer");
    }
    ASSERT_EQ(player->events.size(), 4U);
    EXPECT_EQ(player->events.back().command, AudioCommand::StopAll);
    events.sound_requested.Notify({ AudioCommand::Play, "mp5-fire" });
    EXPECT_EQ(player->events.size(), 4U);
}

TEST_F(WeaponSoundEventsTest, SharedTickActionsPlayOnceWithoutRendering)
{
    auto& soldier = world_->GetStateManager()->GetSoldier(1);
    soldier.control.fire = true;
    soldier.control.mouse_aim_x = 1000;
    const auto interval = soldier.GetActiveWeapon().GetWeaponParameters().fire_interval;
    for (unsigned int tick = 0; tick < interval * 3U; ++tick) {
        Step(tick);
    }
    EXPECT_EQ(soldier.GetActiveWeapon().GetState().shot_sequence, 3U);
    EXPECT_EQ(Count(AudioCommand::Play, "mp5-fire"), 3U);
    const auto before = soldier.GetActiveWeapon().GetState();
    Publish(interval * 3U);
    EXPECT_EQ(Count(AudioCommand::Play, "mp5-fire"), 3U);
    EXPECT_EQ(soldier.GetActiveWeapon().GetState(), before);
}

TEST_F(WeaponSoundEventsTest, ReplayAndConfirmationCannotRepeatPredictedShots)
{
    auto& soldier = world_->GetStateManager()->GetSoldier(1);
    auto ready = soldier.GetActiveWeapon().GetState();
    ready.fire_interval = 0;
    ASSERT_TRUE(soldier.GetActiveWeapon().RestoreState(ready));
    soldier.control.fire = true;
    soldier.control.mouse_aim_x = 1000;
    Step(0);
    ASSERT_EQ(Count(AudioCommand::Play, "mp5-fire"), 1U);
    const auto fired = soldier.GetActiveWeapon().GetState();
    ASSERT_TRUE(soldier.GetActiveWeapon().RestoreState(ready));
    replaying_ = true;
    Step(0);
    EXPECT_EQ(Count(AudioCommand::Play, "mp5-fire"), 1U);
    replaying_ = false;
    ASSERT_TRUE(soldier.GetActiveWeapon().RestoreState(fired));
    Publish(1);
    EXPECT_EQ(Count(AudioCommand::Play, "mp5-fire"), 1U);
}

TEST_F(WeaponSoundEventsTest, AuthoritativeSnapshotsPlayRemoteShotsAndIgnoreSequenceRewinds)
{
    auto& weapon = world_->GetStateManager()->GetSoldier(1).GetActiveWeapon();
    const auto baseline = weapon.GetState();
    auto fired = baseline;
    fired.shot_sequence = 1;
    --fired.ammo;
    ASSERT_TRUE(weapon.RestoreState(fired));
    Publish(5);
    ASSERT_EQ(Count(AudioCommand::Play, "mp5-fire"), 1U);
    Publish(6);
    ASSERT_TRUE(weapon.RestoreState(baseline));
    Publish(7);
    ASSERT_TRUE(weapon.RestoreState(fired));
    Publish(8);
    EXPECT_EQ(Count(AudioCommand::Play, "mp5-fire"), 1U);
}

TEST_F(WeaponSoundEventsTest, ContinuousVoicesStopOnReleasePauseDeathAndRemoval)
{
    ChangeWeapon(WeaponType::Flamer);
    auto& soldier = world_->GetStateManager()->GetSoldier(1);
    soldier.control.fire = true;
    auto shot = soldier.GetActiveWeapon().GetState();
    const auto fire = [&] {
        ++shot.shot_sequence;
        world_->GetPhysicsEvents().weapon_action_event.Notify(
          { WeaponActionKind::Fired, 1, 0, shot, 0 });
    };
    fire();
    ASSERT_EQ(Count(AudioCommand::StartLoop), 1U);
    soldier.control.fire = false;
    Publish(0);
    EXPECT_EQ(Count(AudioCommand::StopLoop), 1U);
    soldier.control.fire = true;
    fire();
    world_->GetStateManager()->PauseGame();
    EXPECT_EQ(Count(AudioCommand::StopAll), 1U);
    const auto paused_count = requests_.size();
    fire();
    EXPECT_EQ(requests_.size(), paused_count);
    world_->GetStateManager()->UnPauseGame();
    fire();
    world_->GetWorldEvents().soldier_died.Notify(soldier);
    EXPECT_EQ(Count(AudioCommand::StopLoop), 2U);
    fire();
    world_->GetStateManager()->RemoveSoldier(1);
    EXPECT_EQ(Count(AudioCommand::StopLoop), 3U);
}

TEST_F(WeaponSoundEventsTest, ProjectileEventsSelectReferenceSoundsAndDeduplicateDelivery)
{
    ProjectilePhysicsEvent event{ .kind = ProjectileEventKind::ProjectileExploded,
                                  .projectile_id = 10,
                                  .style = BulletType::M79Grenade };
    auto& observable = world_->GetPhysicsEvents().projectile_event;
    observable.Notify(event);
    observable.Notify(event);
    EXPECT_EQ(Count(AudioCommand::Play, "m79-explosion"), 1U);
    ++event.projectile_id;
    event.style = BulletType::FragGrenade;
    observable.Notify(event);
    EXPECT_EQ(Count(AudioCommand::Play, "grenade-explosion"), 1U);
    ++event.projectile_id;
    event.kind = ProjectileEventKind::ProjectileFragmented;
    observable.Notify(event);
    EXPECT_EQ(Count(AudioCommand::Play, "clustergrenade"), 1U);
    ++event.projectile_id;
    event.kind = ProjectileEventKind::ProjectileExploded;
    event.style = BulletType::Cluster;
    observable.Notify(event);
    EXPECT_EQ(Count(AudioCommand::Play, "cluster-explosion"), 1U);
}

TEST_F(WeaponSoundEventsTest, MountedGunEventsTriggerMountFireAndOverheat)
{
    auto& item = world_->GetStateManager()->CreateItem({}, 255, ItemType::M2);
    item.mounted_soldier = 1;
    auto& observable = world_->GetWorldEvents().after_item_updates;
    observable.Notify(item, 0);
    EXPECT_EQ(Count(AudioCommand::Play, "m2use"), 1U);
    item.interest = 18;
    observable.Notify(item, 1);
    observable.Notify(item, 1);
    EXPECT_EQ(Count(AudioCommand::Play, "m2fire"), 1U);
    item.interest = 19;
    observable.Notify(item, 2);
    EXPECT_EQ(Count(AudioCommand::Play, "m2overheat"), 1U);
}

TEST_F(WeaponSoundEventsTest, DestroyedRouterLeavesNoCallableSubscriber)
{
    router_.reset();
    world_->GetPhysicsEvents().projectile_event.Notify(
      { .kind = ProjectileEventKind::ProjectileExploded, .projectile_id = 1 });
    Publish(0);
    world_->GetStateManager()->PauseGame();
    world_->GetStateManager()->RemoveSoldier(1);
    EXPECT_TRUE(requests_.empty());
}
