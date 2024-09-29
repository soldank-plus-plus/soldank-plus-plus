#ifndef __CALC_HPP__
#define __CALC_HPP__

#include "core/math/Glm.hpp"

#include <cmath>
#include <algorithm>
#include <optional>
#include <vector>

namespace Soldank::Calc
{
float Distance(const glm::vec2& p1, const glm::vec2& p2);
float SquareDistance(const glm::vec2& p1, const glm::vec2& p2);
float Vec2Length(const glm::vec2& v);
glm::vec2 Vec2Normalize(const glm::vec2& v);
float Vec2Angle(const glm::vec2& v);
glm::vec2 Vec2Scale(const glm::vec2& v, float scale);
float PointLineDistance(const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3);

glm::vec2 Lerp(glm::vec2 a, glm::vec2 b, float t);
float Lerp(float a, float b, float t);

std::vector<glm::vec2> LineCircleCollisionPoints(glm::vec2 line1,
                                                 glm::vec2 line2,
                                                 glm::vec2 circle_center,
                                                 float radius);
std::optional<glm::vec2> LineCircleCollision(const glm::vec2& start_point,
                                             const glm::vec2& end_point,
                                             const glm::vec2& circle_center,
                                             float radius);
bool SegmentsIntersect(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec2 d);
} // namespace Soldank::Calc

#endif
