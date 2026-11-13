#include <gtest/gtest.h>
#include <array>
#include <memory>
#include <limits>
#include <vector>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <cstring>
#include <cstdint>
#include <stdexcept>

import Extern.Glm;
import Shared.Core.World;
import Shared.Core.Physics.Bullets.BulletEffects;
import Shared.Core.Entities.Bullet;
import Shared.Core.Entities.Soldier;
import Shared.Core.Entities.Item;
import Shared.Core.Entities.Bullet;
import Shared.Core.Physics.Weapons.WeaponPhysics;
import Shared.Core.Animations;
import Shared.Core.Map.Map;
import Shared.Core.Map.PMSConstants;
import Shared.Core.Simulation.WorldTick;
import Shared.Core.Physics.Bullets.BulletEvents;
import Shared.Core.Map.PMSStructs;
import Shared.Core.Map.PMSEnums;
import Shared.Core.Map.MapDocument;
import Shared.Core.Map.RuntimeMap;
import Shared.Core.Map.Polygons.PolygonValidation;
import Shared.Core.Map.Polygons.PolygonRenderBatch;
import Shared.Core.Map.Polygons.PolygonBackgroundState;
import Shared.Core.Physics.SoldierPhysics;
import Shared.Core.Physics.ItemPhysics;
import Shared.Core.Physics.Polygons.PolygonContacts;
import Shared.Core.Physics.Polygons.FlagCarry;
import Shared.Core.Physics.Polygons.PolygonContact;
import Shared.Core.Types.TeamType;
import Shared.Core.Types.ItemType;
import Testing.Framework.Shared.MapBuilder;

using namespace Soldank;
namespace
{
std::unique_ptr<World> WorldWithPolygon(PMSPolygonType type)
{
    auto world = std::make_unique<World>();
    auto map = SoldankTesting::MapBuilder::Empty()
                 ->AddPolygon({ -100, -100 }, { 100, -100 }, { 0, 100 }, type)
                 ->Build();
    world->GetStateManager()->OverrideMap(*map);
    world->CreateSoldier(1);
    world->SpawnSoldier(1, glm::vec2{});
    world->GetStateManager()->GetSoldier(1).SetTeam(TeamType::Alpha);
    return world;
}
glm::vec2 Center(const PMSPolygon& polygon)
{
    return { (polygon.vertices[0].x + polygon.vertices[1].x + polygon.vertices[2].x) / 3,
             (polygon.vertices[0].y + polygon.vertices[1].y + polygon.vertices[2].y) / 3 };
}
} // namespace
TEST(PolygonSimulationTest, EveryPolygonUsesTheSameDeclaredPolicyAcrossSoldierProbes)
{
    // Alpha carrier in Normal phase: fixtures from Sprites.TeamCollides and carrier gates.
    constexpr std::array<bool, 26> expected{ true,  false, true,  false, true, true,  true,
                                             true,  true,  true,  false, true, false, false,
                                             false, false, false, false, true, true,  true,
                                             true,  false, false, true,  false };
    for (unsigned int id = 0; id < 26; ++id) {
        SCOPED_TRACE(id);
        auto world = WorldWithPolygon(static_cast<PMSPolygonType>(id));
        auto state = world->GetStateManager();
        auto& soldier = state->GetSoldier(1);
        const auto& map = state->GetConstMap();
        const auto& polygon = map.GetPolygons().at(0);
        const auto center = Center(polygon);
        const auto reset = [&] {
            soldier.dead_meat = false;
            soldier.is_holding_flags = true;
            soldier.polygon_background = { .phase = PolygonBackgroundPhase::Normal, .polygon = -1 };
            soldier.particle.position = center;
            soldier.particle.old_position = center;
            soldier.particle.SetVelocity({ 0, 1 });
            soldier.polygon_contacts.clear();
        };
        reset();
        EXPECT_EQ(SoldierPhysics::CheckMapCollision(
                    soldier, map, center.x, center.y, 0, world->GetPhysicsEvents()),
                  expected.at(id));
        reset();
        EXPECT_EQ(SoldierPhysics::CheckRadiusMapCollision(
                    soldier, map, center.x, center.y, false, world->GetPhysicsEvents()),
                  expected.at(id));
        reset();
        const auto& vertex = polygon.vertices[0];
        EXPECT_EQ(SoldierPhysics::CheckMapVerticesCollision(
                    soldier, map, vertex.x, vertex.y, 3, false, world->GetPhysicsEvents()),
                  expected.at(id));
        reset();
        soldier.dead_meat = true;
        EXPECT_EQ(
          SoldierPhysics::CheckSkeletonMapCollision(soldier, map, 1, center.x + 1, center.y - 4),
          expected.at(id));
    }
}
TEST(PolygonSimulationTest, BackgroundCenterFallbackStateRestoreAndMapReplacement)
{
    auto world = WorldWithPolygon(PMSPolygonType::Background);
    auto state = world->GetStateManager();
    auto& soldier = state->GetSoldier(1);
    const auto center = Center(state->GetConstMap().GetPolygons()[0]);
    PolygonContacts::Begin(
      state->GetConstMap(), soldier.polygon_background, soldier.background_touched, center);
    EXPECT_TRUE(soldier.background_touched);
    EXPECT_EQ(soldier.polygon_background.polygon, 0);
    EXPECT_FALSE(PolygonContacts::SoldierContact(soldier, state->GetConstMap(), 0, center));
    SoldierSnapshot snapshot(soldier);
    EXPECT_EQ(snapshot.GetPolygonBackground(), soldier.polygon_background);
    PolygonContacts::Finish(soldier.polygon_background, false);
    EXPECT_TRUE(PolygonContacts::SoldierContact(soldier, state->GetConstMap(), 0, center));
    soldier.polygon_background = snapshot.GetPolygonBackground();
    auto empty = SoldankTesting::MapBuilder::Empty()->Build();
    state->OverrideMap(*empty);
    PolygonContacts::Begin(
      state->GetConstMap(), soldier.polygon_background, soldier.background_touched, center);
    PolygonContacts::Finish(soldier.polygon_background, soldier.background_touched);
    EXPECT_EQ(soldier.polygon_background.phase, PolygonBackgroundPhase::Normal);
    EXPECT_EQ(soldier.polygon_background.polygon, -1);
}
TEST(PolygonSimulationTest, CarryPickupThrowDeathAndSoldierZeroNeedNoObservers)
{
    auto world = WorldWithPolygon(PMSPolygonType::Normal);
    auto state = world->GetStateManager();
    world->CreateSoldier(0);
    world->SpawnSoldier(0, glm::vec2{});
    auto& zero = state->GetSoldier(0);
    zero.SetCeaseFireCounter(-1);
    auto& flag = state->CreateItem({}, 255, ItemType::AlphaFlag);
    EXPECT_TRUE(FlagCarry::TryAcquire(*state, zero, flag));
    EXPECT_EQ(flag.holding_soldier_id, 0);
    EXPECT_TRUE(zero.is_holding_flags);
    auto& other = state->GetSoldier(1);
    other.SetCeaseFireCounter(-1);
    EXPECT_FALSE(FlagCarry::TryAcquire(*state, other, flag));
    state->ReleaseSoldierFlags(0);
    EXPECT_EQ(flag.holding_soldier_id, -1);
    EXPECT_FALSE(zero.is_holding_flags);
    EXPECT_TRUE(FlagCarry::TryAcquire(*state, other, flag));
    int deaths = 0;
    world->GetWorldEvents().soldier_died.AddObserver([&](auto&) { ++deaths; });
    world->KillSoldier(1);
    world->KillSoldier(1);
    EXPECT_EQ(deaths, 1);
    EXPECT_EQ(flag.holding_soldier_id, -1);
    EXPECT_FALSE(other.is_holding_flags);
    world->SpawnSoldier(1, glm::vec2{});
    EXPECT_FALSE(other.death_committed);
}
TEST(PolygonSimulationTest, HazardWorldStepKillsOnceAndPreservesSeverityWithoutSpawnPoints)
{
    for (auto type :
         { PMSPolygonType::Deadly, PMSPolygonType::BloodyDeadly, PMSPolygonType::Explosive }) {
        auto world = WorldWithPolygon(type);
        auto state = world->GetStateManager();
        const auto center = Center(state->GetConstMap().GetPolygons()[0]);
        world->SpawnSoldier(1, center);
        int deaths = 0;
        world->GetWorldEvents().soldier_died.AddObserver([&](auto&) { ++deaths; });
        world->UpdateSoldier(1);
        EXPECT_TRUE(world->GetSoldier(1).dead_meat);
        EXPECT_EQ(deaths, 1);
        const auto health = world->GetSoldier(1).health;
        EXPECT_LT(health, 0);
        world->UpdateSoldier(1);
        EXPECT_EQ(deaths, 1);
        EXPECT_EQ(world->GetSoldier(1).health, health);
    }
}
TEST(PolygonSimulationTest, RenderBatchesAndValidationAreIndependentOfOpenGL)
{
    std::vector<PMSPolygon> polygons(26);
    for (unsigned int id = 0; id < 26; ++id) {
        polygons[id].id = id;
        polygons[id].polygon_type = static_cast<PMSPolygonType>(id);
        polygons[id].bounciness = 0.5F;
        EXPECT_NO_THROW(ValidatePolygon(polygons[id]));
    }
    const auto batch = BuildPolygonRenderBatch(polygons);
    EXPECT_EQ(batch.background_count, 2);
    EXPECT_EQ(batch.polygons.at(0).id, 24);
    EXPECT_EQ(batch.polygons.at(1).id, 25);
    for (unsigned int id = 0; id < 24; ++id) {
        EXPECT_EQ(batch.polygons.at(id + 2).id, id);
    }
    auto invalid = polygons[0];
    invalid.polygon_type = static_cast<PMSPolygonType>(26);
    EXPECT_THROW(ValidatePolygon(invalid), std::invalid_argument);
    invalid = polygons[0];
    invalid.vertices[0].x = std::numeric_limits<float>::infinity();
    EXPECT_THROW(ValidatePolygon(invalid), std::invalid_argument);
    invalid = polygons[0];
    invalid.bounciness = -1;
    EXPECT_THROW(ValidatePolygon(invalid), std::invalid_argument);
}
TEST(PolygonSimulationTest, MapEditsInvalidateFingerprintAndRejectInvalidReplacementAtomically)
{
    auto world = WorldWithPolygon(PMSPolygonType::Bouncy);
    auto state = world->GetStateManager();
    auto& map = state->GetMap();
    const auto hash = map.GetPolygonFingerprint();
    auto polygon = map.GetPolygons()[0];
    polygon.bounciness = 2.5F;
    map.SetPolygonsById({ { 0, polygon } });
    EXPECT_NE(map.GetPolygonFingerprint(), hash);
    const auto new_hash = map.GetPolygonFingerprint();
    polygon.polygon_type = static_cast<PMSPolygonType>(255);
    EXPECT_THROW(map.SetPolygonsById({ { 0, polygon } }), std::invalid_argument);
    EXPECT_EQ(map.GetPolygonFingerprint(), new_hash);
    const auto document = MapDocument::FromMap(map);
    const auto runtime = RuntimeMap::BuildFromDocument(document);
    EXPECT_EQ(runtime.GetMap().GetPolygons()[0].bounciness, 2.5F);
    EXPECT_EQ(document.GetMap().GetPolygonFingerprint(), new_hash);
}

TEST(PolygonSimulationTest, AllTypesSaveReloadAndProduceAnAcceptanceMap)
{
    Map map;
    map.CreateEmptyMap();
    map.SetTextureName("malina.png");
    map.SetDescription("All polygon types 0-25, five columns");
    map.SetJetCount(2000);
    for (unsigned int id = 0; id < 26; ++id) {
        PMSPolygon polygon{};
        polygon.polygon_type = static_cast<PMSPolygonType>(id);
        polygon.bounciness = id == 18 ? 2.5F : 1.0F;
        const float x = static_cast<float>(id % 5) * 300;
        const float y = static_cast<float>(id / 5) * 250;
        polygon.vertices[0].x = x - 100;
        polygon.vertices[0].y = y;
        polygon.vertices[1].x = x + 100;
        polygon.vertices[1].y = y;
        polygon.vertices[2].x = x;
        polygon.vertices[2].y = y + 100;
        for (auto& vertex : polygon.vertices) {
            vertex.texture_s = vertex.x / 100;
            vertex.texture_t = vertex.y / 100;
            vertex.color = PMSColor(static_cast<unsigned char>(80 + id * 5), 160, 180, 255);
        }
        map.AddNewPolygon(polygon);
    }
    // Extra acceptance lanes: three bounce strengths and overlapping background regions.
    for (unsigned int lane = 1; lane <= 3; ++lane) {
        auto polygon = map.GetPolygons()[18];
        polygon.bounciness = static_cast<float>(lane - 1) * 0.5F;
        const float x = static_cast<float>(lane) * 300;
        polygon.vertices[0].x = x - 100;
        polygon.vertices[1].x = x + 100;
        polygon.vertices[2].x = x;
        polygon.vertices[0].y = 1250;
        polygon.vertices[1].y = 1250;
        polygon.vertices[2].y = 1350;
        map.AddNewPolygon(polygon);
    }
    auto background = map.GetPolygons()[24];
    background.vertices[0].x = -100;
    background.vertices[0].y = 1500;
    background.vertices[1].x = 1300;
    background.vertices[1].y = 1500;
    background.vertices[2].x = 600;
    background.vertices[2].y = 2500;
    map.AddNewPolygon(background);
    auto transition = background;
    transition.polygon_type = PMSPolygonType::BackgroundTransition;
    transition.vertices[1].x = 100;
    transition.vertices[2].x = 0;
    transition.vertices[2].y = 1800;
    map.AddNewPolygon(transition);
    map.AddNewSpawnPoint({ .active = 1, .x = 0, .y = -100, .type = PMSSpawnPointType::General });
    map.AddNewSpawnPoint(
      { .active = 1, .x = -50, .y = -100, .type = PMSSpawnPointType::AlphaFlag });
    map.AddNewSpawnPoint({ .active = 1, .x = 0, .y = -100, .type = PMSSpawnPointType::BravoFlag });
    map.AddNewSpawnPoint(
      { .active = 1, .x = 50, .y = -100, .type = PMSSpawnPointType::YellowFlag });
    map.SaveMap("polygon-acceptance.pms");
    Map loaded;
    loaded.LoadMap("polygon-acceptance.pms");
    ASSERT_EQ(loaded.GetPolygonsCount(), 31U);
    for (unsigned int id = 0; id < 26; ++id) {
        EXPECT_EQ(static_cast<unsigned int>(loaded.GetPolygons()[id].polygon_type), id);
        EXPECT_NEAR(loaded.GetPolygons()[id].bounciness, id == 18 ? 2.5F : 1.0F, 0.00001F);
    }
    EXPECT_EQ(BuildPolygonRenderBatch(loaded.GetPolygons()).background_count, 4U);
    loaded.SaveMap("polygon-roundtrip.pms");
    Map second;
    second.LoadMap("polygon-roundtrip.pms");
    EXPECT_EQ(second.GetPolygonFingerprint(), loaded.GetPolygonFingerprint());
}

TEST(PolygonSimulationTest, MalformedSerializedTypeAndTruncationLeaveTheOldMapUntouched)
{
    auto world = WorldWithPolygon(PMSPolygonType::Normal);
    auto state = world->GetStateManager();
    state->GetMap().SaveMap("polygon-malformed-source.pms");
    std::ifstream input("polygon-malformed-source.pms", std::ios::binary);
    const std::string original((std::istreambuf_iterator<char>(input)),
                               std::istreambuf_iterator<char>());
    constexpr std::size_t header = sizeof(int) + 1 + DESCRIPTION_MAX_LENGTH + 1 +
                                   TEXTURE_NAME_MAX_LENGTH + 2 * sizeof(PMSColor) + sizeof(int) +
                                   4 * sizeof(unsigned char) + sizeof(int);
    const auto type_offset =
      header + sizeof(std::uint32_t) + 3 * sizeof(PMSVertex) + 3 * sizeof(PMSVector);
    ASSERT_LT(type_offset, original.size());
    auto bytes = original;
    bytes.at(type_offset) = static_cast<char>(255);
    {
        std::ofstream output("polygon-malformed.pms", std::ios::binary);
        output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    }
    const auto fingerprint = state->GetConstMap().GetPolygonFingerprint();
    EXPECT_THROW(state->GetMap().LoadMap("polygon-malformed.pms"), std::exception);
    EXPECT_EQ(state->GetConstMap().GetPolygonFingerprint(), fingerprint);
    {
        std::ofstream output("polygon-truncated.pms", std::ios::binary);
        output.write(original.data(), static_cast<std::streamsize>(type_offset));
    }
    EXPECT_THROW(state->GetMap().LoadMap("polygon-truncated.pms"), std::exception);
    EXPECT_EQ(state->GetConstMap().GetPolygonFingerprint(), fingerprint);
}

TEST(PolygonSimulationTest, ExplosiveContactSpawnsThenDetonatesThroughTheExistingProjectilePipeline)
{
    auto world = WorldWithPolygon(PMSPolygonType::Explosive);
    world->SpawnSoldier(1, Center(world->GetStateManager()->GetConstMap().GetPolygons()[0]));
    unsigned int exploded = 0;
    world->GetPhysicsEvents().projectile_event.AddObserver(
      [&](const ProjectilePhysicsEvent& event) {
          if (event.kind == ProjectileEventKind::ProjectileExploded) {
              ++exploded;
          }
      });
    world->Tick({ .tick = 0 });
    EXPECT_TRUE(world->GetSoldier(1).dead_meat);
    EXPECT_EQ(exploded, 0U);
    EXPECT_GE(world->GetStateManager()->GetBulletsCount(), 1U);
    for (unsigned int tick = 1; tick < 10; ++tick) {
        world->Tick({ .tick = tick });
    }
    EXPECT_EQ(exploded, 1U);
}

TEST(PolygonSimulationTest, BackgroundLookupUsesGlobalIdsAndCachedCenterContact)
{
    auto world = WorldWithPolygon(PMSPolygonType::Normal);
    auto state = world->GetStateManager();
    auto polygon = state->GetConstMap().GetPolygons()[0];
    polygon.polygon_type = PMSPolygonType::Background;
    state->GetMap().AddNewPolygon(polygon); // Background-list index 0 differs from global ID 1.
    auto& soldier = state->GetSoldier(1);
    soldier.polygon_background = {};
    PolygonContacts::Begin(state->GetConstMap(),
                           soldier.polygon_background,
                           soldier.background_touched,
                           Center(polygon));
    EXPECT_EQ(soldier.polygon_background.polygon, 1);
    PolygonContacts::Begin(state->GetConstMap(),
                           soldier.polygon_background,
                           soldier.background_touched,
                           Center(polygon));
    EXPECT_TRUE(soldier.background_touched);
    EXPECT_EQ(soldier.polygon_background.polygon, 1);
}

TEST(PolygonSimulationTest, SameSectorRaysAndNonDefaultSectorLayoutsUseActualBounds)
{
    MapData data;
    data.center_x = 100;
    data.center_y = -50;
    data.sectors_size = 10;
    data.sectors_count = 2;
    data.sectors_poly = std::vector<std::vector<PMSSector>>(5, std::vector<PMSSector>(5));
    PMSPolygon polygon{};
    polygon.bounciness = 1;
    polygon.vertices[0].x = 95;
    polygon.vertices[0].y = -55;
    polygon.vertices[1].x = 105;
    polygon.vertices[1].y = -55;
    polygon.vertices[2].x = 100;
    polygon.vertices[2].y = -45;
    data.polygons.push_back(polygon);
    data.sectors_poly[2][2].polygons.push_back(1);
    Map map(std::move(data));
    float distance = -1;
    EXPECT_TRUE(map.RayCast({ 100, -51 }, { 100, -51 }, distance, 100));
    EXPECT_EQ(distance, 0);
    EXPECT_TRUE(map.RayCast({ 96, -54 }, { 104, -46 }, distance, 100));
    EXPECT_FALSE(map.RayCast({ 1000, 1000 }, { 1001, 1001 }, distance, 100));
    EXPECT_FALSE(
      map.RayCast({ std::numeric_limits<float>::quiet_NaN(), 0 }, { 0, 0 }, distance, 100));
    EXPECT_EQ(map.GetSectorIndex({ 100, -50 }), glm::ivec2(2, 2));
}

TEST(PolygonSimulationTest, RespawnAndRestoredEditorStateCannotReuseLifeIdentities)
{
    auto world = WorldWithPolygon(PMSPolygonType::Normal);
    auto state = world->GetStateManager();
    const auto snapshot = state->CreateSoldiersSnapshot();
    const auto first = world->GetSoldier(1).life_sequence;
    world->SpawnSoldier(1, glm::vec2{});
    const auto second = world->GetSoldier(1).life_sequence;
    state->ApplySoldiersSnapshot(snapshot);
    world->SpawnSoldier(1, glm::vec2{});
    EXPECT_GT(second, first);
    EXPECT_GT(world->GetSoldier(1).life_sequence, second);
}

TEST(PolygonSimulationTest, RadiusProbeChecksTheThirdNormal)
{
    auto world = WorldWithPolygon(PMSPolygonType::Normal);
    auto state = world->GetStateManager();
    auto& soldier = state->GetSoldier(1);
    soldier.particle.SetVelocity({});
    // Only the third radius sample enters the right sloping edge of this triangle.
    soldier.particle.position = { 52, 0 };
    soldier.particle.old_position = { 52, 0 };
    EXPECT_TRUE(SoldierPhysics::CheckRadiusMapCollision(
      soldier, state->GetConstMap(), 52, 3, false, world->GetPhysicsEvents()));
    ASSERT_FALSE(soldier.polygon_contacts.empty());
    EXPECT_EQ(soldier.polygon_contacts.front().probe, PolygonProbe::Radius);
}

TEST(PolygonSimulationTest, ActualFlagThrowPreservesBackgroundAndRearmsCarrierRules)
{
    auto world = WorldWithPolygon(PMSPolygonType::NoCollide);
    auto state = world->GetStateManager();
    auto& soldier = state->GetSoldier(1);
    soldier.SetCeaseFireCounter(-1);
    soldier.control.mouse_aim_x = 1000;
    soldier.polygon_background = { .map_fingerprint = state->GetConstMap().GetPolygonFingerprint(),
                                   .phase = PolygonBackgroundPhase::Normal,
                                   .polygon = -1 };
    auto& flag = state->CreateItem({}, 255, ItemType::PointmatchFlag);
    ASSERT_TRUE(FlagCarry::TryAcquire(*state, soldier, flag));
    state->ThrowSoldierFlags(1);
    EXPECT_FALSE(soldier.is_holding_flags);
    EXPECT_EQ(flag.holding_soldier_id, -1);
    EXPECT_EQ(flag.polygon_background, soldier.polygon_background);
}

TEST(PolygonSimulationTest, OwnerDisconnectChangesItemFilteringWithoutDanglingActorAccess)
{
    auto world = WorldWithPolygon(PMSPolygonType::AlphaPlayers);
    auto state = world->GetStateManager();
    auto& item = state->CreateItem({}, 1, ItemType::USSOCOM);
    ItemPhysics::Update(*state, item, world->GetPhysicsEvents());
    EXPECT_TRUE(item.has_collision_owner);
    EXPECT_EQ(item.collision_owner_team, 1);
    state->RemoveSoldier(1);
    ItemPhysics::Update(*state, item, world->GetPhysicsEvents());
    EXPECT_FALSE(item.has_collision_owner);
    const auto center = Center(state->GetConstMap().GetPolygons()[0]);
    EXPECT_TRUE(ItemPhysics::CheckMapCollision(
      item, state->GetConstMap(), center.x, center.y + 0.5F, 1, world->GetPhysicsEvents()));
}

TEST(PolygonSimulationTest, GrenadeObstructionUsesItsPointAndProjectileQueryPurposes)
{
    constexpr std::array<bool, 26> allowed{ false, false, true,  true,  false, false, false,
                                            false, false, false, false, true,  false, true,
                                            false, true,  false, true,  false, false, false,
                                            true,  true,  true,  true,  true };
    for (unsigned int id = 0; id < 26; ++id) {
        auto world = WorldWithPolygon(static_cast<PMSPolygonType>(id));
        auto state = world->GetStateManager();
        world->SpawnSoldier(1, Center(state->GetConstMap().GetPolygons()[0]));
        auto& soldier = state->GetSoldier(1);
        soldier.SetCeaseFireCounter(-1);
        soldier.control.mouse_aim_x = 1000;
        soldier.body_animation = world->GetBodyAnimationState(AnimationType::Throw);
        soldier.body_animation->SetFrame(15);
        std::vector<BulletParams> grenades;
        const auto ammo = soldier.weapons[2].GetAmmoCount();
        EXPECT_EQ(WeaponPhysics::ThrowGrenade(soldier, state->GetConstMap(), grenades),
                  allowed.at(id))
          << id;
        EXPECT_EQ(soldier.weapons[2].GetAmmoCount(), ammo - (allowed.at(id) ? 1 : 0)) << id;
    }
}

TEST(PolygonSimulationTest, PredictedTicksNeverCommitEnvironmentalActions)
{
    auto world = WorldWithPolygon(PMSPolygonType::Explosive);
    auto state = world->GetStateManager();
    state->SetAuthoritativeProjectiles(false);
    world->SpawnSoldier(1, Center(state->GetConstMap().GetPolygons()[0]));
    unsigned int effects = 0;
    world->GetPhysicsEvents().polygon_event.AddObserver([&](const auto&) { ++effects; });
    world->UpdateSoldier(1);
    EXPECT_FALSE(world->GetSoldier(1).dead_meat);
    EXPECT_EQ(world->GetSoldier(1).health, 150);
    EXPECT_TRUE(state->GetBulletEmitter().empty());
    EXPECT_EQ(effects, 0U);
}

TEST(PolygonSimulationTest, ProjectileDeathWeaponDropInheritsBackground)
{
    auto world = WorldWithPolygon(PMSPolygonType::Background);
    auto state = world->GetStateManager();
    auto& soldier = state->GetSoldier(1);
    soldier.polygon_background = {
        .map_fingerprint = state->GetConstMap().GetPolygonFingerprint(),
        .phase = PolygonBackgroundPhase::Transition,
        .polygon = 0,
    };
    Bullet bullet{};
    BulletEffects::DropWeapon(*state, soldier, bullet);
    unsigned int drops = 0;
    state->ForEachItem([&](const Item& item) {
        ++drops;
        EXPECT_EQ(item.polygon_background, soldier.polygon_background);
    });
    EXPECT_EQ(drops, 1U);
}
