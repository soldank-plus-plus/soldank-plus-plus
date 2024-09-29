#include "Calc.hpp"

#include <math.h>

#include <cmath>

namespace Soldank::Calc
{
float Distance(const glm::vec2& p1, const glm::vec2& p2)
{
    return glm::length(p2 - p1);
}

float SquareDistance(const glm::vec2& p1, const glm::vec2& p2)
{
    glm::vec2 diff = p1 - p2;
    return glm::dot(diff, diff);
}

float Vec2Length(const glm::vec2& v)
{
    return glm::length(v);
}

glm::vec2 Vec2Normalize(const glm::vec2& v)
{
    auto magnitude = glm::length(v);
    if (magnitude < 0.001) {
        return { 0.0F, 0.0F };
    }

    return v / magnitude;
}

float Vec2Angle(const glm::vec2& v)
{
    return (float)(-atan2(0.0, 1.0) - atan2(v.y, v.x));
}
glm::vec2 Vec2Scale(const glm::vec2& v, float scale)
{
    return v * scale;
}

float PointLineDistance(const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3)
{
    auto u = ((p3.x - p1.x) * (p2.x - p1.x) + (p3.y - p1.y) * (p2.y - p1.y)) /
             (std::pow(p2.x - p1.x, 2) + std::pow(p2.y - p1.y, 2));

    auto x = p1.x + u * (p2.x - p1.x);
    auto y = p1.y + u * (p2.y - p1.y);

    return (float)std::sqrt(std::pow(x - p3.x, 2) + std::pow(y - p3.y, 2));
}

glm::vec2 Lerp(glm::vec2 a, glm::vec2 b, float t)
{
    return glm::mix(a, b, t);
}

float Lerp(float a, float b, float t)
{
    return std::lerp(a, b, t);
}

std::vector<glm::vec2> LineCircleCollisionPoints(glm::vec2 line1,
                                                 glm::vec2 line2,
                                                 glm::vec2 circle_center,
                                                 float radius)
{
    float diff_x = line2.x - line1.x;
    float diff_y = line2.y - line1.y;

    if (std::abs(diff_x) < 0.00001F && std::abs(diff_y) < 0.00001F) {
        // Floating point inaccuracy
        return {};
    }

    // If the angle of the bullet is bigger than 45 degrees,
    // flip the coordinate system.
    // This algorithm deals with lines being nearly horizontal just fine,
    // but nearly vertical would cause a havoc, as vertical line is not a function.
    bool flipped = false;
    if (std::abs(diff_y) > std::abs(diff_x)) {
        flipped = true;
        std::swap(line1.x, line1.y);
        std::swap(line2.x, line2.y);
        std::swap(circle_center.x, circle_center.y);
        std::swap(diff_x, diff_x);
    }

    // Line equation: ax + b - y = 0. given x1, y1, x2, y2, let's calculate a and b
    // a = (y1 - y2)/(x1 - x2)
    float a = diff_y / diff_x;
    // b := y - ax
    float b = line1.y - a * line1.x;
    // Circle equation: (x - x1)^2 + (y - y1)^2 - r^2 = 0
    // Now we need to solve: (x - x1)^2 + (y - y1)^2 - r^2 = ax + b - y
    // Simplyfing above: (a^2 + 1)x^2 + 2(ab − ay1 − x1)x + (y1^2 − r^2 + x1^2 − 2by1b^2)=0
    // now, since this is a standard Ax^2 + Bx + C equation, we find x and y using
    // x = (-B +/- sqrt(B^2 - 4ac))/(2A)
    // A = (a^2 + 1)
    float a1 = (a * a) + 1;
    // B = 2(ab - ay1 - x1)
    float b1 = 2 * (a * b - a * circle_center.y - circle_center.x);
    // C = y1^2 − r^2 + x1^2 − 2by1 + b^2
    float c1 = (circle_center.y * circle_center.y) - (radius * radius) +
               (circle_center.x * circle_center.x) - 2 * b * circle_center.y + (b * b);
    // delta = B^2 - 4AC;
    float delta = (b1 * b1) - 4 * a1 * c1;
    // having x1 and x2 result, we can calculate y1 and y2 from y = a * x + b

    // if delta < 0, no intersection
    if (delta < 0) {
        return {};
    }

    float minx = NAN;
    float miny = NAN;
    float maxx = NAN;
    float maxy = NAN;

    if (line1.x < line2.x) {
        minx = line1.x;
        maxx = line2.x;
    } else {
        minx = line2.x;
        maxx = line1.x;
    }

    if (line1.y < line2.y) {
        miny = line1.y;
        maxy = line2.y;
    } else {
        miny = line2.y;
        maxy = line1.y;
    }

    std::vector<glm::vec2> result;
    glm::vec2 intersect;
    // we don't care about a case of delta = 0 as it's extremaly rare,
    // also this will handle it fine, just less effecient
    float sqrtdelta = std::sqrt(delta);
    float a2 = 2 * a1;
    intersect.x = (-b1 - sqrtdelta) / a2;
    intersect.y = a * intersect.x + b;
    // we know that infinite line does intersect the circle, now let's see if our part does
    auto in_range = [](float a, float b, float c) { return a >= b && a <= c; };
    if (in_range(intersect.x, minx, maxx) && in_range(intersect.y, miny, maxy)) {
        if (flipped) {
            std::swap(intersect.x, intersect.y);
        }

        result.push_back(intersect);
    }

    intersect.x = (-b1 + sqrtdelta) / a2;
    intersect.y = a * intersect.x + b;
    if (in_range(intersect.x, minx, maxx) && in_range(intersect.y, miny, maxy)) {
        if (flipped) {
            std::swap(intersect.x, intersect.y);
        }

        result.push_back(intersect);
    }

    return result;
}

std::optional<glm::vec2> LineCircleCollision(const glm::vec2& start_point,
                                             const glm::vec2& end_point,
                                             const glm::vec2& circle_center,
                                             float radius)
{
    float r2 = radius * radius;

    if (SquareDistance(start_point, circle_center) <= r2) {
        return start_point;
    }

    if (SquareDistance(end_point, circle_center) <= r2) {
        return end_point;
    }

    auto intersection_result =
      LineCircleCollisionPoints(start_point, end_point, circle_center, radius);
    if (!intersection_result.empty()) {
        if (intersection_result.size() == 2 &&
            SquareDistance(intersection_result[0], start_point) >
              SquareDistance(intersection_result[1], start_point)) {

            return intersection_result[1];
        }

        return intersection_result[0];
    }
    return std::nullopt;
}

bool SegmentsIntersect(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec2 d)
{
    float segment_ab_center_x = b.x - a.x;
    float segment_ab_center_y = b.y - a.y;
    float segment_cd_center_x = d.x - c.x;
    float segment_cd_center_y = d.y - c.y;

    float s = NAN;
    float t = NAN;
    s = (-segment_ab_center_y * (a.x - c.x) + segment_ab_center_x * (a.y - c.y)) /
        (-segment_cd_center_x * segment_ab_center_y + segment_ab_center_x * segment_cd_center_y);
    t = (segment_cd_center_x * (a.y - c.y) - segment_cd_center_y * (a.x - c.x)) /
        (-segment_cd_center_x * segment_ab_center_y + segment_ab_center_x * segment_cd_center_y);

    return s >= 0 && s <= 1 && t >= 0 && t <= 1;
}
} // namespace Soldank::Calc
