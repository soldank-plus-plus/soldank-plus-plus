#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <limits>
#include <string_view>

import Extern.Glm;
import ClientState;
import Audio.AudioEvents;
import Gameplay.PolygonPresentationEventRouter;
import Networking.SoldierStateNetworkEventHandler;
import Networking.ItemStateNetworkEventHandler;
import Networking.PolygonEffectNetworkEventHandler;
import Shared.Core.World;
import Shared.Core.Entities.Soldier;
import Shared.Core.Entities.Item;
import Shared.Core.Map.PMSEnums;
import Shared.Core.Map.Polygons.PolygonBackgroundState;
import Shared.Core.Physics.Polygons.PolygonContact;
import Shared.Networking.NetworkEventDispatcher;
import Shared.Networking.NetworkEvent;
import Shared.Networking.NetworkMessage;
import Shared.Networking.NetworkPackets;
import Shared.Core.Animations;
import Shared.Core.Types.ItemType;
import Testing.Framework.Shared.MapBuilder;

using namespace Soldank;
namespace
{
class PolygonReconciliationTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        world_ = std::make_shared<World>();
        auto map =
          SoldankTesting::MapBuilder::Empty()
            ->AddPolygon({ -100, -100 }, { 100, -100 }, { 0, 100 }, PMSPolygonType::Background)
            ->Build();
        world_->GetStateManager()->OverrideMap(*map);
        world_->GetStateManager()->SetAuthoritativeProjectiles(false);
        world_->CreateSoldier(1);
        world_->SpawnSoldier(1, glm::vec2{});
        client_ = std::make_shared<ClientState>();
        client_->client_soldier_id = 1;
        dispatcher_ = std::make_shared<NetworkEventDispatcher>(
          std::vector<std::shared_ptr<INetworkEventHandler>>{
            std::make_shared<SoldierStateNetworkEventHandler>(world_, client_),
            std::make_shared<ItemStateNetworkEventHandler>(world_),
            std::make_shared<PolygonEffectNetworkEventHandler>(world_) });
        audio_ = std::make_shared<AudioEvents>();
        audio_->sound_requested.AddObserver(
          [this](const AudioEvent& event) { sounds_.push_back(event); });
        router_ =
          PolygonPresentationEventRouter::Create(world_->GetPhysicsEvents(), audio_, client_);
    }
    template<typename Packet>
    NetworkEventDispatchResult Send(NetworkEvent kind, const Packet& packet)
    {
        const ConnectionMetadata connection{
            .connection_id = 1, .send_message_to_connection = [](const NetworkMessage&) {}
        };
        return dispatcher_->ProcessNetworkMessage(connection, NetworkMessage(kind, packet)).first;
    }
    SoldierStatePacket Snapshot(unsigned int tick)
    {
        const auto& soldier = world_->GetSoldier(1);
        SoldierStatePacket packet{};
        packet.server_tick = tick;
        packet.player_id = 1;
        packet.body_animation_type = AnimationType::Stand;
        packet.body_animation_frame = 1;
        packet.body_animation_speed = 3;
        packet.legs_animation_type = AnimationType::Stand;
        packet.legs_animation_frame = 1;
        packet.legs_animation_speed = 3;
        packet.has_polygon_state = true;
        packet.life_sequence = soldier.life_sequence;
        packet.polygon_background = {
            .map_fingerprint = world_->GetStateManager()->GetConstMap().GetPolygonFingerprint(),
            .phase = PolygonBackgroundPhase::Normal,
            .polygon = -1
        };
        packet.weapon_generation = soldier.weapon_generation;
        packet.grenade_can_throw = soldier.grenade_can_throw;
        packet.hit_spray = soldier.hit_spray;
        packet.mounted_weapon = soldier.mounted_weapon;
        packet.combat_bonus = soldier.combat_bonus;
        packet.bonus_ticks = soldier.bonus_ticks;
        packet.control = soldier.control;
        packet.fired = soldier.fired;
        packet.weapon_count = 3;
        for (std::size_t slot = 0; slot < 3; ++slot) {
            packet.weapons[slot] = soldier.weapons[slot].GetState();
        }
        packet.health = 150;
        return packet;
    }
    std::shared_ptr<World> world_;
    std::shared_ptr<ClientState> client_;
    std::shared_ptr<NetworkEventDispatcher> dispatcher_;
    std::shared_ptr<AudioEvents> audio_;
    std::vector<AudioEvent> sounds_;
    std::shared_ptr<PolygonPresentationEventRouter> router_;
};
} // namespace
TEST_F(PolygonReconciliationTest, AccuratePositionDoesNotHideBackgroundOrCarrierMismatch)
{
    auto& soldier = world_->GetStateManager()->GetSoldier(1);
    soldier.polygon_background = {
        .map_fingerprint = world_->GetStateManager()->GetConstMap().GetPolygonFingerprint(),
        .phase = PolygonBackgroundPhase::Normal,
        .polygon = -1
    };
    client_->network.soldier_snapshot_history.emplace_back(0, 0, 10, soldier);
    auto packet = Snapshot(10);
    packet.polygon_background.phase = PolygonBackgroundPhase::Transition;
    packet.polygon_background.polygon = 0;
    packet.carrying_flag = true;
    EXPECT_EQ(Send(NetworkEvent::SoldierState, packet), NetworkEventDispatchResult::Success);
    EXPECT_EQ(soldier.polygon_background, packet.polygon_background);
    EXPECT_TRUE(soldier.is_holding_flags);
    packet.server_tick = 9;
    packet.carrying_flag = false;
    EXPECT_EQ(Send(NetworkEvent::SoldierState, packet), NetworkEventDispatchResult::Success);
    EXPECT_TRUE(soldier.is_holding_flags);
}
TEST_F(PolygonReconciliationTest, InvalidPhasePolygonMapAndHealthAreRejectedWithoutMutation)
{
    const auto baseline = world_->GetSoldier(1).polygon_background;
    auto packet = Snapshot(10);
    packet.polygon_background.phase = static_cast<PolygonBackgroundPhase>(42);
    EXPECT_EQ(Send(NetworkEvent::SoldierState, packet), NetworkEventDispatchResult::HandlerFailure);
    packet = Snapshot(10);
    packet.polygon_background.polygon = 10;
    EXPECT_EQ(Send(NetworkEvent::SoldierState, packet), NetworkEventDispatchResult::HandlerFailure);
    packet = Snapshot(10);
    packet.polygon_background.map_fingerprint ^= 1;
    EXPECT_EQ(Send(NetworkEvent::SoldierState, packet), NetworkEventDispatchResult::HandlerFailure);
    packet = Snapshot(10);
    packet.health = std::numeric_limits<float>::quiet_NaN();
    EXPECT_EQ(Send(NetworkEvent::SoldierState, packet), NetworkEventDispatchResult::HandlerFailure);
    EXPECT_EQ(world_->GetSoldier(1).polygon_background, baseline);
}
TEST_F(PolygonReconciliationTest, ItemHolderMayArriveBeforeSoldierAndCarriesBackgroundState)
{
    ItemStatePacket packet{
        .server_tick = 10, .id = 5, .active = true, .style = ItemType::AlphaFlag, .owner = 255
    };
    packet.holding_soldier_id = 0;
    packet.particle_count = 4;
    packet.polygon_background = {
        .map_fingerprint = world_->GetStateManager()->GetConstMap().GetPolygonFingerprint(),
        .polygon = 0
    };
    EXPECT_EQ(Send(NetworkEvent::ItemState, packet), NetworkEventDispatchResult::Success);
    bool found = false;
    world_->GetStateManager()->ForEachItem([&](const Item& item) {
        if (item.id == 5) {
            found = true;
            EXPECT_EQ(item.holding_soldier_id, 0);
            EXPECT_EQ(item.polygon_background, packet.polygon_background);
        }
    });
    EXPECT_TRUE(found);
    packet.server_tick = 9;
    packet.active = false;
    EXPECT_EQ(Send(NetworkEvent::ItemState, packet), NetworkEventDispatchResult::Success);
    packet.server_tick = 11;
    packet.active = true;
    packet.holding_soldier_id = 32;
    EXPECT_EQ(Send(NetworkEvent::ItemState, packet), NetworkEventDispatchResult::HandlerFailure);
}
TEST_F(PolygonReconciliationTest, ConfirmedEffectsAreDeduplicatedAndNeverApplyDamage)
{
    PolygonEffectPacket packet{
        .event = { .kind = PolygonEventKind::Hurt,
                   .soldier = 1,
                   .tick = 10,
                   .life = 1,
                   .polygon = 0,
                   .amount = 5 },
        .map_fingerprint = world_->GetStateManager()->GetConstMap().GetPolygonFingerprint()
    };
    EXPECT_EQ(Send(NetworkEvent::PolygonEffect, packet), NetworkEventDispatchResult::Success);
    packet.event.amount = 42;
    EXPECT_EQ(Send(NetworkEvent::PolygonEffect, packet), NetworkEventDispatchResult::Success);
    ASSERT_EQ(sounds_.size(), 1U);
    EXPECT_EQ(sounds_.front().sound, "arg");
    EXPECT_EQ(world_->GetSoldier(1).health, 150);
    EXPECT_TRUE(world_->GetStateManager()->GetBulletEmitter().empty());
    client_->network.replaying_prediction = true;
    packet.event.ordinal = 1;
    EXPECT_EQ(Send(NetworkEvent::PolygonEffect, packet), NetworkEventDispatchResult::Success);
    EXPECT_EQ(sounds_.size(), 1U);
    client_->network.replaying_prediction = false;
    EXPECT_EQ(Send(NetworkEvent::PolygonEffect, packet), NetworkEventDispatchResult::Success);
    EXPECT_EQ(sounds_.size(), 2U);
    packet.event.life = 2;
    EXPECT_EQ(Send(NetworkEvent::PolygonEffect, packet), NetworkEventDispatchResult::Success);
    EXPECT_EQ(sounds_.size(), 3U);
    packet.map_fingerprint ^= 1;
    EXPECT_EQ(Send(NetworkEvent::PolygonEffect, packet),
              NetworkEventDispatchResult::HandlerFailure);
}
TEST_F(PolygonReconciliationTest, DestroyedPresentationSubscriberIsSafe)
{
    router_.reset();
    world_->GetPhysicsEvents().polygon_event.Notify(
      { .kind = PolygonEventKind::Heal, .soldier = 1, .polygon = 0 });
    EXPECT_TRUE(sounds_.empty());
    EXPECT_TRUE(client_->polygon_visual_events.empty());
}

TEST_F(PolygonReconciliationTest, SnapshotTicksWrapWithoutRestoringStaleCarrierState)
{
    auto packet = Snapshot(std::numeric_limits<unsigned int>::max() - 1);
    EXPECT_EQ(Send(NetworkEvent::SoldierState, packet), NetworkEventDispatchResult::Success);
    packet.server_tick = 1;
    packet.carrying_flag = true;
    EXPECT_EQ(Send(NetworkEvent::SoldierState, packet), NetworkEventDispatchResult::Success);
    packet.server_tick = std::numeric_limits<unsigned int>::max();
    packet.carrying_flag = false;
    EXPECT_EQ(Send(NetworkEvent::SoldierState, packet), NetworkEventDispatchResult::Success);
    EXPECT_TRUE(world_->GetSoldier(1).is_holding_flags);
}

TEST_F(PolygonReconciliationTest, CachedBackgroundIdCannotReferenceAnOrdinaryPolygon)
{
    auto polygon = world_->GetStateManager()->GetConstMap().GetPolygons()[0];
    polygon.polygon_type = PMSPolygonType::Normal;
    world_->GetStateManager()->GetMap().SetPolygonsById({ { 0, polygon } });
    auto packet = Snapshot(10);
    packet.polygon_background.phase = PolygonBackgroundPhase::Transition;
    packet.polygon_background.polygon = 0;
    EXPECT_EQ(Send(NetworkEvent::SoldierState, packet), NetworkEventDispatchResult::HandlerFailure);
}

TEST_F(PolygonReconciliationTest, EachContactPresentationUsesTheRecordingAudioSink)
{
    const std::vector<PolygonEffectEvent> events{
        { .kind = PolygonEventKind::Hurt, .ordinal = 0 },
        { .kind = PolygonEventKind::Heal, .ordinal = 1 },
        { .kind = PolygonEventKind::Lava, .ordinal = 2 },
        { .kind = PolygonEventKind::Bounce, .ordinal = 3, .amount = 1 },
        { .kind = PolygonEventKind::Bounce, .ordinal = 4, .amount = 1.01F },
        { .kind = PolygonEventKind::Landing, .ordinal = 5, .amount = 3.5F },
        { .kind = PolygonEventKind::Landing, .ordinal = 6, .amount = 3.51F },
        { .kind = PolygonEventKind::Death, .ordinal = 7, .severity = DeathSeverity::Head },
        { .kind = PolygonEventKind::Death, .ordinal = 8, .severity = DeathSeverity::Brutal },
    };
    for (const auto& event : events) {
        world_->GetPhysicsEvents().polygon_event.Notify(event);
    }
    const std::vector<std::string_view> expected{ "arg",  "regenerate", "lava",  "bounce",
                                                  "fall", "fall-hard",  "death", "death3" };
    ASSERT_EQ(sounds_.size(), expected.size());
    for (std::size_t index = 0; index < expected.size(); ++index) {
        EXPECT_EQ(sounds_.at(index).command, AudioCommand::Play);
        EXPECT_EQ(sounds_.at(index).sound, expected.at(index));
    }
    EXPECT_EQ(client_->polygon_visual_events.size(), events.size());
    EXPECT_EQ(world_->GetSoldier(1).health, 150);
    EXPECT_TRUE(world_->GetStateManager()->GetBulletEmitter().empty());
}
