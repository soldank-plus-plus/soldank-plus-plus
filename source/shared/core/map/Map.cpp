#include "Map.hpp"

#include "core/map/Map.hpp"
#include "core/map/PMSConstants.hpp"
#include "core/map/PMSEnums.hpp"
#include "core/math/Calc.hpp"

#include <algorithm>
#include <array>
#include <spdlog/spdlog.h>
#include <utility>
#include <sstream>

namespace Soldank
{
void Map::CreateEmptyMap()
{
    map_data_.boundaries_xy[TopBoundary] = -MAP_BOUNDARY;
    map_data_.boundaries_xy[BottomBoundary] = MAP_BOUNDARY;
    map_data_.boundaries_xy[LeftBoundary] = -MAP_BOUNDARY;
    map_data_.boundaries_xy[RightBoundary] = MAP_BOUNDARY;

    map_data_.name = "Untitled.pms";
    map_data_.description = "New Soldank++ map";
    map_data_.texture_name = "banana.png";

    map_data_.polygons_min_x = 0.0F;
    map_data_.polygons_max_x = 0.0F;
    map_data_.polygons_min_y = 0.0F;
    map_data_.polygons_max_y = 0.0F;

    map_data_.sectors_size = 50;
    map_data_.sectors_count = 50;
    int n = 2 * map_data_.sectors_count + 1;
    map_data_.sectors_poly = std::vector<std::vector<PMSSector>>(n, std::vector<PMSSector>(n));

    map_data_.background_top_color = PMSColor(100, 200, 100, 255);
    map_data_.background_bottom_color = PMSColor(50, 50, 50, 255);

    UpdateBoundaries();
}

void Map::LoadMap(const std::filesystem::path& map_path, const IFileReader& file_reader)
{
    // TODO: Add map validation, whether the map was loaded correctly. Check the sizes of arrays
    auto file_data = file_reader.Read(map_path.string(), std::ios::in | std::ios::binary);
    if (!file_data.has_value()) {
        spdlog::critical("Map not found {}", map_path.string());
        // TODO: should return an error
        return;
    }
    std::stringstream data_buffer{ *file_data };

    map_data_.name = map_path.filename().string();

    map_data_.boundaries_xy[TopBoundary] = -MAP_BOUNDARY;
    map_data_.boundaries_xy[BottomBoundary] = MAP_BOUNDARY;
    map_data_.boundaries_xy[LeftBoundary] = -MAP_BOUNDARY;
    map_data_.boundaries_xy[RightBoundary] = MAP_BOUNDARY;

    map_data_.polygons_min_x = 0.0F;
    map_data_.polygons_max_x = 0.0F;
    map_data_.polygons_min_y = 0.0F;
    map_data_.polygons_max_y = 0.0F;

    ReadFromBuffer(data_buffer, map_data_.version);
    ReadStringFromBuffer(data_buffer, map_data_.description, DESCRIPTION_MAX_LENGTH);
    ReadStringFromBuffer(data_buffer, map_data_.texture_name, TEXTURE_NAME_MAX_LENGTH);

    ReadFromBuffer(data_buffer, map_data_.background_top_color);
    ReadFromBuffer(data_buffer, map_data_.background_bottom_color);
    ReadFromBuffer(data_buffer, map_data_.jet_count);
    ReadFromBuffer(data_buffer, map_data_.grenades_count);
    ReadFromBuffer(data_buffer, map_data_.medikits_count);
    ReadFromBuffer(data_buffer, map_data_.weather_type);
    ReadFromBuffer(data_buffer, map_data_.step_type);
    ReadFromBuffer(data_buffer, map_data_.random_id);

    ReadPolygonsFromBuffer(data_buffer);
    ReadSectorsFromBuffer(data_buffer);

    ReadSceneryInstancesFromBuffer(data_buffer);
    ReadSceneryTypesFromBuffer(data_buffer);

    ReadCollidersFromBuffer(data_buffer);
    ReadSpawnPointsFromBuffer(data_buffer);
    ReadWayPointsFromBuffer(data_buffer);

    UpdateBoundaries();
}

void Map::ReadPolygonsFromBuffer(std::stringstream& buffer)
{
    int polygons_count = 0;
    ReadFromBuffer(buffer, polygons_count);
    map_data_.polygons.clear();
    for (int i = 0; i < polygons_count; i++) {
        PMSPolygon new_polygon;
        new_polygon.id = i;

        for (unsigned int j = 0; j < 3; j++) {
            ReadFromBuffer(buffer, new_polygon.vertices.at(j));

            if (new_polygon.vertices.at(j).x < map_data_.polygons_min_x) {
                map_data_.polygons_min_x = new_polygon.vertices.at(j).x;
            }
            if (new_polygon.vertices.at(j).x > map_data_.polygons_max_x) {
                map_data_.polygons_max_x = new_polygon.vertices.at(j).x;
            }

            if (new_polygon.vertices.at(j).y < map_data_.polygons_min_y) {
                map_data_.polygons_min_y = new_polygon.vertices.at(j).y;
            }
            if (new_polygon.vertices.at(j).y > map_data_.polygons_max_y) {
                map_data_.polygons_max_y = new_polygon.vertices.at(j).y;
            }
        }
        for (unsigned int j = 0; j < 3; j++) {
            ReadFromBuffer(buffer, new_polygon.perpendiculars.at(j));
        }
        new_polygon.bounciness =
          glm::length(glm::vec2(new_polygon.perpendiculars[2].x, new_polygon.perpendiculars[2].y));

        for (unsigned int j = 0; j < 3; j++) {
            glm::vec2 normalized_perpendiculars = Calc::Vec2Normalize(
              glm::vec2(new_polygon.perpendiculars.at(j).x, new_polygon.perpendiculars.at(j).y));
            new_polygon.perpendiculars.at(j).x = normalized_perpendiculars.x;
            new_polygon.perpendiculars.at(j).y = normalized_perpendiculars.y;
        }

        ReadFromBuffer(buffer, new_polygon.polygon_type);

        map_data_.polygons.push_back(new_polygon);
    }
}

void Map::ReadSectorsFromBuffer(std::stringstream& buffer)
{

    ReadFromBuffer(buffer, map_data_.sectors_size);
    ReadFromBuffer(buffer, map_data_.sectors_count);

    int n = 2 * map_data_.sectors_count + 1;
    map_data_.sectors_poly = std::vector<std::vector<PMSSector>>(n, std::vector<PMSSector>(n));

    for (auto& sec_i : map_data_.sectors_poly) {
        for (auto& sec_ij : sec_i) {
            unsigned short sector_polygons_count = 0;
            ReadFromBuffer(buffer, sector_polygons_count);

            for (int j = 0; j < sector_polygons_count; j++) {
                sec_ij.polygons.push_back(0);
                ReadFromBuffer(buffer, sec_ij.polygons.back());
            }
        }
    }
}

void Map::ReadSceneryInstancesFromBuffer(std::stringstream& buffer)
{
    int scenery_instances_count = 0;
    ReadFromBuffer(buffer, scenery_instances_count);
    map_data_.scenery_instances.clear();
    for (int i = 0; i < scenery_instances_count; i++) {
        map_data_.scenery_instances.push_back({});
        ReadFromBuffer(buffer, map_data_.scenery_instances.back());
    }
}
void Map::ReadSceneryTypesFromBuffer(std::stringstream& buffer)
{
    int scenery_types_count = 0;
    ReadFromBuffer(buffer, scenery_types_count);
    map_data_.scenery_types.clear();
    for (int i = 0; i < scenery_types_count; i++) {
        map_data_.scenery_types.push_back({});
        ReadStringFromBuffer(buffer, map_data_.scenery_types.back().name, SCENERY_NAME_MAX_LENGTH);
        ReadFromBuffer(buffer, map_data_.scenery_types.back().timestamp);
    }
}
void Map::ReadCollidersFromBuffer(std::stringstream& buffer)
{
    int colliders_count = 0;
    ReadFromBuffer(buffer, colliders_count);
    map_data_.colliders.clear();
    for (int i = 0; i < colliders_count; i++) {
        map_data_.colliders.push_back({});
        ReadFromBuffer(buffer, map_data_.colliders.back());
    }
}
void Map::ReadSpawnPointsFromBuffer(std::stringstream& buffer)
{
    int spawn_points_count = 0;
    ReadFromBuffer(buffer, spawn_points_count);
    map_data_.spawn_points.clear();
    for (int i = 0; i < spawn_points_count; i++) {
        map_data_.spawn_points.push_back({});
        ReadFromBuffer(buffer, map_data_.spawn_points.back());
    }
}

void Map::ReadWayPointsFromBuffer(std::stringstream& buffer)
{
    int way_points_count = 0;
    ReadFromBuffer(buffer, way_points_count);
    for (int i = 0; i < way_points_count; i++) {
        map_data_.way_points.push_back({});
        ReadFromBuffer(buffer, map_data_.way_points.back());
    }
}

void Map::UpdateBoundaries()
{
    map_data_.width = fabs(map_data_.polygons_max_x - map_data_.polygons_min_x);
    map_data_.height = fabs(map_data_.polygons_max_y - map_data_.polygons_min_y);

    map_data_.center_x = floor((map_data_.polygons_min_x + map_data_.polygons_max_x) / 2.0F);
    map_data_.center_y = floor((map_data_.polygons_min_y + map_data_.polygons_max_y) / 2.0F);

    map_data_.boundaries_xy[TopBoundary] = map_data_.polygons_min_y;
    map_data_.boundaries_xy[BottomBoundary] = map_data_.polygons_max_y;
    map_data_.boundaries_xy[LeftBoundary] = map_data_.polygons_min_x;
    map_data_.boundaries_xy[RightBoundary] = map_data_.polygons_max_x;

    if (map_data_.height > map_data_.width) {
        map_data_.boundaries_xy[LeftBoundary] -= (map_data_.height - map_data_.width) / 2.0F;
        map_data_.boundaries_xy[RightBoundary] += (map_data_.height - map_data_.width) / 2.0F;
    } else {
        map_data_.boundaries_xy[TopBoundary] -= (map_data_.width - map_data_.height) / 2.0F;
        map_data_.boundaries_xy[BottomBoundary] += (map_data_.width - map_data_.height) / 2.0F;
    }

    map_data_.boundaries_xy[TopBoundary] -= MAP_BOUNDARY;
    map_data_.boundaries_xy[BottomBoundary] += MAP_BOUNDARY;
    map_data_.boundaries_xy[LeftBoundary] -= MAP_BOUNDARY;
    map_data_.boundaries_xy[RightBoundary] += MAP_BOUNDARY;
}

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
        PMSPolygonType::NonFlaggerCollides, /* TODO: what's that: PMSPolygonType::NOT_FLAGGERS???*/
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
                  std::uint8_t team_id)
{
    // TODO: implement this
    return false;
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
