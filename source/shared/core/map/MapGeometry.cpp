module;

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <optional>
#include <vector>

module Shared.Core.Map.Map;

import Extern.Glm;

import Shared.Core.Map.PMSEnums;
import Shared.Core.Map.PMSStructs;
import Shared.Core.Math.Calc;

namespace
{
struct RayCastCollisionPolicy
{
    bool player;
    bool flag;
    bool bullet;
    std::uint8_t team_id;
};

bool PolygonCollidesWith(Soldank::PMSPolygonType polygon_type, const RayCastCollisionPolicy& policy)
{
    using Soldank::PMSPolygonType;
    switch (polygon_type) {
        case PMSPolygonType::AlphaBullets:
            return policy.bullet && policy.team_id == 1;
        case PMSPolygonType::AlphaPlayers:
            return policy.player && policy.team_id == 1;
        case PMSPolygonType::BravoBullets:
            return policy.bullet && policy.team_id == 2;
        case PMSPolygonType::BravoPlayers:
            return policy.player && policy.team_id == 2;
        case PMSPolygonType::CharlieBullets:
            return policy.bullet && policy.team_id == 3;
        case PMSPolygonType::CharliePlayers:
            return policy.player && policy.team_id == 3;
        case PMSPolygonType::DeltaBullets:
            return policy.bullet && policy.team_id == 4;
        case PMSPolygonType::DeltaPlayers:
            return policy.player && policy.team_id == 4;
        case PMSPolygonType::FlaggerCollides:
            return policy.player && policy.flag;
        case PMSPolygonType::NonFlaggerCollides:
            // Preserve the existing RayCast behavior. Its two legacy filters jointly reject this
            // polygon type for every player/flag/bullet combination.
            return false;
        case PMSPolygonType::OnlyBulletsCollide:
            return policy.bullet;
        case PMSPolygonType::OnlyPlayersCollide:
            return policy.player;
        case PMSPolygonType::NoCollide:
            return false;
        default:
            return true;
    }
}

bool PolygonCollidesWithCollisionTest(Soldank::PMSPolygonType polygon_type, bool is_flag)
{
    using Soldank::PMSPolygonType;
    constexpr std::array ALWAYS_EXCLUDED{
        PMSPolygonType::OnlyBulletsCollide, PMSPolygonType::OnlyPlayersCollide,
        PMSPolygonType::NoCollide,          PMSPolygonType::AlphaPlayers,
        PMSPolygonType::BravoPlayers,       PMSPolygonType::CharliePlayers,
        PMSPolygonType::DeltaPlayers,
    };
    constexpr std::array NON_FLAG_EXCLUDED{
        PMSPolygonType::FlaggerCollides,
        PMSPolygonType::NonFlaggerCollides,
    };

    return !std::ranges::contains(ALWAYS_EXCLUDED, polygon_type) &&
           (is_flag || !std::ranges::contains(NON_FLAG_EXCLUDED, polygon_type));
}
} // namespace

namespace Soldank
{
bool Map::PointInPoly(glm::vec2 p, PMSPolygon poly)
{
    auto a = poly.vertices[0];
    auto b = poly.vertices[1];
    auto c = poly.vertices[2];

    auto ap_x = p.x - a.x;
    auto ap_y = p.y - a.y;
    auto p_ab = (b.x - a.x) * ap_y - (b.y - a.y) * ap_x > 0.0F;
    auto p_ac = (c.x - a.x) * ap_y - (c.y - a.y) * ap_x > 0.0F;

    if (p_ac == p_ab) {
        return false;
    }

    if (((c.x - b.x) * (p.y - b.y) - (c.y - b.y) * (p.x - b.x) > 0.0F) != p_ab) {
        return false;
    }

    return true;
}

bool Map::PointInPolyEdges(float x, float y, int i) const
{
    const auto& polygon = map_data_.polygons.at(i);
    for (std::size_t edge = 0; edge < polygon.vertices.size(); ++edge) {
        const float u_x = x - polygon.vertices.at(edge).x;
        const float u_y = y - polygon.vertices.at(edge).y;
        const float distance =
          polygon.perpendiculars.at(edge).x * u_x + polygon.perpendiculars.at(edge).y * u_y;
        if (distance < 0.0F) {
            return false;
        }
    }
    return true;
}

bool Map::PointInScenery(glm::vec2 p, const PMSScenery& scenery)
{
    auto scenery_vertex_positions = GetSceneryVertexPositions(scenery);

    for (std::size_t edge = 0; edge < scenery_vertex_positions.size(); ++edge) {
        const auto& a = scenery_vertex_positions.at(edge);
        const auto& b = scenery_vertex_positions.at((edge + 1) % scenery_vertex_positions.size());
        if (Calc::Det(a, b, p) > 0.0F) {
            return false;
        }
    }
    return true;
}

glm::vec2 Map::ClosestPerpendicular(int j, glm::vec2 pos, float* d, int* n) const
{
    auto px = std::array{
        map_data_.polygons[j].vertices[0].x,
        map_data_.polygons[j].vertices[1].x,
        map_data_.polygons[j].vertices[2].x,
    };

    auto py = std::array{
        map_data_.polygons[j].vertices[0].y,
        map_data_.polygons[j].vertices[1].y,
        map_data_.polygons[j].vertices[2].y,
    };

    auto p1 = glm::vec2(px[0], py[0]);
    auto p2 = glm::vec2(px[1], py[1]);

    auto d1 = Soldank::Calc::PointLineDistance(p1, p2, pos);

    *d = d1;

    auto edge_v1 = 1;
    auto edge_v2 = 2;

    p1.x = px[1];
    p1.y = py[1];

    p2.x = px[2];
    p2.y = py[2];

    auto d2 = Soldank::Calc::PointLineDistance(p1, p2, pos);

    if (d2 < d1) {
        edge_v1 = 2;
        edge_v2 = 3;
        *d = d2;
    }

    p1.x = px[2];
    p1.y = py[2];

    p2.x = px[0];
    p2.y = py[0];

    auto d3 = Soldank::Calc::PointLineDistance(p1, p2, pos);

    if ((d3 < d2) && (d3 < d1)) {
        edge_v1 = 3;
        edge_v2 = 1;
        *d = d3;
    }

    if (edge_v1 == 1 && edge_v2 == 2) {
        *n = 1;
        return { map_data_.polygons[j].perpendiculars[0].x,
                 map_data_.polygons[j].perpendiculars[0].y };
    }

    if (edge_v1 == 2 && edge_v2 == 3) {
        *n = 2;
        return { map_data_.polygons[j].perpendiculars[1].x,
                 map_data_.polygons[j].perpendiculars[1].y };
    }

    if (edge_v1 == 3 && edge_v2 == 1) {
        *n = 3;
        return { map_data_.polygons[j].perpendiculars[2].x,
                 map_data_.polygons[j].perpendiculars[2].y };
    }

    return { 0.0F, 0.0F };
}

bool Map::CollisionTest(glm::vec2 pos, glm::vec2& perp_vec, bool is_flag) const
{
    const glm::ivec2 sector_index = GetSectorIndex(pos);
    auto rx = sector_index.x;
    auto ry = sector_index.y;
    if ((rx > 0) && (rx < GetSectorsCount() + 25) && (ry > 0) && (ry < GetSectorsCount() + 25)) {
        for (unsigned short polygon_id : GetSector(rx, ry).polygons) {
            auto poly = GetPolygons().at(polygon_id - 1);

            if (PolygonCollidesWithCollisionTest(poly.polygon_type, is_flag)) {
                if (PointInPoly(pos, poly)) {
                    float d = NAN;
                    int b = 0;
                    perp_vec = ClosestPerpendicular(polygon_id - 1, pos, &d, &b);
                    perp_vec = Calc::Vec2Scale(perp_vec, 1.5F * d);
                    return true;
                }
            }
        }
    }

    return false;
}

bool Map::RayCast(glm::vec2 a,
                  glm::vec2 b,
                  float& distance,
                  float max_dist,
                  bool player,
                  bool flag,
                  bool bullet,
                  bool check_collider,
                  std::uint8_t team_id) const
{
    distance = Calc::Vec2Length(a - b);
    if (distance > max_dist) {
        distance = 9999999.0F;
        return true;
    }

    int ax = ((int)std::round(std::min(a.x, b.x) / (float)map_data_.sectors_size)) + 25;
    int ay = ((int)std::round(std::min(a.y, b.y) / (float)map_data_.sectors_size)) + 25;
    int bx = ((int)std::round(std::max(a.x, b.x) / (float)map_data_.sectors_size)) + 25;
    int by = ((int)std::round(std::max(a.y, b.y) / (float)map_data_.sectors_size)) + 25;

    if (ax > GetSectorsCount() + 25 || bx < 0 || ay > GetSectorsCount() + 25 || by < 0) {
        return false;
    }

    ax = std::max(0, ax);
    ay = std::max(0, ay);
    bx = std::min(GetSectorsCount() + 25, bx);
    by = std::min(GetSectorsCount() + 25, by);

    const RayCastCollisionPolicy collision_policy{ player, flag, bullet, team_id };
    std::vector<bool> tested_polygons(map_data_.polygons.size());

    for (int i = ax; i < bx; ++i) {
        for (int j = ay; j < by; ++j) {
            for (const auto& polygon_id : GetSector(i, j).polygons) {
                if (tested_polygons.at(polygon_id - 1)) {
                    continue;
                }
                tested_polygons.at(polygon_id - 1) = true;
                const PMSPolygon& polygon = map_data_.polygons.at(polygon_id - 1);
                if (PolygonCollidesWith(polygon.polygon_type, collision_policy)) {
                    if (PointInPoly(a, polygon)) {
                        distance = 0;
                        return true;
                    }
                    glm::vec2 d;
                    if (LineInPoly(a, b, polygon, d)) {
                        glm::vec2 c = d - a;
                        distance = Calc::Vec2Length(c);
                        return true;
                    }
                }
            }
        }
    }

    // TODO: Dead code, decide whether it's needed and refactor or delete
    if (check_collider) {
        // check if vector crosses any colliders
        // |A*x + B*y + C| / Sqrt(A^2 + B^2) < r

        float e = a.y - b.y;
        float f = b.x - a.x;
        float g = a.x * b.y - a.y * b.x;
        float h = std::sqrt(e * e + f * f);
        for (const auto& collider : map_data_.colliders) {
            if (collider.active != 0) {
                if (std::abs(e * collider.x + f * collider.y + g) / h <= collider.radius) {
                    float r = Calc::SquareDistance(a, b) + collider.radius * collider.radius;
                    if (Calc::SquareDistance(a, { collider.x, collider.y }) <= r) {
                        if (Calc::SquareDistance(b, { collider.x, collider.y }) <= r) {
                            // TODO: it looks like check_collider never returns true. Check
                            // what's up with that
                            return false;
                        }
                    }
                }
            }
        }
    }

    return false;
}

bool Map::LineInPoly(const glm::vec2& a,
                     const glm::vec2& b,
                     const PMSPolygon& polygon,
                     glm::vec2& v)
{
    for (unsigned int i = 0; i < 3; ++i) {
        unsigned int j = i + 1;
        if (j >= 3) {
            j = 0;
        }

        const auto& p = polygon.vertices.at(i);
        const auto& q = polygon.vertices.at(j);

        if ((std::abs(b.x - a.x) > 0.00001F) || (std::abs(q.x - p.x) > 0.00001F)) {
            if (b.x == a.x) {
                float bk = (q.y - p.y) / (q.x - p.x);
                float bm = p.y - bk * p.x;
                v.x = a.x;
                v.y = bk * v.x + bm;

                if ((v.x > std::min(p.x, q.x)) && (v.x < std::max(p.x, q.x)) &&
                    (v.y > std::min(a.y, b.y)) && (v.y < std::max(a.y, b.y))) {
                    return true;
                }
            } else if (std::abs(q.x - p.x) <= 0.000001F) {
                float ak = (b.y - a.y) / (b.x - a.x);
                float am = a.y - ak * a.x;
                v.x = p.x;
                v.y = ak * v.x + am;

                if ((v.y > std::min(p.y, q.y)) && (v.y < std::max(p.y, q.y)) &&
                    (v.x > std::min(a.x, b.x)) && (v.x < std::max(a.x, b.x))) {
                    return true;
                }
            } else {
                float ak = (b.y - a.y) / (b.x - a.x);
                float bk = (q.y - p.y) / (q.x - p.x);

                if (std::abs(ak - bk) > 0.000001F) {
                    float am = a.y - ak * a.x;
                    float bm = p.y - bk * p.x;
                    v.x = (bm - am) / (ak - bk);
                    v.y = ak * v.x + am;

                    if ((v.x > std::min(p.x, q.x)) && (v.x < std::max(p.x, q.x)) &&
                        (v.x > std::min(a.x, b.x)) && (v.x < std::max(a.x, b.x))) {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

std::array<glm::vec2, 4> Map::GetSceneryVertexPositions(const PMSScenery& scenery)
{
    glm::mat4 transform_matrix(1.0F);
    transform_matrix = glm::rotate(transform_matrix, -scenery.rotation, glm::vec3(0.0, 0.0, 1.0));
    transform_matrix =
      glm::scale(transform_matrix, glm::vec3(scenery.scale_x, scenery.scale_y, 0.0));

    std::array<glm::vec2, 4> vertex_positions{
        glm::vec2{ 0.0F, scenery.height },
        glm::vec2{ scenery.width, scenery.height },
        glm::vec2{ scenery.width, 0.0F },
        glm::vec2{ 0.0F, 0.0F },
    };

    for (auto& vertex_position : vertex_positions) {
        glm::vec4 v =
          transform_matrix * glm::vec4{ vertex_position.x, vertex_position.y, 1.0F, 1.0F };
        vertex_position.x = v.x;
        vertex_position.y = v.y;
        vertex_position.x += scenery.x;
        vertex_position.y += scenery.y;
    }

    return vertex_positions;
}

std::optional<PMSSpawnPoint> Map::FindFirstSpawnPoint(PMSSpawnPointType spawn_point_type) const
{
    for (const auto& spawn_point : GetSpawnPoints()) {
        if (spawn_point.type == spawn_point_type) {
            return spawn_point;
        }
    }

    return std::nullopt;
}
} // namespace Soldank
