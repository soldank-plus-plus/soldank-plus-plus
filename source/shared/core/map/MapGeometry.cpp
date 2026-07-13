module;

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <spdlog/spdlog.h>

module Shared.Core.Map.Map;

import Extern.Glm;

import Shared.Core.Data.IFileReader;
import Shared.Core.Data.FileReader;
import Shared.Core.Data.IFileWriter;
import Shared.Core.Data.FileWriter;
import Shared.Core.Map.PMSConstants;
import Shared.Core.Map.PMSEnums;
import Shared.Core.Map.PMSStructs;
import Shared.Core.Math.Calc;
import Shared.Core.Utility.Observable;

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
    auto u_x = x - map_data_.polygons[i].vertices[0].x;
    auto u_y = y - map_data_.polygons[i].vertices[0].y;
    auto d = map_data_.polygons[i].perpendiculars[0].x * u_x +
             map_data_.polygons[i].perpendiculars[0].y * u_y;
    if (d < 0.0F) {
        return false;
    }

    u_x = x - map_data_.polygons[i].vertices[1].x;
    u_y = y - map_data_.polygons[i].vertices[1].y;
    d = map_data_.polygons[i].perpendiculars[1].x * u_x +
        map_data_.polygons[i].perpendiculars[1].y * u_y;
    if (d < 0.0F) {
        return false;
    }

    u_x = x - map_data_.polygons[i].vertices[2].x;
    u_y = y - map_data_.polygons[i].vertices[2].y;
    d = map_data_.polygons[i].perpendiculars[2].x * u_x +
        map_data_.polygons[i].perpendiculars[2].y * u_y;
    return d >= 0.0F;
}

bool Map::PointInScenery(glm::vec2 p, const PMSScenery& scenery)
{
    auto scenery_vertex_positions = GetSceneryVertexPositions(scenery);

    glm::vec2 a = scenery_vertex_positions.at(0);
    glm::vec2 b = scenery_vertex_positions.at(1);
    if (Calc::Det(a, b, p) > 0) {
        return false;
    }

    a = scenery_vertex_positions.at(1);
    b = scenery_vertex_positions.at(2);
    if (Calc::Det(a, b, p) > 0) {
        return false;
    }

    a = scenery_vertex_positions.at(2);
    b = scenery_vertex_positions.at(3);
    if (Calc::Det(a, b, p) > 0) {
        return false;
    }

    a = scenery_vertex_positions.at(3);
    b = scenery_vertex_positions.at(0);
    return Calc::Det(a, b, p) <= 0;
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
    constexpr const std::array EXCLUDED1 = {
        PMSPolygonType::OnlyBulletsCollide, PMSPolygonType::OnlyPlayersCollide,
        PMSPolygonType::NoCollide,          PMSPolygonType::AlphaPlayers,
        PMSPolygonType::BravoPlayers,       PMSPolygonType::CharliePlayers,
        PMSPolygonType::DeltaPlayers, /* TODO: add those PMSPolygonType::BACKGROUND,
                                         PMSPolygonType::BACKGROUND_TRANSITION*/
    };

    constexpr const std::array EXCLUDED2 = {
        PMSPolygonType::FlaggerCollides,
        PMSPolygonType::NonFlaggerCollides, /* TODO: what's that:
                                               PMSPolygonType::NOT_FLAGGERS???*/
    };

    auto rx = ((int)std::round((pos.x / (float)GetSectorsSize()))) + 25;
    auto ry = ((int)std::round((pos.y / (float)GetSectorsSize()))) + 25;
    if ((rx > 0) && (rx < GetSectorsCount() + 25) && (ry > 0) && (ry < GetSectorsCount() + 25)) {
        for (unsigned short polygon_id : GetSector(rx, ry).polygons) {
            auto poly = GetPolygons().at(polygon_id - 1);

            if (!std::ranges::contains(EXCLUDED1, poly.polygon_type) &&
                (is_flag || !std::ranges::contains(EXCLUDED2, poly.polygon_type))) {
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

    bool npCol = !player;
    bool nbCol = !bullet;

    for (unsigned int i = ax; i < bx; ++i) {
        for (unsigned int j = ay; j < by; ++j) {
            for (const auto& polygon_id : GetSector(i, j).polygons) {
                const PMSPolygon& polygon = map_data_.polygons.at(polygon_id - 1);

                bool testcol = true;
                // TODO: replace team_id with some enum
                if ((polygon.polygon_type == PMSPolygonType::AlphaBullets &&
                     (team_id != 1 || nbCol)) ||
                    (polygon.polygon_type == PMSPolygonType::AlphaPlayers &&
                     (team_id != 1 || npCol))) {

                    testcol = false;
                }

                if ((polygon.polygon_type == PMSPolygonType::BravoBullets &&
                     (team_id != 2 || nbCol)) ||
                    (polygon.polygon_type == PMSPolygonType::BravoPlayers &&
                     (team_id != 2 || npCol))) {

                    testcol = false;
                }

                if ((polygon.polygon_type == PMSPolygonType::CharlieBullets &&
                     (team_id != 3 || nbCol)) ||
                    (polygon.polygon_type == PMSPolygonType::CharliePlayers &&
                     (team_id != 3 || npCol))) {

                    testcol = false;
                }

                if ((polygon.polygon_type == PMSPolygonType::DeltaBullets &&
                     (team_id != 4 || nbCol)) ||
                    (polygon.polygon_type == PMSPolygonType::DeltaPlayers &&
                     (team_id != 4 || npCol))) {

                    testcol = false;
                }

                if (((!flag || npCol) &&
                     (polygon.polygon_type == PMSPolygonType::FlaggerCollides)) ||
                    ((flag || npCol) &&
                     polygon.polygon_type == PMSPolygonType::NonFlaggerCollides)) {

                    testcol = false;
                }

                if ((!flag || npCol || nbCol) &&
                    polygon.polygon_type == PMSPolygonType::NonFlaggerCollides) {

                    testcol = false;
                }

                if (
              (polygon.polygon_type == PMSPolygonType::OnlyBulletsCollide && nbCol) ||
              (polygon.polygon_type == PMSPolygonType::OnlyPlayersCollide && npCol) || (polygon.polygon_type == PMSPolygonType::NoCollide
              // TODO: add this when those are implemented
              /*|| polygon.polygon_type == PMSPolygonType::POLY_TYPE_BACKGROUND || polygon.polygon_type == PMSPolygonType::POLY_TYPE_BACKGROUND_TRANSITION*/)) {

                    testcol = false;
                }

                if (testcol) {
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
