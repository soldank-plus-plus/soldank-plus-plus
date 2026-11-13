#include "core/math/Glm.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <expected>
#include <ios>
#include <memory>
#include <string>
#include <utility>
#include <vector>

import Shared.Core.Animations;
import Shared.Core.World;
import Shared.CoreEventHandler;
import Shared.Core.Data.IFileReader;
import Shared.Core.Entities.Bullet;
import Shared.Core.Entities.Item;
import Shared.Core.Entities.Soldier;
import Shared.Core.Entities.Weapon;
import Shared.Core.Entities.WeaponParameters;
import Shared.Core.Entities.WeaponParametersFactory;
import Shared.Core.Physics.SoldierPhysics;
import Shared.Core.Physics.Weapons.WeaponPhysics;
import Shared.Core.Physics.Weapons.StationaryWeaponPhysics;
import Shared.Core.Physics.Bullets.BulletEvents;
import Shared.Core.Physics.Bullets.BulletEffects;
import Shared.Core.Physics.Bullets.BulletLagCompensation;
import Shared.Core.Simulation.SimulationEvents;
import Shared.Networking.ProtocolConversions;
import Shared.Core.Map.Map;
import Shared.Core.Map.PMSEnums;
import Shared.Core.Map.PMSStructs;
import Shared.Core.Physics.BulletPhysics;
import Shared.Core.Physics.Bullets.BulletCollision;
import Shared.Core.Physics.Bullets.BulletDamage;
import Shared.Core.Physics.Bullets.BulletImpactResolver;
import Shared.Core.Physics.Bullets.BulletTypes;
import Shared.Core.Physics.Particles;
import Shared.Core.Physics.PhysicsEvents;
import Shared.Core.State.StateManager;
import Shared.Core.Types.BulletType;
import Shared.Core.Types.ItemType;
import Shared.Core.Types.TeamType;
import Shared.Core.Types.WeaponType;
import Testing.Framework.Shared.MapBuilder;

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

Soldank::Bullet CreateBullet(glm::vec2 position,
                             glm::vec2 velocity,
                             std::int16_t timeout = 100,
                             float hit_multiply = 1.0F)
{
    return Soldank::Bullet{ Soldank::BulletParams{
      .style = Soldank::BulletType::Bullet,
      .weapon = Soldank::WeaponType::Ak74,
      .position = position,
      .velocity = velocity,
      .timeout = timeout,
      .hit_multiply = hit_multiply,
      .team = Soldank::TeamType::None,
      .owner_id = 255,
      .push = 0.0F,
    } };
}

Soldank::StateManager CreateStateManager(Soldank::AnimationDataManager& animation_data_manager)
{
    return Soldank::StateManager(
      animation_data_manager,
      std::make_shared<Soldank::ParticleSystem>(std::vector<Soldank::Particle>{},
                                                std::vector<Soldank::Constraint>{}));
}

Soldank::StateManager CreateStateManagerWithSoldierSkeleton(
  Soldank::AnimationDataManager& animation_data_manager)
{
    std::vector<Soldank::Particle> particles;
    particles.reserve(23);
    for (int particle_index = 0; particle_index < 23; ++particle_index) {
        particles.emplace_back(true,
                               glm::vec2(0.0F, 0.0F),
                               glm::vec2(0.0F, 0.0F),
                               glm::vec2(0.0F, 0.0F),
                               glm::vec2(0.0F, 0.0F),
                               1.0F,
                               1.0F,
                               0.0F,
                               1.0F,
                               0.0F);
    }

    return Soldank::StateManager(
      animation_data_manager,
      std::make_shared<Soldank::ParticleSystem>(particles, std::vector<Soldank::Constraint>{}));
}

Soldank::AnimationDataManager CreateAnimationDataManager()
{
    AnimationDataReader animation_data_reader;
    Soldank::AnimationDataManager animation_data_manager;
    animation_data_manager.LoadAnimationData(
      Soldank::AnimationType::Stand, "stand.poa", true, 1, animation_data_reader);
    return animation_data_manager;
}

std::unique_ptr<Soldank::Map> CreateMapWithColliders(std::vector<Soldank::PMSCollider> colliders)
{
    Soldank::MapData map_data;
    map_data.sectors_size = 100;
    map_data.sectors_poly =
      std::vector<std::vector<Soldank::PMSSector>>(51, std::vector<Soldank::PMSSector>(51));
    map_data.colliders = std::move(colliders);
    return std::make_unique<Soldank::Map>(std::move(map_data));
}

std::shared_ptr<Soldank::ParticleSystem> CreateItemSkeleton(glm::vec2 position,
                                                            glm::vec2 old_position)
{
    return std::make_shared<Soldank::ParticleSystem>(
      std::vector<Soldank::Particle>{
        { true, position, old_position, {}, {}, 1.0F, 1.0F, 0.0F, 1.0F, 0.0F },
        { true, position, old_position, {}, {}, 1.0F, 1.0F, 0.0F, 1.0F, 0.0F },
      },
      std::vector<Soldank::Constraint>{});
}
} // namespace

TEST(BulletPhysicsTest, MovesBulletUsingParticleEulerIntegration)
{
    auto animation_data_manager = CreateAnimationDataManager();
    auto state_manager = CreateStateManager(animation_data_manager);
    auto map = SoldankTesting::MapBuilder::Empty()->Build();
    Soldank::PhysicsEvents physics_events;
    auto bullet = CreateBullet({ 0.0F, 0.0F }, { 10.0F, 0.0F });

    Soldank::BulletPhysics::UpdateBullet(physics_events, bullet, *map, state_manager);

    EXPECT_EQ(bullet.particle.old_position, (glm::vec2{ 0.0F, 0.0F }));
    EXPECT_EQ(bullet.particle.position, (glm::vec2{ 10.0F, 0.135F }));
    EXPECT_EQ(bullet.particle.GetVelocity(), (glm::vec2{ 9.9F, 0.13365F }));
}

TEST(BulletPhysicsTest, AppliesFlameBuoyancyForTheFollowingUpdate)
{
    auto animation_data_manager = CreateAnimationDataManager();
    auto state_manager = CreateStateManager(animation_data_manager);
    auto map = SoldankTesting::MapBuilder::Empty()->Build();
    Soldank::PhysicsEvents physics_events;
    auto bullet = CreateBullet({ 0.0F, 0.0F }, { 0.0F, 0.0F });
    bullet.style = Soldank::BulletType::Flame;

    Soldank::BulletPhysics::UpdateBullet(physics_events, bullet, *map, state_manager);

    EXPECT_FLOAT_EQ(bullet.particle.GetForce().y, -0.15F);

    Soldank::BulletPhysics::UpdateBullet(physics_events, bullet, *map, state_manager);

    EXPECT_NEAR(bullet.particle.position.y, 0.25365F, 0.00001F);
    EXPECT_NEAR(bullet.particle.GetVelocity().y, 0.1174635F, 0.00001F);
    EXPECT_FLOAT_EQ(bullet.particle.GetForce().y, -0.15F);
}

TEST(BulletPhysicsTest, FindsTheNearestActiveCircularMapCollider)
{
    auto map = CreateMapWithColliders({
      { .active = 0, .x = -15.0F, .y = 0.0F, .radius = 5.0F },
      { .active = 1, .x = -10.0F, .y = 0.0F, .radius = 5.0F },
      { .active = 1, .x = 0.0F, .y = 0.0F, .radius = 5.0F },
    });
    auto bullet = CreateBullet({ -20.0F, 0.0F }, { 40.0F, 0.0F });

    bullet.particle.Euler();
    const auto collision = Soldank::BulletCollision::FindColliderCollision(bullet, *map);

    ASSERT_TRUE(collision.has_value());
    EXPECT_EQ(collision->kind, Soldank::BulletCollisionKind::MapCollider);
    ASSERT_TRUE(collision->collider_id.has_value());
    EXPECT_EQ(*collision->collider_id, 1U);
    EXPECT_NEAR(collision->position.x, -12.94F, 0.02F);
    EXPECT_NEAR(collision->distance, 7.06F, 0.02F);
}

TEST(BulletPhysicsTest, CircularMapColliderBlocksProjectileDuringUpdate)
{
    auto animation_data_manager = CreateAnimationDataManager();
    auto state_manager = CreateStateManager(animation_data_manager);
    auto map = CreateMapWithColliders({
      { .active = 1, .x = 0.0F, .y = 0.0F, .radius = 5.0F },
    });
    Soldank::PhysicsEvents physics_events;
    auto bullet = CreateBullet({ -20.0F, 0.0F }, { 40.0F, 0.0F });

    Soldank::BulletPhysics::UpdateBullet(physics_events, bullet, *map, state_manager);

    EXPECT_FALSE(bullet.active);
    EXPECT_NEAR(bullet.particle.position.x, -2.94F, 0.02F);
}

TEST(BulletPhysicsTest, PushesMovableItemsUsingRelativeVelocityAndPerItemCooldown)
{
    auto animation_data_manager = CreateAnimationDataManager();
    auto state_manager = CreateStateManager(animation_data_manager);
    auto item_skeleton = CreateItemSkeleton({ 0.0F, 0.0F }, { 0.0F, 0.0F });
    Soldank::Item& item =
      state_manager.CreateItem({ 0.0F, 0.0F }, 255, Soldank::ItemType::Ak74, item_skeleton);
    item_skeleton->SetOldPos(1, { -2.0F, 0.0F });
    auto sweeping_bullet = CreateBullet({ -20.0F, 0.0F }, { 40.0F, 0.0F });
    sweeping_bullet.timeout = 98;
    sweeping_bullet.particle.Euler();
    const auto found_collision =
      Soldank::BulletCollision::FindThingCollision(sweeping_bullet, state_manager);
    ASSERT_TRUE(found_collision.has_value());
    ASSERT_TRUE(found_collision->item_id.has_value());
    ASSERT_TRUE(found_collision->item_particle_id.has_value());
    EXPECT_EQ(*found_collision->item_id, item.id);
    EXPECT_EQ(*found_collision->item_particle_id, 1U);

    auto bullet = CreateBullet({ 0.0F, 0.0F }, { 10.0F, 0.0F });
    bullet.push = 0.5F;
    const Soldank::BulletCollisionResult collision{
        .kind = Soldank::BulletCollisionKind::Item,
        .position = { 0.0F, 0.0F },
        .distance = 0.0F,
        .item_id = item.id,
        .item_particle_id = 1,
    };
    Soldank::PhysicsEvents physics_events;

    state_manager.SetGameTick(10);
    Soldank::BulletImpactResolver::ResolveImpact(physics_events, bullet, state_manager, collision);

    EXPECT_TRUE(bullet.active);
    EXPECT_FALSE(item.static_type);
    EXPECT_FLOAT_EQ(item_skeleton->GetPos(1).x, 36.0F);
    ASSERT_EQ(bullet.item_collision_cooldowns.size(), 1U);
    EXPECT_EQ(bullet.item_collision_cooldowns.at(0).item_id, item.id);
    EXPECT_EQ(bullet.item_collision_cooldowns.at(0).cooldown_end_tick, 70U);

    item.static_type = true;
    item_skeleton->SetPos(1, { 0.0F, 0.0F });
    item_skeleton->SetOldPos(1, { -2.0F, 0.0F });
    state_manager.SetGameTick(11);
    Soldank::BulletImpactResolver::ResolveImpact(physics_events, bullet, state_manager, collision);

    EXPECT_FLOAT_EQ(item_skeleton->GetPos(1).x, 0.0F);
    EXPECT_TRUE(item.static_type);

    state_manager.SetGameTick(70);
    Soldank::BulletImpactResolver::ResolveImpact(physics_events, bullet, state_manager, collision);

    EXPECT_FLOAT_EQ(item_skeleton->GetPos(1).x, 36.0F);
    EXPECT_FALSE(item.static_type);
}

TEST(BulletPhysicsTest, DeactivatesBulletWhenItsLifetimeExpires)
{
    auto animation_data_manager = CreateAnimationDataManager();
    auto state_manager = CreateStateManager(animation_data_manager);
    auto map = SoldankTesting::MapBuilder::Empty()->Build();
    Soldank::PhysicsEvents physics_events;
    auto bullet = CreateBullet({ 0.0F, 0.0F }, { 0.0F, 0.0F }, 1);

    Soldank::BulletPhysics::UpdateBullet(physics_events, bullet, *map, state_manager);

    EXPECT_FALSE(bullet.active);
    EXPECT_EQ(bullet.timeout_prev, 1);
    EXPECT_EQ(bullet.timeout, 0);
}

TEST(BulletPhysicsTest, DeactivatesBulletOutsideMapBounds)
{
    auto animation_data_manager = CreateAnimationDataManager();
    auto state_manager = CreateStateManager(animation_data_manager);
    auto map = SoldankTesting::MapBuilder::Empty()->Build();
    Soldank::PhysicsEvents physics_events;
    auto bullet = CreateBullet({ 1'000.0F, 1'000.0F }, { 0.0F, 0.0F });

    Soldank::BulletPhysics::UpdateBullet(physics_events, bullet, *map, state_manager);

    EXPECT_FALSE(bullet.active);
}

TEST(BulletPhysicsTest, DetectsMapCollisionAlongBulletSweepAndEmitsEvent)
{
    auto animation_data_manager = CreateAnimationDataManager();
    auto state_manager = CreateStateManager(animation_data_manager);
    auto map =
      SoldankTesting::MapBuilder::Empty()
        ->AddPolygon(
          { -5.0F, -10.0F }, { 5.0F, -10.0F }, { 0.0F, 10.0F }, Soldank::PMSPolygonType::Normal)
        ->Build();
    Soldank::PhysicsEvents physics_events;
    int collision_count = 0;
    glm::vec2 collision_position{};
    physics_events.bullet_collides_with_polygon.AddObserver(
      [&](const Soldank::Bullet&, const glm::vec2& position) {
          ++collision_count;
          collision_position = position;
      });
    auto bullet = CreateBullet({ -20.0F, 0.0F }, { 40.0F, 0.0F });

    EXPECT_EQ(map->GetSectorsPerAxis(), 51);

    Soldank::BulletPhysics::UpdateBullet(physics_events, bullet, *map, state_manager);

    EXPECT_FALSE(bullet.active);
    EXPECT_EQ(collision_count, 1);
    EXPECT_LT(collision_position.x, 0.0F);
    EXPECT_GT(collision_position.y, 0.0F);
    EXPECT_LT(collision_position.y, 0.135F);
}

TEST(BulletPhysicsTest, RicochetsEligibleProjectilesAwayFromMapSurfaces)
{
    constexpr auto RICOCHETING_STYLES = std::array{
        Soldank::BulletType::Bullet,     Soldank::BulletType::GaugeBullet,
        Soldank::BulletType::Fist,       Soldank::BulletType::Blade,
        Soldank::BulletType::M2Bullet,   Soldank::BulletType::M79Grenade,
        Soldank::BulletType::FlameArrow, Soldank::BulletType::LAWMissile,
    };

    for (const Soldank::BulletType style : RICOCHETING_STYLES) {
        auto animation_data_manager = CreateAnimationDataManager();
        auto state_manager = CreateStateManager(animation_data_manager);
        auto map = SoldankTesting::MapBuilder::Empty()
                     ->AddPolygon({ -100.0F, 0.0F },
                                  { 100.0F, 0.0F },
                                  { 100.0F, 100.0F },
                                  Soldank::PMSPolygonType::Normal)
                     ->Build();
        Soldank::PhysicsEvents physics_events;
        const float surface_y = map->GetPolygons().at(0).vertices.at(0).y;
        auto bullet = CreateBullet({ -20.0F, surface_y - 1.0F }, { 40.0F, 2.0F });
        bullet.style = style;
        bullet.hit_spot = { -100.0F, surface_y };

        Soldank::BulletPhysics::UpdateBullet(physics_events, bullet, *map, state_manager);

        EXPECT_TRUE(bullet.active);
        EXPECT_GT(bullet.particle.GetVelocity().x, 0.0F);
        EXPECT_LT(bullet.particle.GetVelocity().y, 0.0F);
        EXPECT_LT(glm::length(bullet.particle.GetVelocity()), 40.0F);
        EXPECT_EQ(bullet.ricochet_count, 1);
    }
}

TEST(BulletPhysicsTest, BouncesFragGrenadesAndFlamesWithDamping)
{
    constexpr auto BOUNCING_STYLES =
      std::array{ Soldank::BulletType::FragGrenade, Soldank::BulletType::Flame };

    for (const Soldank::BulletType style : BOUNCING_STYLES) {
        auto animation_data_manager = CreateAnimationDataManager();
        auto state_manager = CreateStateManager(animation_data_manager);
        auto map =
          SoldankTesting::MapBuilder::Empty()
            ->AddPolygon(
              { -5.0F, -10.0F }, { 5.0F, -10.0F }, { 0.0F, 10.0F }, Soldank::PMSPolygonType::Normal)
            ->Build();
        Soldank::PhysicsEvents physics_events;
        auto bullet = CreateBullet({ -20.0F, 0.0F }, { 40.0F, 0.0F });
        bullet.style = style;

        Soldank::BulletPhysics::UpdateBullet(physics_events, bullet, *map, state_manager);

        EXPECT_TRUE(bullet.active);
        EXPECT_LT(bullet.particle.GetVelocity().x, 0.0F);
        EXPECT_LT(glm::length(bullet.particle.GetVelocity()), 40.0F);
        if (style == Soldank::BulletType::Flame) {
            EXPECT_EQ(bullet.timeout, 15);
        }
    }
}

TEST(BulletPhysicsTest, MakesArrowsResistMapImpacts)
{
    auto animation_data_manager = CreateAnimationDataManager();
    auto state_manager = CreateStateManager(animation_data_manager);
    auto map =
      SoldankTesting::MapBuilder::Empty()
        ->AddPolygon(
          { -5.0F, -10.0F }, { 5.0F, -10.0F }, { 0.0F, 10.0F }, Soldank::PMSPolygonType::Normal)
        ->Build();
    Soldank::PhysicsEvents physics_events;
    auto bullet = CreateBullet({ -20.0F, 0.0F }, { 40.0F, 0.0F }, 400);
    bullet.style = Soldank::BulletType::Arrow;

    Soldank::BulletPhysics::UpdateBullet(physics_events, bullet, *map, state_manager);

    EXPECT_TRUE(bullet.active);
    EXPECT_EQ(bullet.timeout, 279);
    EXPECT_LT(bullet.particle.GetForce().y, 0.0F);
}

TEST(BulletPhysicsTest, DeactivatesClusterProjectilesAndCreatesKnifeItemsOnMapImpact)
{
    constexpr auto DETONATING_STYLES =
      std::array{ Soldank::BulletType::ClusterGrenade, Soldank::BulletType::Cluster };

    for (const Soldank::BulletType style : DETONATING_STYLES) {
        auto animation_data_manager = CreateAnimationDataManager();
        auto state_manager = CreateStateManager(animation_data_manager);
        auto map =
          SoldankTesting::MapBuilder::Empty()
            ->AddPolygon(
              { -5.0F, -10.0F }, { 5.0F, -10.0F }, { 0.0F, 10.0F }, Soldank::PMSPolygonType::Normal)
            ->Build();
        Soldank::PhysicsEvents physics_events;
        auto bullet = CreateBullet({ -20.0F, 0.0F }, { 40.0F, 0.0F });
        bullet.style = style;

        Soldank::BulletPhysics::UpdateBullet(physics_events, bullet, *map, state_manager);

        EXPECT_FALSE(bullet.active);
    }

    auto animation_data_manager = CreateAnimationDataManager();
    auto state_manager = CreateStateManager(animation_data_manager);
    auto map =
      SoldankTesting::MapBuilder::Empty()
        ->AddPolygon(
          { -5.0F, -10.0F }, { 5.0F, -10.0F }, { 0.0F, 10.0F }, Soldank::PMSPolygonType::Normal)
        ->Build();
    Soldank::PhysicsEvents physics_events;
    auto bullet = CreateBullet({ -20.0F, 0.0F }, { 40.0F, 0.0F });
    bullet.style = Soldank::BulletType::ThrownKnife;

    Soldank::BulletPhysics::UpdateBullet(physics_events, bullet, *map, state_manager);

    std::size_t knife_count = 0;
    state_manager.ForEachItem([&knife_count](const Soldank::Item& item) {
        if (item.style == Soldank::ItemType::Knife) {
            ++knife_count;
        }
    });
    EXPECT_FALSE(bullet.active);
    EXPECT_EQ(knife_count, 1U);
}

TEST(BulletPhysicsTest, MapCollisionResultContainsImpactDistanceAndTargetMetadata)
{
    auto map =
      SoldankTesting::MapBuilder::Empty()
        ->AddPolygon(
          { -5.0F, -10.0F }, { 5.0F, -10.0F }, { 0.0F, 10.0F }, Soldank::PMSPolygonType::Normal)
        ->Build();
    auto bullet = CreateBullet({ -20.0F, 0.0F }, { 40.0F, 0.0F });

    bullet.particle.Euler();
    const auto collision = Soldank::BulletCollision::FindMapCollision(bullet, *map);

    ASSERT_TRUE(collision.has_value());
    EXPECT_EQ(collision->kind, Soldank::BulletCollisionKind::MapPolygon);
    EXPECT_GT(collision->distance, 0.0F);
    ASSERT_TRUE(collision->surface_normal.has_value());
    EXPECT_LT(glm::dot(*collision->surface_normal, bullet.particle.GetVelocity()), 0.0F);
    EXPECT_TRUE(collision->polygon_id.has_value());
    EXPECT_EQ(*collision->polygon_id, 0U);
    EXPECT_FALSE(collision->soldier_id.has_value());
    EXPECT_FALSE(collision->body_part_id.has_value());
}

TEST(BulletPhysicsTest, SoldierCollisionResultContainsBodyPartAndImpactPosition)
{
    auto animation_data_manager = CreateAnimationDataManager();
    auto state_manager = CreateStateManagerWithSoldierSkeleton(animation_data_manager);
    auto soldiers = state_manager.CreateEmptySoldiersSnapshot();
    soldiers.at(0).active = true;
    state_manager.ApplySoldiersSnapshot(soldiers);
    state_manager.TransformSoldier(0, [](Soldank::Soldier& soldier) {
        soldier.dead_meat = true;
        soldier.particle.position = { 0.0F, 0.0F };
        for (unsigned int body_part_id = 1; body_part_id <= 16; ++body_part_id) {
            soldier.skeleton->SetPos(body_part_id, { 0.0F, 0.0F });
        }
    });
    auto bullet = CreateBullet({ -10.0F, 0.0F }, { 20.0F, 0.0F });
    bullet.particle.Euler();

    const auto collision =
      Soldank::BulletCollision::FindSoldierCollision(bullet, state_manager, -1.0F);

    ASSERT_TRUE(collision.has_value());
    EXPECT_EQ(collision->kind, Soldank::BulletCollisionKind::Soldier);
    EXPECT_EQ(collision->soldier_id, 0);
    EXPECT_EQ(collision->body_part_id, 12);
    EXPECT_NEAR(collision->position.x, -7.0F, 0.01F);
    EXPECT_NEAR(collision->position.y, 0.02F, 0.01F);
    EXPECT_NEAR(collision->distance, 3.0F, 0.01F);
}

TEST(BulletPhysicsTest, SelectsNearestSoldierCollisionRatherThanFirstActiveSoldier)
{
    auto animation_data_manager = CreateAnimationDataManager();
    auto state_manager = CreateStateManagerWithSoldierSkeleton(animation_data_manager);
    auto soldiers = state_manager.CreateEmptySoldiersSnapshot();
    soldiers.at(0).active = true;
    soldiers.at(1).active = true;
    state_manager.ApplySoldiersSnapshot(soldiers);

    const auto position_skeleton = [](Soldank::Soldier& soldier, glm::vec2 position) {
        soldier.dead_meat = true;
        soldier.particle.position = position;
        soldier.skeleton = std::make_shared<Soldank::ParticleSystem>(
          std::vector<Soldank::Particle>(
            23, Soldank::Particle(true, position, position, {}, {}, 1.0F, 1.0F, 0.0F, 1.0F, 0.0F)),
          std::vector<Soldank::Constraint>{});
        for (unsigned int body_part_id = 1; body_part_id <= 16; ++body_part_id) {
            soldier.skeleton->SetPos(body_part_id, position);
        }
    };
    state_manager.TransformSoldier(
      0, [&](Soldank::Soldier& soldier) { position_skeleton(soldier, { 20.0F, 0.0F }); });
    state_manager.TransformSoldier(
      1, [&](Soldank::Soldier& soldier) { position_skeleton(soldier, { 0.0F, 0.0F }); });
    auto bullet = CreateBullet({ -10.0F, 0.0F }, { 40.0F, 0.0F });
    bullet.particle.Euler();

    const auto collision =
      Soldank::BulletCollision::FindSoldierCollision(bullet, state_manager, -1.0F);

    ASSERT_TRUE(collision.has_value());
    EXPECT_EQ(collision->soldier_id, 1);
    EXPECT_LT(collision->distance, 20.0F);
}

TEST(BulletPhysicsTest, ResolvesNearSoldierBeforeContinuingToWall)
{
    auto animation_data_manager = CreateAnimationDataManager();
    auto state_manager = CreateStateManagerWithSoldierSkeleton(animation_data_manager);
    auto soldiers = state_manager.CreateEmptySoldiersSnapshot();
    soldiers.at(0).active = true;
    state_manager.ApplySoldiersSnapshot(soldiers);
    state_manager.TransformSoldier(0, [](Soldank::Soldier& soldier) {
        soldier.dead_meat = true;
        soldier.particle.position = { -5.0F, 0.0F };
        for (unsigned int body_part_id = 1; body_part_id <= 16; ++body_part_id) {
            soldier.skeleton->SetPos(body_part_id, { -5.0F, 0.0F });
        }
    });
    auto map =
      SoldankTesting::MapBuilder::Empty()
        ->AddPolygon(
          { 5.0F, -10.0F }, { 15.0F, -10.0F }, { 10.0F, 10.0F }, Soldank::PMSPolygonType::Normal)
        ->Build();
    Soldank::PhysicsEvents physics_events;
    int soldier_hit_count = 0;
    int map_hit_count = 0;
    physics_events.soldier_hit_by_bullet.AddObserver(
      [&](Soldank::Soldier&, float) { ++soldier_hit_count; });
    physics_events.bullet_collides_with_polygon.AddObserver(
      [&](const Soldank::Bullet&, const glm::vec2&) { ++map_hit_count; });
    auto bullet = CreateBullet({ -20.0F, 0.0F }, { 40.0F, 0.0F });

    Soldank::BulletPhysics::UpdateBullet(physics_events, bullet, *map, state_manager);

    EXPECT_EQ(soldier_hit_count, 1);
    EXPECT_EQ(map_hit_count, 1);
}

TEST(BulletPhysicsTest, MapBulletCollisionPolicyCoversEverySpecialPolygonType)
{
    constexpr std::array ALWAYS_COLLIDABLE{
        Soldank::PMSPolygonType::Normal,    Soldank::PMSPolygonType::Ice,
        Soldank::PMSPolygonType::Deadly,    Soldank::PMSPolygonType::BloodyDeadly,
        Soldank::PMSPolygonType::Hurts,     Soldank::PMSPolygonType::Regenerates,
        Soldank::PMSPolygonType::Lava,      Soldank::PMSPolygonType::Bouncy,
        Soldank::PMSPolygonType::Explosive, Soldank::PMSPolygonType::HurtFlaggers,
    };

    for (const auto polygon_type : ALWAYS_COLLIDABLE) {
        EXPECT_TRUE(Soldank::Map::BulletCollidesWithPolygon(polygon_type, 0));
    }

    EXPECT_TRUE(
      Soldank::Map::BulletCollidesWithPolygon(Soldank::PMSPolygonType::OnlyBulletsCollide, 0));
    EXPECT_FALSE(
      Soldank::Map::BulletCollidesWithPolygon(Soldank::PMSPolygonType::OnlyPlayersCollide, 0));
    EXPECT_FALSE(Soldank::Map::BulletCollidesWithPolygon(Soldank::PMSPolygonType::NoCollide, 0));
    EXPECT_FALSE(
      Soldank::Map::BulletCollidesWithPolygon(Soldank::PMSPolygonType::FlaggerCollides, 0));
    EXPECT_FALSE(
      Soldank::Map::BulletCollidesWithPolygon(Soldank::PMSPolygonType::NonFlaggerCollides, 0));
    EXPECT_FALSE(Soldank::Map::BulletCollidesWithPolygon(Soldank::PMSPolygonType::Background, 0));
    EXPECT_FALSE(
      Soldank::Map::BulletCollidesWithPolygon(Soldank::PMSPolygonType::BackgroundTransition, 0));

    EXPECT_FALSE(Soldank::Map::BulletCollidesWithPolygon(Soldank::PMSPolygonType::FlagCollides, 0));
    constexpr std::array TEAM_BULLET_TYPES{
        Soldank::PMSPolygonType::AlphaBullets,
        Soldank::PMSPolygonType::BravoBullets,
        Soldank::PMSPolygonType::CharlieBullets,
        Soldank::PMSPolygonType::DeltaBullets,
    };
    constexpr std::array TEAM_PLAYER_TYPES{
        Soldank::PMSPolygonType::AlphaPlayers,
        Soldank::PMSPolygonType::BravoPlayers,
        Soldank::PMSPolygonType::CharliePlayers,
        Soldank::PMSPolygonType::DeltaPlayers,
    };

    for (std::uint8_t team_id = 1; team_id <= 4; ++team_id) {
        for (std::size_t type_index = 0; type_index < TEAM_BULLET_TYPES.size(); ++type_index) {
            EXPECT_EQ(
              Soldank::Map::BulletCollidesWithPolygon(TEAM_BULLET_TYPES.at(type_index), team_id),
              type_index == static_cast<std::size_t>(team_id - 1));
            EXPECT_FALSE(
              Soldank::Map::BulletCollidesWithPolygon(TEAM_PLAYER_TYPES.at(type_index), team_id));
        }
    }
}

TEST(BulletPhysicsTest, MapSweepIgnoresBulletExcludedPolygonTypes)
{
    constexpr std::array EXCLUDED_POLYGON_TYPES{
        Soldank::PMSPolygonType::FlagCollides,
        Soldank::PMSPolygonType::OnlyPlayersCollide,
        Soldank::PMSPolygonType::NoCollide,
        Soldank::PMSPolygonType::FlaggerCollides,
        Soldank::PMSPolygonType::NonFlaggerCollides,
        Soldank::PMSPolygonType::Background,
        Soldank::PMSPolygonType::BackgroundTransition,
    };

    for (const auto polygon_type : EXCLUDED_POLYGON_TYPES) {
        auto map =
          SoldankTesting::MapBuilder::Empty()
            ->AddPolygon({ -5.0F, -10.0F }, { 5.0F, -10.0F }, { 0.0F, 10.0F }, polygon_type)
            ->Build();
        auto bullet = CreateBullet({ -20.0F, 0.0F }, { 40.0F, 0.0F });

        bullet.particle.Euler();

        EXPECT_FALSE(Soldank::BulletCollision::FindMapCollision(bullet, *map).has_value());
    }
}

TEST(BulletPhysicsTest, PreservesProjectilesForEligiblePenetratingHits)
{
    EXPECT_EQ(Soldank::BulletDamage::GetPenetrationVelocityMultiplier(true, true, 30.0F, 30.0F),
              0.9F);
    EXPECT_EQ(Soldank::BulletDamage::GetPenetrationVelocityMultiplier(false, true, 10.0F, 30.0F),
              0.75F);
    EXPECT_EQ(Soldank::BulletDamage::GetPenetrationVelocityMultiplier(false, false, 24.0F, 30.0F),
              0.75F);
    EXPECT_EQ(Soldank::BulletDamage::GetPenetrationVelocityMultiplier(false, false, 20.0F, 20.0F),
              0.66F);
    EXPECT_FALSE(
      Soldank::BulletDamage::GetPenetrationVelocityMultiplier(false, false, 10.0F, 30.0F));
}

TEST(BulletPhysicsTest, KeepsBulletActiveAfterPenetratingDeadSoldier)
{
    auto animation_data_manager = CreateAnimationDataManager();
    auto state_manager = CreateStateManagerWithSoldierSkeleton(animation_data_manager);
    auto soldiers = state_manager.CreateEmptySoldiersSnapshot();
    soldiers.at(0).active = true;
    state_manager.ApplySoldiersSnapshot(soldiers);
    state_manager.TransformSoldier(0, [](Soldank::Soldier& soldier) {
        soldier.dead_meat = true;
        soldier.particle.position = { 0.0F, 0.0F };
        for (unsigned int body_part_id = 1; body_part_id <= 16; ++body_part_id) {
            soldier.skeleton->SetPos(body_part_id, { 0.0F, 0.0F });
        }
    });
    auto map = SoldankTesting::MapBuilder::Empty()->Build();
    Soldank::PhysicsEvents physics_events;
    auto bullet = CreateBullet({ -20.0F, 0.0F }, { 40.0F, 0.0F });

    Soldank::BulletPhysics::UpdateBullet(physics_events, bullet, *map, state_manager);

    EXPECT_TRUE(bullet.active);
    EXPECT_NEAR(bullet.particle.GetVelocity().x, 35.64F, 0.01F);
}

TEST(BulletPhysicsTest, ReducesDamageMultiplierAfterTravellingPastFirstThreshold)
{
    auto animation_data_manager = CreateAnimationDataManager();
    auto state_manager = CreateStateManager(animation_data_manager);
    auto map = CreateMapWithColliders({});
    Soldank::PhysicsEvents physics_events;
    auto bullet = CreateBullet({ 0.0F, 0.0F }, { 501.0F, 0.0F }, 7, 2.0F);

    Soldank::BulletPhysics::UpdateBullet(physics_events, bullet, *map, state_manager);

    EXPECT_EQ(bullet.degrade_count, 1U);
    EXPECT_FLOAT_EQ(bullet.hit_multiply_prev, 2.0F);
    EXPECT_FLOAT_EQ(bullet.hit_multiply, 1.0F);
}

TEST(BulletPhysicsTest, DelaysAndAccumulatesProjectilePushes)
{
    Soldank::Soldier soldier;
    soldier.SetPingTicks(4);
    soldier.ScheduleProjectilePush(10, 3, { 2.0F, 1.0F });
    soldier.ScheduleProjectilePush(10, 3, { 1.0F, -1.0F });
    soldier.ApplyProjectilePushes(15);
    EXPECT_EQ(soldier.particle.GetVelocity(), glm::vec2(0.0F));
    soldier.ApplyProjectilePushes(16);
    EXPECT_EQ(soldier.particle.GetVelocity(), glm::vec2(3.0F, 0.0F));
    soldier.ApplyProjectilePushes(17);
    EXPECT_EQ(soldier.particle.GetVelocity(), glm::vec2(3.0F, 0.0F));
    EXPECT_TRUE(soldier.projectile_pushes.empty());
}

TEST(BulletPhysicsTest, CapsProjectilePushLatency)
{
    Soldank::Soldier soldier;
    soldier.ScheduleProjectilePush(10, 1000, { 2.0F, 0.0F });
    soldier.ApplyProjectilePushes(134);
    EXPECT_EQ(soldier.particle.GetVelocity(), glm::vec2(0.0F));
    soldier.ApplyProjectilePushes(135);
    EXPECT_EQ(soldier.particle.GetVelocity(), glm::vec2(2.0F, 0.0F));
}

TEST(BulletPhysicsTest, FiltersProtectedAndAlreadyPenetratedSoldiers)
{
    auto animation_data_manager = CreateAnimationDataManager();
    auto state_manager = CreateStateManagerWithSoldierSkeleton(animation_data_manager);
    auto soldiers = state_manager.CreateEmptySoldiersSnapshot();
    soldiers.at(0).active = true;
    state_manager.ApplySoldiersSnapshot(soldiers);
    auto bullet = CreateBullet({ -20.0F, 0.0F }, { 40.0F, 0.0F });
    bullet.particle.position = { 20.0F, 0.0F };
    auto& soldier = state_manager.GetSoldier(0);
    for (unsigned int part = 1; part <= 16; ++part) {
        soldier.skeleton->SetPos(part, { 0.0F, 0.0F });
    }
    EXPECT_TRUE(Soldank::BulletCollision::FindSoldierCollision(bullet, state_manager, -1.0F));
    soldier.SetTeam(Soldank::TeamType::Spectator);
    EXPECT_FALSE(Soldank::BulletCollision::FindSoldierCollision(bullet, state_manager, -1.0F));
    soldier.SetTeam(Soldank::TeamType::None);
    soldier.SetCeaseFireCounter(0);
    EXPECT_FALSE(Soldank::BulletCollision::FindSoldierCollision(bullet, state_manager, -1.0F));
    soldier.SetCeaseFireCounter(-1);
    bullet.owner_id = 0;
    EXPECT_FALSE(Soldank::BulletCollision::FindSoldierCollision(bullet, state_manager, -1.0F));
    bullet.timeout -= 21;
    EXPECT_TRUE(Soldank::BulletCollision::FindSoldierCollision(bullet, state_manager, -1.0F));
    bullet.penetrated_soldiers.push_back(0);
    EXPECT_FALSE(Soldank::BulletCollision::FindSoldierCollision(bullet, state_manager, -1.0F));
}

TEST(BulletPhysicsTest, PenetratesTwoTargetsInOneTickWithoutRepeatedDamage)
{
    auto animation_data_manager = CreateAnimationDataManager();
    auto state_manager = CreateStateManagerWithSoldierSkeleton(animation_data_manager);
    auto soldiers = state_manager.CreateEmptySoldiersSnapshot();
    soldiers.at(0).active = true;
    soldiers.at(1).active = true;
    state_manager.ApplySoldiersSnapshot(soldiers);
    for (unsigned int id = 0; id < 2; ++id) {
        auto& soldier = state_manager.GetSoldier(id);
        soldier.dead_meat = true;
        const glm::vec2 position{ static_cast<float>(id) * 25.0F, 0.0F };
        soldier.particle.position = position;
        for (unsigned int part = 1; part <= 16; ++part) {
            soldier.skeleton->SetPos(part, position);
        }
    }
    auto map = SoldankTesting::MapBuilder::Empty()->Build();
    Soldank::PhysicsEvents physics_events;
    unsigned int hits = 0;
    physics_events.soldier_hit_by_bullet.AddObserver([&](Soldank::Soldier&, float) { ++hits; });
    auto bullet = CreateBullet({ -20.0F, 0.0F }, { 80.0F, 0.0F });
    Soldank::BulletPhysics::UpdateBullet(physics_events, bullet, *map, state_manager);
    EXPECT_EQ(hits, 2U);
    EXPECT_TRUE(bullet.active);
    EXPECT_NEAR(bullet.particle.GetVelocity().x, 80.0F * 0.99F * 0.9F * 0.9F, 0.01F);
    EXPECT_EQ(bullet.penetrated_soldiers.size(), 2U);
}

TEST(BulletPhysicsTest, DirectDamageUsesProjectileFalloffMultiplier)
{
    auto animation_data_manager = CreateAnimationDataManager();
    auto state_manager = CreateStateManagerWithSoldierSkeleton(animation_data_manager);
    auto soldiers = state_manager.CreateEmptySoldiersSnapshot();
    soldiers.at(0).active = true;
    state_manager.ApplySoldiersSnapshot(soldiers);
    Soldank::PhysicsEvents physics_events;
    std::vector<float> damages;
    physics_events.soldier_hit_by_bullet.AddObserver(
      [&](Soldank::Soldier&, float damage) { damages.push_back(damage); });
    auto bullet = CreateBullet({ -20.0F, 0.0F }, { 40.0F, 0.0F });
    const Soldank::BulletCollisionResult collision{
        .kind = Soldank::BulletCollisionKind::Soldier,
        .position = { -7.0F, 0.0F },
        .distance = 13.0F,
        .soldier_id = 0,
        .body_part_id = 12,
    };
    Soldank::BulletDamage::ApplyDirectHit(physics_events, bullet, state_manager, collision);
    bullet.particle.SetVelocity({ 40.0F, 0.0F });
    bullet.hit_multiply *= 0.5F;
    Soldank::BulletDamage::ApplyDirectHit(physics_events, bullet, state_manager, collision);
    ASSERT_EQ(damages.size(), 2U);
    EXPECT_GT(damages.at(0), 0.0F);
    EXPECT_FLOAT_EQ(damages.at(1), damages.at(0) * 0.5F);
}

TEST(BulletPhysicsTest, CapturesShooterPingWhenProjectileIsCreated)
{
    auto animation_data_manager = CreateAnimationDataManager();
    auto state_manager = CreateStateManagerWithSoldierSkeleton(animation_data_manager);
    auto soldiers = state_manager.CreateEmptySoldiersSnapshot();
    soldiers.at(0).active = true;
    state_manager.ApplySoldiersSnapshot(soldiers);
    state_manager.GetSoldier(0).SetPingTicks(7);
    state_manager.CreateProjectile({
      .style = Soldank::BulletType::Bullet,
      .weapon = Soldank::WeaponType::Ak74,
      .position = {},
      .velocity = { 10.0F, 0.0F },
      .timeout = 100,
      .hit_multiply = 1.0F,
      .team = Soldank::TeamType::None,
      .owner_id = 0,
      .push = 1.0F,
    });
    state_manager.GetSoldier(0).SetPingTicks(20);
    unsigned int count = 0;
    state_manager.ForEachBullet([&](const Soldank::Bullet& bullet) {
        ++count;
        EXPECT_EQ(bullet.owner_ping_ticks, 7);
    });
    EXPECT_EQ(count, 1U);
}

namespace
{
void ActivateTarget(Soldank::StateManager& state,
                    unsigned int id,
                    glm::vec2 position,
                    bool dead = false)
{
    auto& soldier = state.GetSoldier(static_cast<std::uint8_t>(id));
    soldier.active = true;
    soldier.id = static_cast<std::uint8_t>(id);
    soldier.skeleton = std::make_shared<Soldank::ParticleSystem>(
      std::vector<Soldank::Particle>(
        23, Soldank::Particle(true, position, position, {}, {}, 1.0F, 1.0F, 0.0F, 1.0F, 0.0F)),
      std::vector<Soldank::Constraint>{});
    soldier.dead_meat = dead;
    soldier.health = 10000.0F;
    soldier.particle.position = position;
    for (unsigned int part = 1; part <= 23; ++part) {
        soldier.skeleton->SetPos(part, position);
        soldier.skeleton->SetOldPos(part, position);
    }
}

std::size_t CountEvents(const Soldank::StateManager& state, Soldank::ProjectileEventKind kind)
{
    std::size_t count = 0;
    for (const auto& event : state.GetProjectileEvents()) {
        count += event.kind == kind ? 1U : 0U;
    }
    return count;
}
} // namespace

TEST(BulletPhysicsTest, SweepsPolygonsThinnerThanTheOldSamplingInterval)
{
    auto map = SoldankTesting::MapBuilder::Empty()
                 ->AddPolygon({ 1.234F, -10.0F },
                              { 1.244F, -10.0F },
                              { 1.239F, 10.0F },
                              Soldank::PMSPolygonType::Normal)
                 ->Build();
    auto bullet = CreateBullet({ 0.0F, 0.0F }, { 100.0F, 0.0F });
    bullet.particle.position = { 100.0F, 0.0F };
    const auto collision = Soldank::BulletCollision::FindMapCollision(bullet, *map);
    ASSERT_TRUE(collision);
    const auto& polygon = map->GetPolygons().at(0);
    const float entry_x = (polygon.vertices.at(0).x + polygon.vertices.at(2).x) * 0.5F;
    EXPECT_NEAR(collision->position.x, entry_x, 0.001F);
}

TEST(BulletPhysicsTest, DetonatesEveryExplosiveStyleOnTimeoutOnce)
{
    constexpr std::array STYLES{ Soldank::BulletType::FragGrenade, Soldank::BulletType::M79Grenade,
                                 Soldank::BulletType::FlameArrow,  Soldank::BulletType::LAWMissile,
                                 Soldank::BulletType::Cluster,     Soldank::BulletType::M2Bullet };
    for (const auto style : STYLES) {
        auto animations = CreateAnimationDataManager();
        auto state = CreateStateManager(animations);
        auto map = SoldankTesting::MapBuilder::Empty()->Build();
        Soldank::PhysicsEvents events;
        auto bullet = CreateBullet({}, {}, 1);
        bullet.style = style;
        Soldank::BulletPhysics::UpdateBullet(events, bullet, *map, state);
        EXPECT_FALSE(bullet.active);
        EXPECT_EQ(CountEvents(state, Soldank::ProjectileEventKind::ProjectileExploded), 1U);
        Soldank::BulletPhysics::UpdateBullet(events, bullet, *map, state);
        EXPECT_EQ(CountEvents(state, Soldank::ProjectileEventKind::ProjectileExploded), 1U);
    }
}

TEST(BulletPhysicsTest, ExplosiveWallImpactsDetonateAndGrenadesBounceBeforeExpiry)
{
    for (const auto style : { Soldank::BulletType::M79Grenade,
                              Soldank::BulletType::LAWMissile,
                              Soldank::BulletType::FlameArrow }) {
        auto animations = CreateAnimationDataManager();
        auto state = CreateStateManager(animations);
        auto map =
          SoldankTesting::MapBuilder::Empty()
            ->AddPolygon(
              { -5.0F, -10.0F }, { 5.0F, -10.0F }, { 0.0F, 10.0F }, Soldank::PMSPolygonType::Normal)
            ->Build();
        Soldank::PhysicsEvents events;
        auto bullet = CreateBullet({ -20.0F, 0.0F }, { 40.0F, 0.0F });
        bullet.style = style;
        Soldank::BulletPhysics::UpdateBullet(events, bullet, *map, state);
        EXPECT_FALSE(bullet.active);
        EXPECT_EQ(CountEvents(state, Soldank::ProjectileEventKind::ProjectileExploded), 1U);
    }
}

TEST(BulletPhysicsTest, ExplosionDamageUsesDistanceHitboxesAndDelayedRadialPush)
{
    auto animations = CreateAnimationDataManager();
    auto state = CreateStateManagerWithSoldierSkeleton(animations);
    ActivateTarget(state, 0, { 10.0F, 0.0F });
    ActivateTarget(state, 1, { 30.0F, 0.0F });
    ActivateTarget(state, 2, { 100.0F, 0.0F });
    Soldank::PhysicsEvents events;
    auto bullet = CreateBullet({}, {});
    bullet.style = Soldank::BulletType::FragGrenade;
    bullet.weapon = Soldank::WeaponType::FragGrenade;
    Soldank::BulletDamage::Explode(events, bullet, state);
    const auto& near = state.GetSoldier(0);
    const auto& far = state.GetSoldier(1);
    EXPECT_LT(near.health, far.health);
    EXPECT_LT(far.health, 10000.0F);
    EXPECT_FLOAT_EQ(state.GetSoldier(2).health, 10000.0F);
    ASSERT_EQ(near.projectile_pushes.size(), 1U);
    ASSERT_EQ(far.projectile_pushes.size(), 1U);
    EXPECT_GT(near.projectile_pushes.front().impulse.x, 0.0F);
    EXPECT_GT(near.projectile_pushes.front().impulse.x / 10.0F,
              far.projectile_pushes.front().impulse.x / 30.0F);
    EXPECT_EQ(near.projectile_pushes.front().tick, state.GetGameTick() + 1);
    EXPECT_EQ(CountEvents(state, Soldank::ProjectileEventKind::SoldierHitByExplosion), 2U);
}

TEST(BulletPhysicsTest, ExplosionMovesRagdollsAndItemsAndChainsEachRoundOnce)
{
    auto animations = CreateAnimationDataManager();
    auto state = CreateStateManagerWithSoldierSkeleton(animations);
    ActivateTarget(state, 0, { 10.0F, 0.0F }, true);
    auto& item = state.CreateItem({ 10.0F, 0.0F },
                                  255,
                                  Soldank::ItemType::Knife,
                                  CreateItemSkeleton({ 10.0F, 0.0F }, { 10.0F, 0.0F }));
    item.collide_with_bullets = true;
    for (const float position : { 20.0F, 40.0F }) {
        state.CreateProjectile({ .style = Soldank::BulletType::FragGrenade,
                                 .weapon = Soldank::WeaponType::FragGrenade,
                                 .position = { position, 0.0F },
                                 .velocity = {},
                                 .timeout = 100,
                                 .hit_multiply = 1.0F,
                                 .team = Soldank::TeamType::None,
                                 .owner_id = 255,
                                 .push = 1.0F });
    }
    Soldank::PhysicsEvents events;
    auto bullet = CreateBullet({}, {});
    bullet.style = Soldank::BulletType::FragGrenade;
    Soldank::BulletDamage::Explode(events, bullet, state);
    EXPECT_EQ(state.GetBulletsCount(), 0U);
    EXPECT_EQ(CountEvents(state, Soldank::ProjectileEventKind::ProjectileExploded), 3U);
    EXPECT_EQ(CountEvents(state, Soldank::ProjectileEventKind::ProjectileChainDetonated), 2U);
    EXPECT_NE(state.GetSoldier(0).skeleton->GetOldPos(1).x, 10.0F);
    EXPECT_NE(item.skeleton->GetOldPos(1).x, 10.0F);
    EXPECT_FALSE(item.static_type);
}

TEST(BulletPhysicsTest, ClusterChildrenAreSeededQueuedAndOwned)
{
    auto animations = CreateAnimationDataManager();
    auto first = CreateStateManager(animations);
    auto second = CreateStateManager(animations);
    auto bullet = CreateBullet({ 10.0F, 20.0F }, { 2.0F, 3.0F });
    bullet.seed = 123;
    bullet.style = Soldank::BulletType::ClusterGrenade;
    Soldank::BulletEffects::Fragment(first, bullet);
    Soldank::BulletEffects::Fragment(second, bullet);
    ASSERT_EQ(first.GetBulletEmitter().size(), 5U);
    ASSERT_EQ(second.GetBulletEmitter().size(), 5U);
    EXPECT_EQ(first.GetBulletsCount(), 0U);
    for (std::size_t index = 0; index < 5; ++index) {
        const auto& child = first.GetBulletEmitter().at(index);
        EXPECT_EQ(child.velocity, second.GetBulletEmitter().at(index).velocity);
        EXPECT_EQ(child.seed, second.GetBulletEmitter().at(index).seed);
        EXPECT_EQ(child.owner_id, bullet.owner_id);
        EXPECT_EQ(child.style, Soldank::BulletType::Cluster);
        EXPECT_TRUE(child.derived);
    }
    EXPECT_EQ(CountEvents(first, Soldank::ProjectileEventKind::ProjectileFragmented), 1U);
    second.ClearBulletEmitter();
    second.SetAuthoritativeProjectiles(false);
    Soldank::BulletEffects::Fragment(second, bullet);
    EXPECT_TRUE(second.GetBulletEmitter().empty());
}

TEST(BulletPhysicsTest, RespectsAllSixWeaponMasks)
{
    for (unsigned int explosion = 0; explosion < 2; ++explosion) {
        for (unsigned int category = 0; category < 3; ++category) {
            const auto mask = static_cast<std::uint8_t>(1U << (category + explosion * 3));
            for (unsigned int target = 0; target < 3; ++target) {
                EXPECT_EQ(
                  Soldank::AllowsWeaponCollision(mask, target == 2, target == 1, explosion != 0),
                  target != category);
                EXPECT_TRUE(
                  Soldank::AllowsWeaponCollision(mask, target == 2, target == 1, explosion == 0));
            }
        }
    }
}

TEST(BulletPhysicsTest, FriendlyFireAndProtectedTargetsBlockDirectAndExplosionDamage)
{
    auto animations = CreateAnimationDataManager();
    auto state = CreateStateManagerWithSoldierSkeleton(animations);
    ActivateTarget(state, 0, {});
    auto& target = state.GetSoldier(0);
    target.team = Soldank::TeamType::Alpha;
    auto bullet = CreateBullet({}, { 10.0F, 0.0F });
    bullet.team = Soldank::TeamType::Alpha;
    state.SetFriendlyFire(false);
    for (const bool explosion : { false, true }) {
        EXPECT_FALSE(Soldank::BulletDamage::CanDamage(state, target, bullet, explosion));
        state.SetFriendlyFire(true);
        EXPECT_TRUE(Soldank::BulletDamage::CanDamage(state, target, bullet, explosion));
        target.SetCeaseFireCounter(0);
        EXPECT_FALSE(Soldank::BulletDamage::CanDamage(state, target, bullet, explosion));
        target.SetCeaseFireCounter(-1);
        target.team = Soldank::TeamType::Spectator;
        EXPECT_FALSE(Soldank::BulletDamage::CanDamage(state, target, bullet, explosion));
        target.team = Soldank::TeamType::Alpha;
        state.SetFriendlyFire(false);
    }
}

TEST(BulletPhysicsTest, UsesBoundedHistoricalPositionsForShooterLatency)
{
    auto animations = CreateAnimationDataManager();
    auto state = CreateStateManagerWithSoldierSkeleton(animations);
    ActivateTarget(state, 0, {});
    auto& target = state.GetSoldier(0);
    target.RecordPosition(10);
    ActivateTarget(state, 0, { 50.0F, 0.0F });
    target.RecordPosition(20);
    state.SetGameTick(20);
    auto bullet = CreateBullet({ -20.0F, 0.0F }, { 40.0F, 0.0F });
    bullet.particle.position = { 20.0F, 0.0F };
    EXPECT_FALSE(Soldank::BulletCollision::FindSoldierCollision(bullet, state, -1.0F));
    bullet.owner_ping_ticks = 10;
    EXPECT_TRUE(Soldank::BulletCollision::FindSoldierCollision(bullet, state, -1.0F));
    for (unsigned int tick = 21; tick < 300; ++tick) {
        target.RecordPosition(tick);
    }
    EXPECT_EQ(target.position_history.size(), 125U);
    target.RecordPosition(290);
    EXPECT_EQ(target.position_history.back().tick, 290U);
}

TEST(BulletPhysicsTest, FlamePropagationIsBoundedAndRetainsReducedDamage)
{
    auto animations = CreateAnimationDataManager();
    auto state = CreateStateManagerWithSoldierSkeleton(animations);
    ActivateTarget(state, 0, {});
    auto& target = state.GetSoldier(0);
    target.particle.SetVelocity({ 2.0F, 0.0F });
    auto bullet = CreateBullet({}, {}, 2, 1000.0F);
    bullet.style = Soldank::BulletType::Flame;
    Soldank::BulletEffects::PropagateFlame(state, bullet, target, 12);
    ASSERT_EQ(state.GetBulletEmitter().size(), 1U);
    EXPECT_FLOAT_EQ(state.GetBulletEmitter().front().hit_multiply, 2000.0F / 3.0F);
    EXPECT_EQ(state.GetBulletEmitter().front().velocity, glm::vec2(-2.0F, 0.0F));
    EXPECT_EQ(bullet.timeout, 31);
    EXPECT_EQ(bullet.ricochet_count, 1);
    bullet.timeout = 2;
    Soldank::BulletEffects::PropagateFlame(state, bullet, target, 12);
    bullet.timeout = 2;
    Soldank::BulletEffects::PropagateFlame(state, bullet, target, 12);
    EXPECT_EQ(state.GetBulletEmitter().size(), 2U);
}

TEST(BulletPhysicsTest, FiresM79AndPrimaryProjectileStylesWithCombatMetadata)
{
    auto animations = CreateAnimationDataManager();
    auto state = CreateStateManagerWithSoldierSkeleton(animations);
    ActivateTarget(state, 0, {});
    auto& owner = state.GetSoldier(0);
    owner.team = Soldank::TeamType::Bravo;
    owner.SetPingTicks(8);
    owner.control.mouse_aim_x = 100;
    for (const auto weapon :
         { Soldank::WeaponType::M79, Soldank::WeaponType::Bow, Soldank::WeaponType::Flamer }) {
        owner.GetActiveWeapon() = Soldank::WeaponParametersFactory::GetParameters(weapon, false);
        auto ready = owner.GetActiveWeapon().GetState();
        ready.ammo = owner.GetActiveWeapon().GetWeaponParameters().ammo;
        ready.fire_interval = 0;
        ready.startup = 0;
        ASSERT_TRUE(owner.GetActiveWeapon().RestoreState(ready));
        owner.is_shooting = false;
        std::vector<Soldank::BulletParams> emitted;
        Soldank::SoldierPhysics::Fire(owner, emitted);
        ASSERT_EQ(emitted.size(), 1U);
        EXPECT_EQ(emitted.front().weapon, weapon);
        EXPECT_EQ(emitted.front().team, owner.team);
        EXPECT_EQ(emitted.front().owner_ping_ticks, 8);
    }
}

TEST(BulletPhysicsTest, SpawnReplicationPreservesDeterministicAndLatencyMetadata)
{
    Soldank::ProjectileSpawnedEvent event{ .bullet_params = { .style = Soldank::BulletType::Cluster,
                                                              .weapon =
                                                                Soldank::WeaponType::Cluster,
                                                              .position = { 1.0F, 2.0F },
                                                              .velocity = { 3.0F, 4.0F },
                                                              .timeout = 50,
                                                              .hit_multiply = 12.0F,
                                                              .team = Soldank::TeamType::Charlie,
                                                              .owner_id = 2,
                                                              .push = 0.5F,
                                                              .owner_ping_ticks = 9,
                                                              .id = 60000,
                                                              .seed = 12345,
                                                              .spawn_tick = 100,
                                                              .derived = true } };
    const auto packet = Soldank::ProtocolConversions::ToProjectileSpawnPacket(60000, event);
    const auto decoded = Soldank::ProtocolConversions::ToProjectileSpawnedEvent(packet);
    EXPECT_EQ(decoded.bullet_params.id, event.bullet_params.id);
    EXPECT_EQ(decoded.bullet_params.seed, event.bullet_params.seed);
    EXPECT_EQ(decoded.bullet_params.spawn_tick, 100U);
    EXPECT_EQ(decoded.bullet_params.owner_ping_ticks, 9);
    EXPECT_FLOAT_EQ(decoded.bullet_params.push, 0.5F);
    EXPECT_TRUE(decoded.bullet_params.derived);
}

TEST(BulletPhysicsTest, GrenadeBouncesAndThenExplodesOnItsLastTick)
{
    auto animations = CreateAnimationDataManager();
    auto state = CreateStateManager(animations);
    auto map =
      SoldankTesting::MapBuilder::Empty()
        ->AddPolygon(
          { -5.0F, -10.0F }, { 5.0F, -10.0F }, { 0.0F, 10.0F }, Soldank::PMSPolygonType::Normal)
        ->Build();
    Soldank::PhysicsEvents events;
    auto bullet = CreateBullet({ -20.0F, 0.0F }, { 40.0F, 0.0F }, 2);
    bullet.style = Soldank::BulletType::FragGrenade;
    Soldank::BulletPhysics::UpdateBullet(events, bullet, *map, state);
    ASSERT_TRUE(bullet.active);
    EXPECT_EQ(CountEvents(state, Soldank::ProjectileEventKind::ProjectileBounced), 1U);
    Soldank::BulletPhysics::UpdateBullet(events, bullet, *map, state);
    EXPECT_FALSE(bullet.active);
    EXPECT_EQ(CountEvents(state, Soldank::ProjectileEventKind::ProjectileExploded), 1U);
}

TEST(BulletPhysicsTest, PunchDropsEnemyWeaponButPreservesBows)
{
    auto animations = CreateAnimationDataManager();
    auto state = CreateStateManagerWithSoldierSkeleton(animations);
    ActivateTarget(state, 0, {});
    auto& target = state.GetSoldier(0);
    target.GetActiveWeapon() =
      Soldank::WeaponParametersFactory::GetParameters(Soldank::WeaponType::Ak74, false);
    auto bullet = CreateBullet({}, { 10.0F, 0.0F });
    bullet.style = Soldank::BulletType::Fist;
    const Soldank::BulletCollisionResult collision{ .kind = Soldank::BulletCollisionKind::Soldier,
                                                    .position = {},
                                                    .distance = 0.0F,
                                                    .soldier_id = 0,
                                                    .body_part_id = 12 };
    Soldank::PhysicsEvents events;
    Soldank::BulletDamage::ApplyDirectHit(events, bullet, state, collision);
    EXPECT_EQ(target.GetActiveWeapon().GetWeaponParameters().kind, Soldank::WeaponType::NoWeapon);
    EXPECT_EQ(CountEvents(state, Soldank::ProjectileEventKind::WorldItemSpawnedFromProjectile), 1U);
    target.GetActiveWeapon() =
      Soldank::WeaponParametersFactory::GetParameters(Soldank::WeaponType::Bow, false);
    Soldank::BulletDamage::ApplyDirectHit(events, bullet, state, collision);
    EXPECT_EQ(target.GetActiveWeapon().GetWeaponParameters().kind, Soldank::WeaponType::Bow);
}

TEST(BulletPhysicsTest, ArrowSticksToSoldierWithoutRepeatedDamage)
{
    auto animations = CreateAnimationDataManager();
    auto state = CreateStateManagerWithSoldierSkeleton(animations);
    ActivateTarget(state, 0, {});
    auto map = SoldankTesting::MapBuilder::Empty()->Build();
    auto bullet = CreateBullet({ -20.0F, 0.0F }, { 40.0F, 0.0F }, 400);
    bullet.style = Soldank::BulletType::Arrow;
    Soldank::PhysicsEvents events;
    Soldank::BulletPhysics::UpdateBullet(events, bullet, *map, state);
    ASSERT_TRUE(bullet.stuck);
    ASSERT_TRUE(bullet.attached_soldier);
    const float health = state.GetSoldier(0).health;
    Soldank::BulletPhysics::UpdateBullet(events, bullet, *map, state);
    EXPECT_FLOAT_EQ(state.GetSoldier(0).health, health);
    EXPECT_EQ(CountEvents(state, Soldank::ProjectileEventKind::ProjectileStuck), 1U);
}

TEST(BulletPhysicsTest, CommitsDamageBeforePublishingItsValueSnapshot)
{
    auto animations = CreateAnimationDataManager();
    auto state = CreateStateManagerWithSoldierSkeleton(animations);
    ActivateTarget(state, 0, {});
    Soldank::PhysicsEvents events;
    float observed_health = 0.0F;
    events.soldier_hit_by_bullet.AddObserver(
      [&](Soldank::Soldier& soldier, float) { observed_health = soldier.health; });
    auto bullet = CreateBullet({}, { 10.0F, 0.0F });
    Soldank::BulletDamage::ApplyDamage(
      events, state, state.GetSoldier(0), bullet, 12.0F, 12, false);
    EXPECT_FLOAT_EQ(observed_health, 9988.0F);
    const auto snapshot = state.GetProjectileEvents().back();
    state.GetSoldier(0).health = 100.0F;
    EXPECT_FLOAT_EQ(snapshot.damage, 12.0F);
    EXPECT_EQ(snapshot.particle_id, 12U);
}

TEST(BulletPhysicsTest, ClientDoesNotRepeatAuthoritativeDamageOrSpawnEffects)
{
    auto animations = CreateAnimationDataManager();
    auto state = CreateStateManagerWithSoldierSkeleton(animations);
    ActivateTarget(state, 0, { 10.0F, 0.0F });
    state.SetAuthoritativeProjectiles(false);
    auto bullet = CreateBullet({}, {});
    bullet.style = Soldank::BulletType::FragGrenade;
    Soldank::PhysicsEvents events;
    Soldank::BulletDamage::Explode(events, bullet, state);
    EXPECT_FLOAT_EQ(state.GetSoldier(0).health, 10000.0F);
    EXPECT_TRUE(state.GetSoldier(0).projectile_pushes.empty());
    Soldank::BulletEffects::Fragment(state, bullet);
    EXPECT_TRUE(state.GetBulletEmitter().empty());
}

TEST(BulletPhysicsTest, RebuildsOnlyFuturePushesDuringSnapshotReplay)
{
    Soldank::Soldier soldier;
    soldier.ScheduleProjectilePush(10, 0, { 1.0F, 0.0F });
    soldier.ScheduleProjectilePush(15, 0, { 2.0F, 0.0F });
    soldier.ApplyProjectilePushes(20);
    EXPECT_EQ(soldier.particle.GetVelocity(), glm::vec2(3.0F, 0.0F));
    soldier.RestoreProjectilePushesAfterTick(12);
    ASSERT_EQ(soldier.projectile_pushes.size(), 1U);
    soldier.particle.SetVelocity({ 1.0F, 0.0F });
    soldier.ApplyProjectilePushes(16);
    EXPECT_EQ(soldier.particle.GetVelocity(), glm::vec2(3.0F, 0.0F));
    soldier.ApplyProjectilePushes(17);
    EXPECT_EQ(soldier.particle.GetVelocity(), glm::vec2(3.0F, 0.0F));
    soldier.ApplyProjectilePushes(200);
    EXPECT_TRUE(soldier.projectile_push_history.empty());
}

TEST(BulletPhysicsTest, DirectDamageReportsEachHitboxAndItsImpactNormal)
{
    auto animations = CreateAnimationDataManager();
    auto state = CreateStateManagerWithSoldierSkeleton(animations);
    ActivateTarget(state, 0, {});
    Soldank::PhysicsEvents events;
    auto bullet = CreateBullet({}, { 10.0F, 0.0F });
    for (const int part : { 3, 6, 12 }) {
        const Soldank::BulletCollisionResult collision{ .kind =
                                                          Soldank::BulletCollisionKind::Soldier,
                                                        .position = { -7.0F, 0.0F },
                                                        .distance = 1.0F,
                                                        .soldier_id = 0,
                                                        .body_part_id = part };
        Soldank::BulletDamage::ApplyDirectHit(events, bullet, state, collision);
        const auto& event = state.GetProjectileEvents().back();
        ASSERT_EQ(event.kind, Soldank::ProjectileEventKind::SoldierHitByProjectile);
        EXPECT_FLOAT_EQ(event.damage,
                        10.0F * Soldank::BulletDamage::HitboxModifier(bullet.weapon, part));
        ASSERT_TRUE(event.collision);
        EXPECT_EQ(event.position, collision.position);
        ASSERT_TRUE(event.collision->surface_normal);
        EXPECT_NEAR(event.collision->surface_normal->x, -9.1F, 0.00001F);
        EXPECT_FLOAT_EQ(event.collision->surface_normal->y, 0.0F);
    }
}

TEST(BulletPhysicsTest, ThrownKnifeIsRecoverableAfterHittingALivingSoldier)
{
    auto animations = CreateAnimationDataManager();
    auto state = CreateStateManagerWithSoldierSkeleton(animations);
    ActivateTarget(state, 0, {});
    Soldank::PhysicsEvents events;
    auto bullet = CreateBullet({}, { 10.0F, 0.0F });
    bullet.style = Soldank::BulletType::ThrownKnife;
    const Soldank::BulletCollisionResult collision{ .kind = Soldank::BulletCollisionKind::Soldier,
                                                    .position = {},
                                                    .distance = 0.0F,
                                                    .soldier_id = 0,
                                                    .body_part_id = 12 };
    EXPECT_EQ(Soldank::BulletDamage::ApplyDirectHit(events, bullet, state, collision),
              Soldank::BulletDamage::DirectHitOutcome::Destroyed);
    EXPECT_EQ(CountEvents(state, Soldank::ProjectileEventKind::WorldItemSpawnedFromProjectile), 1U);
}

TEST(BulletPhysicsTest, OnlineProjectilesWaitForAuthoritativeImpactAndRemoval)
{
    auto animations = CreateAnimationDataManager();
    auto state = CreateStateManager(animations);
    state.SetAuthoritativeProjectiles(false);
    auto map =
      SoldankTesting::MapBuilder::Empty()
        ->AddPolygon(
          { -5.0F, -10.0F }, { 5.0F, -10.0F }, { 0.0F, 10.0F }, Soldank::PMSPolygonType::Normal)
        ->Build();
    auto bullet = CreateBullet({ -20.0F, 0.0F }, { 40.0F, 0.0F }, 1);
    bullet.style = Soldank::BulletType::M79Grenade;
    Soldank::PhysicsEvents events;
    Soldank::BulletPhysics::UpdateBullet(events, bullet, *map, state);
    EXPECT_TRUE(bullet.active);
    EXPECT_TRUE(state.GetProjectileEvents().empty());
}

TEST(BulletPhysicsTest, ColliderImpactsRecoverKnivesAndRespectGrenadeArmingGrace)
{
    auto animations = CreateAnimationDataManager();
    auto state = CreateStateManager(animations);
    auto map = CreateMapWithColliders({ { .active = 1, .x = 0.0F, .y = 0.0F, .radius = 17.0F } });
    Soldank::PhysicsEvents events;
    auto knife = CreateBullet({ -20.0F, 0.0F }, { 40.0F, 0.0F });
    knife.style = Soldank::BulletType::ThrownKnife;
    Soldank::BulletPhysics::UpdateBullet(events, knife, *map, state);
    EXPECT_FALSE(knife.active);
    EXPECT_EQ(CountEvents(state, Soldank::ProjectileEventKind::WorldItemSpawnedFromProjectile), 1U);
    auto grenade = CreateBullet({ -20.0F, 0.0F }, { 40.0F, 0.0F });
    grenade.style = Soldank::BulletType::FragGrenade;
    grenade.particle.position = { 20.0F, 0.0F };
    EXPECT_FALSE(Soldank::BulletCollision::FindColliderCollision(grenade, *map));
    grenade.timeout -= 3;
    EXPECT_TRUE(Soldank::BulletCollision::FindColliderCollision(grenade, *map));
}

TEST(BulletPhysicsTest, ThrownKnifeEventPreservesOwnerCombatMetadata)
{
    auto world = std::make_shared<Soldank::World>();
    Soldank::CoreEventHandler::ObserveAll(world.get());
    const auto owner_id = world->CreateSoldier().id;
    auto& owner = world->GetStateManager()->GetSoldier(owner_id);
    owner.GetActiveWeapon() =
      Soldank::WeaponParametersFactory::GetParameters(Soldank::WeaponType::Knife, false);
    owner.SetTeam(Soldank::TeamType::Alpha);
    owner.SetPingTicks(9);
    owner.control.mouse_aim_x = 100;
    owner.dead_meat = false;
    Soldank::WeaponPhysics::ThrowWeapon(*world->GetStateManager(), owner);
    const auto& emitted = world->GetStateManager()->GetBulletEmitter();
    ASSERT_EQ(emitted.size(), 1U);
    EXPECT_EQ(emitted.front().style, Soldank::BulletType::ThrownKnife);
    EXPECT_EQ(emitted.front().team, Soldank::TeamType::Alpha);
    EXPECT_EQ(emitted.front().owner_ping_ticks, 9);
    EXPECT_FLOAT_EQ(
      emitted.front().push,
      Soldank::WeaponParametersFactory::GetParameters(Soldank::WeaponType::ThrownKnife, false)
        .push);
}

TEST(WeaponFiringTest, SpasEmitsSixSeededPelletsAndSpendsOneShell)
{
    auto animations = CreateAnimationDataManager();
    auto state = CreateStateManagerWithSoldierSkeleton(animations);
    ActivateTarget(state, 0, {});
    auto& owner = state.GetSoldier(0);
    owner.control.mouse_aim_x = 100;
    owner.GetActiveWeapon() =
      Soldank::WeaponParametersFactory::GetParameters(Soldank::WeaponType::Spas12, false);
    auto ready = owner.GetActiveWeapon().GetState();
    ready.fire_interval = 0;
    ready.startup = 0;
    ASSERT_TRUE(owner.GetActiveWeapon().RestoreState(ready));
    const auto velocity = owner.particle.GetVelocity();
    std::vector<Soldank::BulletParams> first;
    Soldank::SoldierPhysics::Fire(owner, first);
    ASSERT_EQ(first.size(), 6U);
    EXPECT_EQ(owner.GetActiveWeapon().GetAmmoCount(), ready.ammo - 1);
    EXPECT_NE(first[0].velocity, first[1].velocity);
    EXPECT_NE(owner.particle.GetVelocity(), velocity);
    std::vector<Soldank::BulletParams> blocked;
    Soldank::SoldierPhysics::Fire(owner, blocked);
    EXPECT_TRUE(blocked.empty());
    owner.GetActiveWeapon().RestoreState(ready);
    owner.particle.SetVelocity(velocity);
    std::vector<Soldank::BulletParams> replay;
    Soldank::SoldierPhysics::Fire(owner, replay);
    ASSERT_EQ(replay.size(), first.size());
    for (std::size_t i = 0; i < first.size(); ++i) {
        EXPECT_EQ(first[i].velocity, replay[i].velocity);
        EXPECT_EQ(first[i].seed, replay[i].seed);
    }
}

TEST(WeaponFiringTest, EaglesPairConsumesOneRoundAndHasIndependentSpread)
{
    auto animations = CreateAnimationDataManager();
    auto state = CreateStateManagerWithSoldierSkeleton(animations);
    ActivateTarget(state, 0, {});
    auto& owner = state.GetSoldier(0);
    owner.control.mouse_aim_x = -100;
    owner.control.mouse_aim_y = 100;
    auto ready = owner.GetActiveWeapon().GetState();
    ready.fire_interval = 0;
    ready.startup = 0;
    ASSERT_TRUE(owner.GetActiveWeapon().RestoreState(ready));
    std::vector<Soldank::BulletParams> bullets;
    Soldank::SoldierPhysics::Fire(owner, bullets);
    ASSERT_EQ(bullets.size(), 2U);
    EXPECT_EQ(owner.GetActiveWeapon().GetAmmoCount(), ready.ammo - 1);
    EXPECT_NE(bullets[0].velocity, bullets[1].velocity);
    EXPECT_GT(bullets[1].position.x, bullets[0].position.x);
    EXPECT_GT(bullets[1].position.y, bullets[0].position.y);
}

TEST(WeaponFiringTest, RejectedLawDoesNotSpendAmmoOrAdvanceShotState)
{
    auto animations = CreateAnimationDataManager();
    auto state = CreateStateManagerWithSoldierSkeleton(animations);
    ActivateTarget(state, 0, {});
    auto& owner = state.GetSoldier(0);
    owner.GetActiveWeapon() =
      Soldank::WeaponParametersFactory::GetParameters(Soldank::WeaponType::LAW, false);
    const auto before = owner.GetActiveWeapon().GetState();
    std::vector<Soldank::BulletParams> bullets;
    Soldank::SoldierPhysics::Fire(owner, bullets);
    EXPECT_TRUE(bullets.empty());
    EXPECT_EQ(before, owner.GetActiveWeapon().GetState());
    EXPECT_FALSE(owner.is_shooting);
}

TEST(WeaponFiringTest, CeasefireAndSpectatorsCannotSpendAmmo)
{
    auto animations = CreateAnimationDataManager();
    auto state = CreateStateManagerWithSoldierSkeleton(animations);
    ActivateTarget(state, 0, {});
    auto& owner = state.GetSoldier(0);
    auto ready = owner.GetActiveWeapon().GetState();
    ready.fire_interval = 0;
    ready.startup = 0;
    owner.GetActiveWeapon().RestoreState(ready);
    std::vector<Soldank::BulletParams> bullets;
    owner.SetCeaseFireCounter(1);
    Soldank::SoldierPhysics::Fire(owner, bullets);
    EXPECT_TRUE(bullets.empty());
    EXPECT_EQ(ready, owner.GetActiveWeapon().GetState());
    owner.SetCeaseFireCounter(-1);
    owner.SetTeam(Soldank::TeamType::Spectator);
    Soldank::SoldierPhysics::Fire(owner, bullets);
    EXPECT_TRUE(bullets.empty());
    EXPECT_EQ(ready, owner.GetActiveWeapon().GetState());
}

TEST(WeaponFiringTest, GrenadeRequiresValidReleaseAndOnlyConsumesOncePerThrow)
{
    auto animations = CreateAnimationDataManager();
    AnimationDataReader reader;
    animations.LoadAnimationData(Soldank::AnimationType::Throw, "stand.poa", false, 1, reader);
    auto state = CreateStateManagerWithSoldierSkeleton(animations);
    ActivateTarget(state, 0, {});
    auto& owner = state.GetSoldier(0);
    owner.control.mouse_aim_x = 100;
    owner.control.throw_grenade = false;
    owner.body_animation =
      Soldank::SoldierPhysics::CreateBodyAnimationState(Soldank::AnimationType::Throw, animations);
    auto map = SoldankTesting::MapBuilder::Empty()->Build();
    const auto ammo = owner.weapons[2].GetAmmoCount();
    std::vector<Soldank::BulletParams> grenades;
    owner.body_animation->SetFrame(14);
    EXPECT_FALSE(Soldank::WeaponPhysics::ThrowGrenade(owner, *map, grenades));
    owner.body_animation->SetFrame(15);
    EXPECT_TRUE(Soldank::WeaponPhysics::ThrowGrenade(owner, *map, grenades));
    EXPECT_FALSE(Soldank::WeaponPhysics::ThrowGrenade(owner, *map, grenades));
    ASSERT_EQ(grenades.size(), 1U);
    EXPECT_EQ(owner.weapons[2].GetAmmoCount(), ammo - 1);
    EXPECT_EQ(grenades[0].style, Soldank::BulletType::FragGrenade);
    EXPECT_LT(grenades[0].velocity.y, 0.0F);
    owner.weapons[2].ResetThrow();
    owner.control.throw_grenade = true;
    owner.body_animation->SetFrame(35);
    EXPECT_FALSE(Soldank::WeaponPhysics::ThrowGrenade(owner, *map, grenades));
    owner.body_animation->SetFrame(36);
    EXPECT_TRUE(Soldank::WeaponPhysics::ThrowGrenade(owner, *map, grenades));
    EXPECT_FALSE(Soldank::WeaponPhysics::ThrowGrenade(owner, *map, grenades));
    EXPECT_EQ(grenades.size(), 2U);
}

TEST(WeaponFiringTest, M2MountsExclusivelyAndStopsAtOverheat)
{
    auto animations = CreateAnimationDataManager();
    auto state = CreateStateManagerWithSoldierSkeleton(animations);
    ActivateTarget(state, 0, {});
    ActivateTarget(state, 1, {});
    state.GetSoldier(0).control.mouse_aim_x = 100;
    state.GetSoldier(0).control.fire = true;
    state.GetSoldier(1).control.mouse_aim_x = 100;
    std::vector<Soldank::Particle> particles(4);
    auto skeleton =
      std::make_shared<Soldank::ParticleSystem>(particles, std::vector<Soldank::Constraint>{});
    auto& item = state.CreateItem({}, 255, Soldank::ItemType::M2, skeleton);
    item.time_out = 0;
    const auto interval =
      Soldank::WeaponParametersFactory::GetParameters(Soldank::WeaponType::M2, false).fire_interval;
    for (unsigned int shot = 0; shot < 25; ++shot) {
        state.SetGameTick(shot * interval);
        Soldank::StationaryWeaponPhysics::Update(state, item);
    }
    EXPECT_EQ(state.GetSoldier(0).mounted_weapon, item.id);
    EXPECT_EQ(state.GetSoldier(1).mounted_weapon, -1);
    EXPECT_EQ(state.GetBulletEmitter().size(), 19U);
    EXPECT_EQ(item.interest, 19);
    state.GetSoldier(0).control.up = true;
    Soldank::StationaryWeaponPhysics::Update(state, item);
    EXPECT_EQ(state.GetSoldier(0).mounted_weapon, -1);
    EXPECT_EQ(state.GetSoldier(1).mounted_weapon, item.id);
}

TEST(BulletPhysicsTest, VestAndCombatBonusesFollowReferenceDamageOrder)
{
    auto animations = CreateAnimationDataManager();
    auto state = CreateStateManagerWithSoldierSkeleton(animations);
    ActivateTarget(state, 0, {});
    auto& target = state.GetSoldier(0);
    target.health = 150.0F;
    target.vest = 100.0F;
    auto bullet = CreateBullet({}, {});
    Soldank::PhysicsEvents events;
    Soldank::BulletDamage::ApplyDamage(events, state, target, bullet, 40.0F, 12, false);
    EXPECT_FLOAT_EQ(target.health, 140.0F);
    EXPECT_FLOAT_EQ(target.vest, 87.0F);
    ActivateTarget(state, 1, { 100.0F, 0.0F });
    state.GetSoldier(1).combat_bonus = Soldank::CombatBonus::Berserk;
    bullet.owner_id = 1;
    Soldank::BulletDamage::ApplyDamage(events, state, target, bullet, 40.0F, 12, false);
    EXPECT_FLOAT_EQ(target.health, -20.0F);
    EXPECT_FLOAT_EQ(target.vest, 74.0F);
    target.health = 150.0F;
    target.dead_meat = false;
    target.combat_bonus = Soldank::CombatBonus::FlameGod;
    Soldank::BulletDamage::ApplyDamage(events, state, target, bullet, 40.0F, 12, false);
    EXPECT_FLOAT_EQ(target.health, 150.0F);
    EXPECT_FLOAT_EQ(target.vest, 74.0F);
}
