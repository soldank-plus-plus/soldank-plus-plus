#include <gtest/gtest.h>
#include <cmath>

import Extern.Glm;
import Shared.Core.Animations;
import Shared.Core.Map.Polygons.PolygonTraits;
import Shared.Core.Physics.Polygons.PolygonSurfaceResponse;

using namespace Soldank;

TEST(PolygonSurfaceResponseTest, BounceUsesIncomingSpeedAndUnitNormal)
{
    for (float strength : { 0.0F, 0.5F, 1.0F, 2.5F, 10.0F }) {
        const auto result = PolygonSurfaceResponse::BounceImpulse({ 0, 4 }, { 3, 4 }, strength);
        EXPECT_FLOAT_EQ(result.x, 0);
        EXPECT_FLOAT_EQ(result.y, 5 * strength);
    }
    EXPECT_EQ(PolygonSurfaceResponse::BounceImpulse({}, { 3, 4 }, 1), glm::vec2{});
    EXPECT_EQ(PolygonSurfaceResponse::BounceImpulse({ 0, 1 }, {}, 1), glm::vec2{});
    EXPECT_EQ(PolygonSurfaceResponse::BounceImpulse({ 0, 1 }, { 3, 4 }, -1), glm::vec2{});
}

TEST(PolygonSurfaceResponseTest, ReferenceStanceFrictionAndIceExceptions)
{
    for (auto surface : { PolygonSurface::Ordinary, PolygonSurface::Ice, PolygonSurface::Bouncy }) {
        for (auto animation : { AnimationType::Stand,
                                AnimationType::Crouch,
                                AnimationType::Fall,
                                AnimationType::ProneMove }) {
            const auto result = PolygonSurfaceResponse::Friction(
              surface, animation, 30, false, { 0, 1 }, { 2, 3 }, { 4, 5 });
            EXPECT_EQ(result.velocity,
                      surface == PolygonSurface::Ordinary ? glm::vec2(0) : glm::vec2(2, 3));
        }
        const auto run = PolygonSurfaceResponse::Friction(
          surface, AnimationType::Run, 1, false, { 0, 1 }, { 2, 3 }, { 4, 5 });
        EXPECT_FLOAT_EQ(run.velocity.x, 1.94F);
        EXPECT_FLOAT_EQ(run.velocity.y, 2.91F);
        const auto crouch = PolygonSurfaceResponse::Friction(
          surface, AnimationType::CrouchRun, 1, false, { 0, 1 }, { 2, 3 }, { 4, 5 });
        EXPECT_FLOAT_EQ(crouch.velocity.x, 1.7F);
        EXPECT_FLOAT_EQ(crouch.velocity.y, 2.91F);
    }
    const auto before = PolygonSurfaceResponse::Friction(
      PolygonSurface::Ordinary, AnimationType::Prone, 24, false, { 0, 1 }, { 2, 3 }, {});
    const auto after = PolygonSurfaceResponse::Friction(
      PolygonSurface::Ordinary, AnimationType::Prone, 25, false, { 0, 1 }, { 2, 3 }, {});
    EXPECT_FLOAT_EQ(before.velocity.x, 1.94F);
    EXPECT_EQ(after.velocity, glm::vec2{});
    const auto moving = PolygonSurfaceResponse::Friction(
      PolygonSurface::Ordinary, AnimationType::Prone, 25, true, { 0, 1 }, { 2, 3 }, {});
    EXPECT_EQ(moving.velocity, glm::vec2(2, 3));
    const auto boundary = PolygonSurfaceResponse::Friction(
      PolygonSurface::Ordinary, AnimationType::Stand, 1, false, { 0, 0.2F }, { 2, 3 }, {});
    EXPECT_EQ(boundary.velocity, glm::vec2(2, 3));
}
