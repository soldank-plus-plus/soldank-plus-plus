#include "core/math/Glm.hpp"

#include <gtest/gtest.h>

import Shared.Core.Math.Calc;

TEST(CalcTests, TestDistance)
{
    float expected = 5.0;
    float actual = Soldank::Calc::Distance({ 0.0, 0.0 }, { 3.0, 4.0 });
    ASSERT_FLOAT_EQ(expected, actual);
    expected = 0.0;
    actual = Soldank::Calc::Distance({ 4.5668F, -2.60855F }, { 4.5668F, -2.60855F });
    ASSERT_FLOAT_EQ(expected, actual);
    expected = 6.979953;
    actual = Soldank::Calc::Distance({ 4.5668F, -2.60855F }, { 1.05F, -8.6378F });
    ASSERT_FLOAT_EQ(expected, actual);
}

TEST(CalcTests, TestVec2Length)
{
    float expected = 1.4142135;
    float actual = Soldank::Calc::Vec2Length({ 1.0, 1.0 });
    ASSERT_FLOAT_EQ(expected, actual);
    expected = 8.367198;
    actual = Soldank::Calc::Vec2Length({ 3.5, 7.6 });
    ASSERT_FLOAT_EQ(expected, actual);
}

TEST(CalcTests, TestVec2Normalize)
{
    glm::vec2 expected = { 0.70710677, 0.70710677 };
    glm::vec2 actual = Soldank::Calc::Vec2Normalize({ 1.0, 1.0 });
    ASSERT_FLOAT_EQ(expected.x, actual.x);
    ASSERT_FLOAT_EQ(expected.y, actual.y);
    expected = { 0.90100163, 0.43381557 };
    actual = Soldank::Calc::Vec2Normalize({ 5.4, 2.6 });
    ASSERT_FLOAT_EQ(expected.x, actual.x);
    ASSERT_FLOAT_EQ(expected.y, actual.y);
}

TEST(CalcTests, TestVec2Angle)
{
    float expected = -0.7853982;
    float actual = Soldank::Calc::Vec2Angle({ 1.0, 1.0 });
    ASSERT_FLOAT_EQ(expected, actual);
    expected = -0.5705828;
    actual = Soldank::Calc::Vec2Angle({ 6.7, 4.3 });
    ASSERT_FLOAT_EQ(expected, actual);
    expected = -1.8925469;
    actual = Soldank::Calc::Vec2Angle({ -2.3, 6.9 });
    ASSERT_FLOAT_EQ(expected, actual);
    expected = 1.8925469;
    actual = Soldank::Calc::Vec2Angle({ -2.3, -6.9 });
    ASSERT_FLOAT_EQ(expected, actual);
}

TEST(CalcTests, TestPointLineDistance)
{
    float expected = 0.0;
    float actual = Soldank::Calc::PointLineDistance({ 1.0, 1.0 }, { 2.0, 1.0 }, { 3.0, 1.0 });
    ASSERT_FLOAT_EQ(expected, actual);
    expected = 22.645206;
    actual = Soldank::Calc::PointLineDistance({ 78.45, -90.1 }, { 15.0, 15.0 }, { -3.0, 1.0 });
    ASSERT_FLOAT_EQ(expected, actual);
    expected = 0.061429512;
    actual = Soldank::Calc::PointLineDistance({ -10.0, -10.0 }, { 2.0, 1.0 }, { 3.0, 2.0 });
    ASSERT_FLOAT_EQ(expected, actual);
}

TEST(CalcTests, TestLerpOnVec)
{
    glm::vec2 expected = { 6.0, 6.0 };
    glm::vec2 actual = Soldank::Calc::Lerp({ 1.0, 1.0 }, { 2.0, 2.0 }, 5.0);
    ASSERT_FLOAT_EQ(expected.x, actual.x);
    ASSERT_FLOAT_EQ(expected.y, actual.y);

    expected = { 25.195002, -6.7679996 };
    actual = Soldank::Calc::Lerp({ -13.4, 1.2 }, { 2.1, -2.0 }, 2.49);
    ASSERT_FLOAT_EQ(expected.x, actual.x);
    ASSERT_FLOAT_EQ(expected.y, actual.y);

    expected = { 1.0F, 1.0F };
    actual = Soldank::Calc::Lerp({ 1e8F, 1e8F }, { 1.0F, 1.0F }, 1.0F);
    ASSERT_FLOAT_EQ(expected.x, actual.x);
    ASSERT_FLOAT_EQ(expected.y, actual.y);

    expected = { 1e8F, 1e8F };
    actual = Soldank::Calc::Lerp({ 1e8F, 1e8F }, { 1.0F, 1.0F }, 0.0F);
    ASSERT_FLOAT_EQ(expected.x, actual.x);
    ASSERT_FLOAT_EQ(expected.y, actual.y);

    expected = { 5e07F, 5e07F };
    actual = Soldank::Calc::Lerp({ 1e8F, 1e8F }, { 1.0F, 1.0F }, 0.5F);
    ASSERT_FLOAT_EQ(expected.x, actual.x);
    ASSERT_FLOAT_EQ(expected.y, actual.y);
}

TEST(CalcTests, TestLerpOnFloats)
{
    float a = 1e8F;
    float b = 1.0F;
    float expected = 5e07F;
    float actual = Soldank::Calc::Lerp(a, b, 0.5F);
    ASSERT_FLOAT_EQ(expected, actual);

    expected = a;
    actual = Soldank::Calc::Lerp(1e8F, 1.0F, 0.0F);
    ASSERT_FLOAT_EQ(expected, actual);

    expected = b;
    actual = Soldank::Calc::Lerp(1e8F, 1.0F, 1.0F);
    ASSERT_FLOAT_EQ(expected, actual);
}

TEST(CalcTests, TestSquareDistance)
{
    float expected = 5.0 * 5.0;
    float actual = Soldank::Calc::SquareDistance({ 0.0, 0.0 }, { 3.0, 4.0 });
    ASSERT_FLOAT_EQ(expected, actual);
    expected = 0.0;
    actual = Soldank::Calc::SquareDistance({ 4.5668F, -2.60855F }, { 4.5668F, -2.60855F });
    ASSERT_FLOAT_EQ(expected, actual);
    expected = 6.979953 * 6.979953;
    actual = Soldank::Calc::SquareDistance({ 4.5668F, -2.60855F }, { 1.05F, -8.6378F });
    ASSERT_FLOAT_EQ(expected, actual);
}

TEST(CalcTests, TestVec2Scale)
{
    glm::vec2 expected = { 10.0F, 10.0F };
    glm::vec2 actual = Soldank::Calc::Vec2Scale({ 5.0F, 5.0F }, 2.0F);
    ASSERT_FLOAT_EQ(expected.x, actual.x);
    ASSERT_FLOAT_EQ(expected.y, actual.y);

    expected = { 2.5F, 2.5F };
    actual = Soldank::Calc::Vec2Scale({ 5.0F, 5.0F }, 0.5F);
    ASSERT_FLOAT_EQ(expected.x, actual.x);
    ASSERT_FLOAT_EQ(expected.y, actual.y);
}

TEST(CalcTests, TestLineCircleCollision)
{
    std::optional<glm::vec2> actual =
      Soldank::Calc::LineCircleCollision({ 3.0F, 3.0F }, { 5.0F, 3.0F }, { 5.0F, 5.0F }, 2.0F);
    ASSERT_TRUE(actual.has_value());
    glm::vec2 collision_point = *actual;
    ASSERT_FLOAT_EQ(collision_point.x, 5.0F);
    ASSERT_FLOAT_EQ(collision_point.y, 3.0F);

    actual =
      Soldank::Calc::LineCircleCollision({ 3.0F, 3.0F }, { 4.0F, 6.0F }, { 5.0F, 5.0F }, 2.0F);
    ASSERT_TRUE(actual.has_value());
    collision_point = *actual;
    ASSERT_FLOAT_EQ(collision_point.x, 4.0F);
    ASSERT_FLOAT_EQ(collision_point.y, 6.0F);

    actual =
      Soldank::Calc::LineCircleCollision({ 3.0F, 4.0F }, { 4.0F, 3.0F }, { 5.0F, 5.0F }, 2.0F);
    ASSERT_FALSE(actual.has_value());
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
