#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <vector>

import Extern.Glm;
import WeaponVisualEffects;
import SoldierVisuals;
import Shared.Core.World;
import Shared.Core.Entities.Soldier;
import Shared.Core.Entities.Bullet;
import Shared.Core.State.StateManager;
import Shared.Core.Types.BulletType;
import Shared.Core.Types.WeaponType;
import Shared.Core.Map.PMSEnums;
import Shared.Core.Physics.Bullets.BulletEvents;
import Shared.Core.Simulation.PlayerInputApplication;
import Shared.Networking.ProtocolConversions;
import Shared.Networking.NetworkPackets;
import Testing.Framework.Shared.MapBuilder;

using namespace Soldank;

namespace
{
std::size_t Count(const WeaponVisualEffects& effects, WeaponVisualEffectKind kind)
{
    return std::ranges::count_if(effects.GetEffects(),
                                 [kind](const auto& effect) { return effect.kind == kind; });
}
} // namespace

TEST(VisualEffectsTest, FragTrailUsesReferenceOffsetTintAndStartupGate)
{
    Bullet bullet;
    bullet.active = true;
    bullet.style = BulletType::FragGrenade;
    bullet.spawn_tick = 10;
    bullet.particle.position = { 20.0F, 30.0F };
    bullet.particle.old_position = { 17.0F, 26.0F };
    bullet.particle.velocity_ = { 3.0F, 4.0F };
    EXPECT_FALSE(GetProjectileTrail(bullet, 12, 1.0F));
    const auto trail = GetProjectileTrail(bullet, 14, 1.0F);
    ASSERT_TRUE(trail);
    EXPECT_EQ(trail->position, glm::vec2(19.0F, 28.0F));
    EXPECT_NEAR(trail->scale.x, 5.0F / 3.0F, 0.00001F);
    EXPECT_NEAR(trail->color.r, 100.0F / 255.0F, 0.00001F);
    EXPECT_EQ(trail->color.g, 1.0F);
    EXPECT_NEAR(trail->color.a, 82.5F / 255.0F, 0.00001F);
    EXPECT_NEAR(trail->rotation, -std::atan2(-4.0F, -3.0F), 0.00001F);
}

TEST(VisualEffectsTest, ReferenceDoesNotDrawTrailsForClustersKnivesOrFlamingArrows)
{
    Bullet bullet;
    bullet.active = true;
    bullet.particle.velocity_ = { 5.0F, 1.0F };
    for (const auto style : { BulletType::ClusterGrenade,
                              BulletType::Cluster,
                              BulletType::ThrownKnife,
                              BulletType::FlameArrow }) {
        bullet.style = style;
        EXPECT_FALSE(GetProjectileTrail(bullet, 100, 1.0F));
    }
    bullet.style = BulletType::Bullet;
    ASSERT_TRUE(GetProjectileTrail(bullet, 100, 1.0F));
    bullet.stuck = true;
    EXPECT_FALSE(GetProjectileTrail(bullet, 100, 1.0F));
}

TEST(VisualEffectsTest, PenetratedBulletsHaveTheReferenceRedTint)
{
    Bullet bullet;
    bullet.active = true;
    bullet.style = BulletType::Bullet;
    bullet.particle.velocity_ = { 14.0F, 0.0F };
    const auto normal = GetProjectileTrail(bullet, 10, 1.0F);
    bullet.penetrated_soldiers.push_back(1);
    const auto penetrated = GetProjectileTrail(bullet, 10, 1.0F);
    ASSERT_TRUE(normal);
    ASSERT_TRUE(penetrated);
    EXPECT_FLOAT_EQ(normal->scale.x, 4.0F);
    EXPECT_FLOAT_EQ(penetrated->scale.x, 3.5F);
    EXPECT_LT(penetrated->color.g, normal->color.g);
}

TEST(VisualEffectsTest, JetTintAndLifetimeUseReferenceCurves)
{
    WeaponVisualEffect effect{ .kind = WeaponVisualEffectKind::JetFire,
                               .position = { 10.0F, 20.0F },
                               .old_position = { 8.0F, 18.0F },
                               .lifetime = 20,
                               .initial_lifetime = 40,
                               .tint = { 0.2F, 0.5F, 0.9F, 1.0F } };
    const auto sprite = GetEffectAppearance(effect, 1.0F);
    EXPECT_EQ(sprite.position, effect.position);
    EXPECT_FLOAT_EQ(sprite.color.r, 0.2F);
    EXPECT_FLOAT_EQ(sprite.color.b, 0.9F);
    EXPECT_NEAR(sprite.color.a, 100.0F / 255.0F, 0.00001F);
    EXPECT_EQ(GetEffectAppearance(effect, 0.0F).position, effect.old_position);
}

TEST(VisualEffectsTest, ExpiredFireAndSmokeNeverProduceInfiniteTransforms)
{
    for (const auto kind : { WeaponVisualEffectKind::Flame,
                             WeaponVisualEffectKind::BlackSmoke,
                             WeaponVisualEffectKind::Exhaust }) {
        WeaponVisualEffect effect{ .kind = kind, .lifetime = 0, .initial_lifetime = 0 };
        const auto sprite = GetEffectAppearance(effect, 1.0F);
        EXPECT_TRUE(std::isfinite(sprite.position.y));
        EXPECT_TRUE(std::isfinite(sprite.scale.x));
        EXPECT_GE(sprite.color.a, 0.0F);
        EXPECT_LE(sprite.color.a, 1.0F);
    }
}

TEST(VisualEffectsTest, BloodEventsAreDeduplicatedAndDoNotConsumeSimulationRandomness)
{
    World world;
    auto state = world.GetStateManager();
    state->SetRandomSeed(42);
    const int expected = state->GetRandomInt(0, 100000);
    state->SetRandomSeed(42);
    WeaponVisualEffects effects;
    const ProjectilePhysicsEvent hit{ .kind = ProjectileEventKind::SoldierHitByProjectile,
                                      .projectile_id = 7,
                                      .position = { 10.0F, 20.0F },
                                      .velocity_before = { 10.0F, 0.0F },
                                      .damage = 20.0F,
                                      .soldier_id = 1 };
    effects.Update(*state, std::vector{ hit, hit });
    EXPECT_EQ(Count(effects, WeaponVisualEffectKind::Blood), 2U);
    EXPECT_GE(Count(effects, WeaponVisualEffectKind::SmallBlood), 2U);
    const auto count = effects.GetEffects().size();
    effects.Update(*state, std::vector{ hit });
    EXPECT_EQ(effects.GetEffects().size(), count);
    EXPECT_EQ(state->GetRandomInt(0, 100000), expected);
}

TEST(VisualEffectsTest, WeatherEmissionIsTickBoundedAndCanBeDisabled)
{
    World world;
    auto state = world.GetStateManager();
    state->GetMap().SetWeatherType(PMSWeatherType::Rain);
    WeaponVisualEffects effects;
    state->SetGameTick(17);
    effects.Update(*state, {}, {}, { 800.0F, 600.0F });
    ASSERT_EQ(Count(effects, WeaponVisualEffectKind::Rain), 8U);
    for (unsigned int frame = 0; frame < 100; ++frame) {
        effects.Update(*state, {}, {}, { 800.0F, 600.0F });
    }
    EXPECT_EQ(Count(effects, WeaponVisualEffectKind::Rain), 8U);
    state->SetGameTick(34);
    effects.Update(*state, {}, {}, { 800.0F, 600.0F }, false);
    EXPECT_EQ(Count(effects, WeaponVisualEffectKind::Rain), 8U);
    effects.Reset();
    EXPECT_TRUE(effects.GetEffects().empty());
    effects.Update(*state, {}, {}, { 800.0F, 600.0F }, false);
    EXPECT_TRUE(effects.GetEffects().empty());
}

TEST(VisualEffectsTest, JetsRequireLiveSoldierAndFuelAndCaptureTint)
{
    World world;
    world.CreateSoldier(1);
    world.SpawnSoldier(1, glm::vec2{});
    auto state = world.GetStateManager();
    auto empty_map = SoldankTesting::MapBuilder::Empty()->Build();
    state->OverrideMap(*empty_map);
    auto& soldier = state->GetSoldier(1);
    soldier.control.jets = true;
    soldier.jets_count = 100;
    soldier.appearance.jet = { 0.2F, 0.5F, 0.9F, 1.0F };
    WeaponVisualEffects effects;
    for (unsigned int tick = 0; tick < 120; ++tick) {
        state->SetGameTick(tick);
        effects.Update(*state, {});
    }
    ASSERT_GT(Count(effects, WeaponVisualEffectKind::JetFire), 0U);
    for (const auto& effect : effects.GetEffects()) {
        if (effect.kind == WeaponVisualEffectKind::JetFire) {
            EXPECT_EQ(effect.tint, soldier.appearance.jet);
            EXPECT_EQ(effect.owner, 1);
            EXPECT_TRUE(effect.has_owner);
        }
    }
    soldier.jets_count = 0;
    effects.Reset();
    for (unsigned int tick = 120; tick < 240; ++tick) {
        state->SetGameTick(tick);
        effects.Update(*state, {});
    }
    EXPECT_EQ(Count(effects, WeaponVisualEffectKind::JetFire), 0U);
}

TEST(VisualEffectsTest, SpawnEffectsDeduplicateBySoldierLifeAndCaptureAppearance)
{
    World world;
    world.CreateSoldier(1);
    world.SpawnSoldier(1, glm::vec2{});
    auto& soldier = world.GetStateManager()->GetSoldier(1);
    WeaponVisualEffects effects;
    effects.ObserveSpawn(soldier);
    effects.ObserveSpawn(soldier);
    ASSERT_EQ(Count(effects, WeaponVisualEffectKind::Spawn), 1U);
    EXPECT_EQ(effects.GetEffects().front().tint, soldier.appearance.shirt);
    world.SpawnSoldier(1, glm::vec2{});
    effects.ObserveSpawn(soldier);
    EXPECT_EQ(Count(effects, WeaponVisualEffectKind::Spawn), 2U);
}

TEST(VisualEffectsTest, AppearanceInputRoundTripsAndRejectsInvalidColors)
{
    World world;
    world.CreateSoldier(1);
    SoldierInputPacket packet{};
    packet.has_appearance = true;
    packet.appearance.hair_style = HairStyle::Dreadlocks;
    packet.appearance.jet = { 0.1F, 0.2F, 0.9F, 1.0F };
    auto command = ProtocolConversions::ToPlayerInputCommand(1, packet);
    ASSERT_TRUE(command.appearance);
    ApplyPlayerInputCommand(*world.GetStateManager(), command);
    EXPECT_EQ(world.GetSoldier(1).appearance.hair_style, HairStyle::Dreadlocks);
    EXPECT_EQ(world.GetSoldier(1).appearance.jet, packet.appearance.jet);
    packet.appearance.skin.x = std::numeric_limits<float>::quiet_NaN();
    EXPECT_FALSE(packet.appearance.IsValid());
    EXPECT_FALSE(ProtocolConversions::ToPlayerInputCommand(1, packet).appearance);
    packet.appearance = {};
    packet.appearance.hair_style = static_cast<HairStyle>(255);
    EXPECT_FALSE(packet.appearance.IsValid());
}

TEST(VisualEffectsTest, DreadlockRootsAndStrandScaleFollowReferenceTransform)
{
    for (unsigned int strand = 0; strand < 5; ++strand) {
        const auto right =
          GetDreadlockTransform({ 1.0F, 0.0F }, { 0.0F, 0.5F }, { 45.0F, 90.0F }, 0.0F, 1, strand);
        const auto left =
          GetDreadlockTransform({ 1.0F, 0.0F }, { 0.0F, 0.5F }, { 45.0F, 90.0F }, 0.0F, -1, strand);
        EXPECT_NEAR(right.longitudinal_scale, 0.75F + strand * 0.05F, 0.00001F);
        EXPECT_NEAR(right.anchor_offset.y, 10.0F, 0.00001F);
        EXPECT_NEAR(left.anchor_offset.y, -10.0F, 0.00001F);
        EXPECT_NEAR(right.anchor_offset.x, 0.0F, 0.00001F);
    }
}

TEST(VisualEffectsTest, CorrectedImpactPositionDoesNotReplayAnAlreadySeenEvent)
{
    World world;
    WeaponVisualEffects effects;
    ProjectilePhysicsEvent hit{ .kind = ProjectileEventKind::ProjectileImpact,
                                .projectile_id = 42,
                                .tick = 10,
                                .position = { 10.0F, 20.0F } };
    effects.Update(*world.GetStateManager(), std::vector{ hit });
    const auto count = effects.GetEffects().size();
    hit.position.x += 0.1F;
    effects.Update(*world.GetStateManager(), std::vector{ hit });
    EXPECT_EQ(effects.GetEffects().size(), count);
}

TEST(VisualEffectsTest, PausingFreezesLifetimesAndDefersQueuedEvents)
{
    World world;
    auto state = world.GetStateManager();
    WeaponVisualEffects effects;
    ProjectilePhysicsEvent impact{ .kind = ProjectileEventKind::ProjectileImpact,
                                   .projectile_id = 1 };
    effects.Update(*state, std::vector{ impact });
    const auto count = effects.GetEffects().size();
    const auto lifetime = effects.GetEffects().front().lifetime;
    state->PauseGame();
    impact.projectile_id = 2;
    effects.Update(*state, std::vector{ impact });
    EXPECT_EQ(effects.GetEffects().size(), count);
    EXPECT_EQ(effects.GetEffects().front().lifetime, lifetime);
    state->UnPauseGame();
    effects.Update(*state, std::vector{ impact });
    EXPECT_GT(effects.GetEffects().size(), count);
}

TEST(VisualEffectsTest, ImportantExplosionsSurviveParticleBudgetPressure)
{
    World world;
    WeaponVisualEffects effects;
    std::vector<ProjectilePhysicsEvent> events;
    for (unsigned int index = 0; index < 3000; ++index) {
        events.push_back({ .kind = ProjectileEventKind::ProjectileImpact, .projectile_id = index });
    }
    events.push_back({ .kind = ProjectileEventKind::ProjectileExploded,
                       .projectile_id = 4000,
                       .style = BulletType::FragGrenade });
    effects.Update(*world.GetStateManager(), events);
    EXPECT_LE(effects.GetEffects().size(), 2048U);
    EXPECT_EQ(Count(effects, WeaponVisualEffectKind::Explosion), 1U);
    effects.Reset();
    EXPECT_TRUE(effects.GetEffects().empty());
}

TEST(VisualEffectsTest, RespawnRestoresSelectedHeadgearAndSlotReuseClearsAppearance)
{
    World world;
    world.CreateSoldier(1);
    auto& soldier = world.GetStateManager()->GetSoldier(1);
    soldier.appearance.headgear = 2;
    soldier.appearance.cigar = 1;
    soldier.appearance.hair_style = HairStyle::Dreadlocks;
    world.SpawnSoldier(1, glm::vec2{});
    EXPECT_EQ(soldier.wear_helmet, 1);
    EXPECT_EQ(soldier.has_cigar, 5);
    soldier.wear_helmet = 0;
    world.SpawnSoldier(1, glm::vec2{});
    EXPECT_EQ(soldier.wear_helmet, 1);
    EXPECT_EQ(soldier.appearance.hair_style, HairStyle::Dreadlocks);
    world.GetStateManager()->RemoveSoldier(1);
    world.CreateSoldier(1);
    EXPECT_EQ(soldier.appearance.hair_style, HairStyle::Normal);
    EXPECT_EQ(soldier.appearance.headgear, 0);
}
