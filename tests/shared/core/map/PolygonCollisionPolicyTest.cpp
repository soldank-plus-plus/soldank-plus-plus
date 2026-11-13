#include <gtest/gtest.h>
#include <array>
#include <cstdint>

import Shared.Core.Map.PMSEnums;
import Shared.Core.Map.PMSConstants;
import Shared.Core.Map.Polygons.PolygonTraits;
import Shared.Core.Map.Polygons.PolygonBackgroundState;
import Shared.Core.Map.Polygons.PolygonCollisionPolicy;

using namespace Soldank;

// Independent bit vectors transcribed from PolyMap.RayCast, not generated from traits.
// Mask bits: player=1, flag=2, bullet=4. Bit N means the query mask N blocks.
constexpr std::array<unsigned int, 26> RAY_MASKS{ 255, 240, 170, 0,   255, 255, 255, 255, 255,
                                                  255, 240, 170, 240, 170, 240, 170, 240, 170,
                                                  255, 255, 255, 136, 34,  128, 0,   0 };
constexpr std::array<bool, 26> SOLDIER_SOLID{ true,  false, true,  false, true,  true,  true,
                                              true,  true,  true,  false, false, false, false,
                                              false, false, false, false, true,  true,  true,
                                              false, true,  false, true,  false };
constexpr std::array<bool, 26> BULLET_SOLID{ true,  true,  false, false, true,  true,  true,
                                             true,  true,  true,  false, false, false, false,
                                             false, false, false, false, true,  true,  true,
                                             false, false, false, false, false };

TEST(PolygonCollisionPolicyTest, EveryTypeTeamAndRayMaskMatchesReference)
{
    for (unsigned int id = 0; id < 26; ++id) {
        for (std::uint8_t team = 0; team <= 5; ++team) {
            for (unsigned int mask = 0; mask < 8; ++mask) {
                SCOPED_TRACE(::testing::Message() << id << ':' << unsigned(team) << ':' << mask);
                const bool team_matches = id < 10 || id > 17 || team == (id - 10) / 2 + 1;
                const bool expected = team_matches && ((RAY_MASKS.at(id) >> mask) & 1U) != 0;
                EXPECT_EQ(AllowsPolygon(static_cast<PMSPolygonType>(id),
                                        { .kind = PolygonQueryKind::Ray,
                                          .team = team,
                                          .player = (mask & 1U) != 0,
                                          .flag = (mask & 2U) != 0,
                                          .bullet = (mask & 4U) != 0 }),
                          expected);
            }
        }
    }
}

TEST(PolygonCollisionPolicyTest, EverySoldierCorpseAndProjectileTeamCarrierAndPhase)
{
    for (unsigned int id = 0; id < 26; ++id) {
        for (std::uint8_t team = 0; team <= 5; ++team) {
            for (bool carrier : { false, true }) {
                for (const auto phase :
                     { PolygonBackgroundPhase::Normal, PolygonBackgroundPhase::Transition }) {
                    auto expected = SOLDIER_SOLID.at(id);
                    if (id >= 10 && id <= 17) {
                        expected = id % 2 == 1 && team == (id - 10) / 2 + 1;
                    }
                    if (id == 21) {
                        expected = carrier;
                    }
                    if (id == 22) {
                        expected = !carrier;
                    }
                    if (id == 24) {
                        expected = phase == PolygonBackgroundPhase::Normal;
                    }
                    for (auto kind : { PolygonQueryKind::Soldier, PolygonQueryKind::Corpse }) {
                        EXPECT_EQ(AllowsPolygon(static_cast<PMSPolygonType>(id),
                                                { .kind = kind,
                                                  .team = team,
                                                  .carrying_flag = carrier,
                                                  .background = phase }),
                                  expected)
                          << id;
                    }
                    auto bullet = BULLET_SOLID.at(id);
                    if (id >= 10 && id <= 17) {
                        bullet = id % 2 == 0 && team == (id - 10) / 2 + 1;
                    }
                    EXPECT_EQ(AllowsPolygon(static_cast<PMSPolygonType>(id),
                                            { .kind = PolygonQueryKind::Projectile,
                                              .team = team,
                                              .carrying_flag = carrier,
                                              .background = phase }),
                              bullet)
                      << id;
                }
            }
        }
    }
}

TEST(PolygonCollisionPolicyTest, EveryItemAndPointContextRetainsItsOwnRules)
{
    for (unsigned int id = 0; id < 26; ++id) {
        for (std::uint8_t team = 0; team <= 5; ++team) {
            for (bool flag : { false, true }) {
                for (bool owner : { false, true }) {
                    bool expected =
                      id != 1 && id != 2 && id != 3 && id != 21 && id != 22 && id != 25;
                    if (id >= 10 && id <= 17) {
                        expected = !flag && (!owner || (id % 2 == 1 && team == (id - 10) / 2 + 1));
                    }
                    if (id == 23 && owner) {
                        expected = false;
                    }
                    EXPECT_EQ(AllowsPolygon(static_cast<PMSPolygonType>(id),
                                            { .kind = PolygonQueryKind::Item,
                                              .team = team,
                                              .flag_item = flag,
                                              .has_owner = owner }),
                              expected)
                      << id;
                }
                bool point = id != 1 && id != 2 && id != 3 && id != 24 && id != 25 &&
                             !(id >= 10 && id <= 17 && id % 2 == 1);
                if (id >= 21 && id <= 23) {
                    point = flag;
                }
                EXPECT_EQ(AllowsPolygon(static_cast<PMSPolygonType>(id),
                                        { .kind = PolygonQueryKind::Point, .flag = flag }),
                          point)
                  << id;
            }
        }
    }
}

TEST(PolygonCollisionPolicyTest, InvalidTypesAreNeverSolidAndTraitsCoverDiskFormat)
{
    EXPECT_EQ(POLYGON_TRAITS.size(), 26U);
    EXPECT_EQ(POLYGON_TYPES_COUNT, POLYGON_TRAITS.size());
    for (unsigned int id = 26; id <= 255; ++id) {
        for (auto kind : { PolygonQueryKind::Soldier,
                           PolygonQueryKind::Projectile,
                           PolygonQueryKind::Item,
                           PolygonQueryKind::Ray,
                           PolygonQueryKind::Point }) {
            EXPECT_FALSE(AllowsPolygon(static_cast<PMSPolygonType>(id), { .kind = kind }));
        }
    }
    EXPECT_EQ(GetPolygonTraits(PMSPolygonType::Lava).effect, PolygonEffect::Lava);
    EXPECT_TRUE(GetPolygonTraits(PMSPolygonType::BackgroundTransition).background);
    // Bravo's literal upstream typo is deliberately not reproduced.
    EXPECT_TRUE(AllowsPolygon(PMSPolygonType::BravoBullets,
                              { .kind = PolygonQueryKind::Projectile, .team = 2 }));
}

TEST(PolygonCollisionPolicyTest, CosmeticContactsUseSparkRulesRatherThanPointOrCarrierMasks)
{
    for (unsigned int id = 0; id < 26; ++id) {
        for (bool carrier : { false, true }) {
            bool expected = id != 1 && id != 2 && id != 3 && id != 23 && id != 24 && id != 25;
            if (id >= 10 && id <= 17) {
                expected = id == 11;
            }
            if (id == 18) {
                expected = carrier;
            }
            EXPECT_EQ(AllowsPolygon(static_cast<PMSPolygonType>(id),
                                    { .kind = PolygonQueryKind::Cosmetic,
                                      .team = 1,
                                      .carrying_flag = carrier,
                                      .has_owner = true }),
                      expected)
              << id;
            EXPECT_FALSE(AllowsPolygon(static_cast<PMSPolygonType>(id),
                                       { .kind = PolygonQueryKind::Cosmetic }));
        }
    }
}
