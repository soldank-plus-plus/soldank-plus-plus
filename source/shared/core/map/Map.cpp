#include "Map.hpp"

#include "core/map/Map.hpp"
#include "core/map/PMSConstants.hpp"
#include "core/map/PMSEnums.hpp"
#include "core/map/PMSStructs.hpp"
#include "core/math/Calc.hpp"

#include <algorithm>
#include <array>
#include <cmath>
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

    int n = 2 * map_data_.sectors_count + 1;
    map_data_.sectors_poly = std::vector<std::vector<PMSSector>>(n, std::vector<PMSSector>(n));

    map_data_.background_top_color = PMSColor(100, 200, 100, 255);
    map_data_.background_bottom_color = PMSColor(50, 50, 50, 255);

    UpdateBoundaries();
    GenerateSectors();
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

void Map::SaveMap(const std::filesystem::path& map_path, std::shared_ptr<IFileWriter> file_writer)
{
    UpdateBoundaries();
    GenerateSectors();

    AppendToFileWriter(file_writer, map_data_.version);
    AppendStringToFileWriter(file_writer, map_data_.description, DESCRIPTION_MAX_LENGTH);
    AppendStringToFileWriter(file_writer, map_data_.texture_name, TEXTURE_NAME_MAX_LENGTH);

    AppendToFileWriter(file_writer, map_data_.background_top_color);
    AppendToFileWriter(file_writer, map_data_.background_bottom_color);
    AppendToFileWriter(file_writer, map_data_.jet_count);
    AppendToFileWriter(file_writer, map_data_.grenades_count);
    AppendToFileWriter(file_writer, map_data_.medikits_count);
    AppendToFileWriter(file_writer, map_data_.weather_type);
    AppendToFileWriter(file_writer, map_data_.step_type);
    AppendToFileWriter(file_writer, map_data_.random_id);

    AppendPolygonsToFileWriter(file_writer);
    AppendSectorsToFileWriter(file_writer);
    AppendSceneryInstancesToFileWriter(file_writer);
    AppendSceneryTypesToFileWriter(file_writer);
    AppendCollidersToFileWriter(file_writer);
    AppendSpawnPointsToFileWriter(file_writer);
    AppendWayPointsToFileWriter(file_writer);

    int zero = 0;
    AppendToFileWriter(file_writer, zero);
    AppendToFileWriter(file_writer, zero);

    auto error = file_writer->Write(map_path, std::ios::out | std::ios::binary | std::ios::trunc);
    if (error != FileWriterError::NoError) {
        spdlog::critical("Could not save map: {}", map_path.string());
        return;
    }
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

void Map::AppendPolygonsToFileWriter(std::shared_ptr<IFileWriter>& file_writer)
{
    unsigned int polygons_count = map_data_.polygons.size();
    AppendToFileWriter(file_writer, polygons_count);

    for (auto& polygon : map_data_.polygons) {
        // Polygons' vertices have to be arranged in clock-wise order.
        if (!polygon.AreVerticesClockwise()) {
            PMSVertex tmp = polygon.vertices[1];
            polygon.vertices[1] = polygon.vertices[2];
            polygon.vertices[2] = tmp;
        }

        for (const auto& vertex : polygon.vertices) {
            auto vertex_copy = vertex;
            vertex_copy.x -= map_data_.center_x;
            vertex_copy.y -= map_data_.center_y;
            vertex_copy.z = 1.0F;

            AppendToFileWriter(file_writer, vertex_copy);
        }

        for (int i = 0; i < 3; ++i) {
            unsigned int j = i + 1;
            if (j > 2) {
                j = 0;
            }

            float diff_x = polygon.vertices.at(j).x - polygon.vertices.at(i).x;
            float diff_y = polygon.vertices.at(i).y - polygon.vertices.at(j).y;
            float length = NAN;
            if (fabs(diff_x) < 0.00001F && fabs(diff_y) < 0.00001F) {
                length = 1.0F;
            } else {
                length = hypotf(diff_x, diff_y);
            }

            float bounciness = 1.0F;

            if (polygon.polygon_type == PMSPolygonType::Bouncy) {
                if (bounciness < 1.0F) {
                    bounciness = 1.0F;
                } else {
                    bounciness = polygon.bounciness;
                }
            } else {
                bounciness = 1.0F;
            }

            polygon.perpendiculars.at(i).x = (diff_y / length) * bounciness;
            polygon.perpendiculars.at(i).y = (diff_x / length) * bounciness;
            polygon.perpendiculars.at(i).z = 1.0F;

            AppendToFileWriter(file_writer, polygon.perpendiculars.at(i));
        }
        AppendToFileWriter(file_writer, polygon.polygon_type);
    }
}

void Map::AppendSectorsToFileWriter(std::shared_ptr<IFileWriter>& file_writer)
{
    AppendToFileWriter(file_writer, map_data_.sectors_size);
    /**
     * In VB6/Pascal, an array can have negative indexes. Basically, Soldat creates an
     * array like this: [-sectorsCount...sectorsCount, -sectorsCount...sectorsCount].
     * This is a 2-dimensional array equal to C++'s [sectorsCount * 2 + 1][sectorsCount * 2 + 1].
     */
    AppendToFileWriter(file_writer, map_data_.sectors_count);
    for (int x = 0; x <= map_data_.sectors_count * 2; ++x) {
        for (int y = 0; y <= map_data_.sectors_count * 2; ++y) {
            unsigned short sector_polygons_count = map_data_.sectors_poly[x][y].polygons.size();
            AppendToFileWriter(file_writer, sector_polygons_count);
            for (const auto& polygon_id : map_data_.sectors_poly[x][y].polygons) {
                AppendToFileWriter(file_writer, polygon_id);
            }
        }
    }
}

void Map::AppendSceneryInstancesToFileWriter(std::shared_ptr<IFileWriter>& file_writer)
{
    unsigned int scenery_instances_count = map_data_.scenery_instances.size();
    AppendToFileWriter(file_writer, scenery_instances_count);
    for (const auto& scenery : map_data_.scenery_instances) {
        auto scenery_copy = scenery;
        scenery_copy.x -= map_data_.center_x;
        scenery_copy.y -= map_data_.center_y;
        AppendToFileWriter(file_writer, scenery_copy);
    }
}

void Map::AppendSceneryTypesToFileWriter(std::shared_ptr<IFileWriter>& file_writer)
{
    unsigned int scenery_types_count = map_data_.scenery_types.size();
    AppendToFileWriter(file_writer, scenery_types_count);
    for (const auto& scenery_type : map_data_.scenery_types) {
        AppendStringToFileWriter(file_writer, scenery_type.name, SCENERY_NAME_MAX_LENGTH);
        AppendToFileWriter(file_writer, scenery_type.timestamp);
    }
}

void Map::AppendCollidersToFileWriter(std::shared_ptr<IFileWriter>& file_writer)
{
    unsigned int colliders_count = map_data_.colliders.size();
    AppendToFileWriter(file_writer, colliders_count);
    for (const auto& collider : map_data_.colliders) {
        auto collider_copy = collider;
        collider_copy.x -= map_data_.center_x;
        collider_copy.y -= map_data_.center_y;
        AppendToFileWriter(file_writer, collider_copy);
    }
}

void Map::AppendSpawnPointsToFileWriter(std::shared_ptr<IFileWriter>& file_writer)
{
    unsigned int spawn_points_count = map_data_.spawn_points.size();
    AppendToFileWriter(file_writer, spawn_points_count);
    for (const auto& spawn_point : map_data_.spawn_points) {
        auto spawn_point_copy = spawn_point;
        spawn_point_copy.x -= (int)map_data_.center_x;
        spawn_point_copy.y -= (int)map_data_.center_y;
        AppendToFileWriter(file_writer, spawn_point_copy);
    }
}

void Map::AppendWayPointsToFileWriter(std::shared_ptr<IFileWriter>& file_writer)
{
    unsigned int way_points_count = map_data_.way_points.size();
    AppendToFileWriter(file_writer, way_points_count);
    for (const auto& way_point : map_data_.way_points) {
        auto way_point_copy = way_point;
        way_point_copy.x -= (int)map_data_.center_x;
        way_point_copy.y -= (int)map_data_.center_y;
        AppendToFileWriter(file_writer, way_point_copy);
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

    map_change_events_.changed_background_color.Notify(
      map_data_.background_top_color, map_data_.background_bottom_color, GetBoundaries());
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

std::optional<PMSSpawnPoint> Map::FindFirstSpawnPoint(PMSSpawnPointType spawn_point_type) const
{
    for (const auto& spawn_point : GetSpawnPoints()) {
        if (spawn_point.type == spawn_point_type) {
            return spawn_point;
        }
    }

    return std::nullopt;
}

PMSPolygon Map::AddNewPolygon(const PMSPolygon& polygon)
{

    PMSPolygon new_polygon = polygon;

    new_polygon.id = map_data_.polygons.size();

    SetPolygonVerticesAndPerpendiculars(new_polygon);

    map_data_.polygons.push_back(new_polygon);

    UpdateMinMaxPolygonPositions();
    FixPolygonIds();
    UpdateBoundaries();
    are_sectors_generated_ = false;

    map_change_events_.added_new_polygon.Notify(new_polygon);

    return new_polygon;
}

void Map::AddPolygons(const std::vector<PMSPolygon>& polygons)
{
    std::vector<PMSPolygon> polygons_to_add = polygons;
    std::sort(polygons_to_add.begin(),
              polygons_to_add.end(),
              [](const PMSPolygon& a, const PMSPolygon& b) { return a.id < b.id; });

    for (auto& polygon : polygons_to_add) {
        SetPolygonVerticesAndPerpendiculars(polygon);
    }
    std::vector<PMSPolygon> old_polygons = map_data_.polygons;
    map_data_.polygons.clear();
    unsigned int old_polygons_id = 0;
    unsigned int polygons_to_add_id = 0;
    while (old_polygons_id < old_polygons.size() || polygons_to_add_id < polygons_to_add.size()) {
        if (polygons_to_add_id < polygons_to_add.size() &&
            polygons_to_add.at(polygons_to_add_id).id == map_data_.polygons.size()) {

            map_data_.polygons.push_back(polygons_to_add.at(polygons_to_add_id));
            ++polygons_to_add_id;
        } else {
            map_data_.polygons.push_back(old_polygons.at(old_polygons_id));
            ++old_polygons_id;
        }
    }

    UpdateMinMaxPolygonPositions();
    FixPolygonIds();
    UpdateBoundaries();
    are_sectors_generated_ = false;

    map_change_events_.added_new_polygons.Notify(polygons_to_add, map_data_.polygons);
}

PMSPolygon Map::RemovePolygonById(unsigned int id)
{
    PMSPolygon removed_polygon = map_data_.polygons.at(id);
    map_data_.polygons.erase(map_data_.polygons.begin() + id);
    UpdateMinMaxPolygonPositions();
    FixPolygonIds();
    UpdateBoundaries();
    are_sectors_generated_ = false;

    map_change_events_.removed_polygon.Notify(removed_polygon, map_data_.polygons);

    return removed_polygon;
}

void Map::RemovePolygonsById(const std::vector<unsigned int>& polygon_ids)
{
    std::vector<unsigned int> sorted_polygon_ids = polygon_ids;
    std::sort(sorted_polygon_ids.begin(), sorted_polygon_ids.end());
    std::vector<PMSPolygon> removed_polygons;
    removed_polygons.reserve(polygon_ids.size());
    for (const auto& polygon_id : sorted_polygon_ids) {
        removed_polygons.push_back(map_data_.polygons.at(polygon_id));
    }

    unsigned int polygon_id = 0;
    unsigned int removal_id = 0;
    while (polygon_id + removal_id < map_data_.polygons.size()) {
        while (removal_id < sorted_polygon_ids.size() &&
               polygon_id + removal_id == sorted_polygon_ids.at(removal_id)) {
            ++removal_id;
        }

        if (polygon_id + removal_id >= map_data_.polygons.size()) {
            break;
        }

        if (removal_id > 0) {
            std::swap(map_data_.polygons[polygon_id], map_data_.polygons[polygon_id + removal_id]);
        }

        ++polygon_id;
    }

    for (unsigned i = 0; i < sorted_polygon_ids.size(); ++i) {
        map_data_.polygons.pop_back();
    }

    UpdateMinMaxPolygonPositions();
    FixPolygonIds();
    UpdateBoundaries();
    are_sectors_generated_ = false;

    map_change_events_.removed_polygons.Notify(removed_polygons, map_data_.polygons);
}

void Map::SetPolygonVerticesColorById(
  const std::vector<std::pair<std::pair<unsigned int, unsigned int>, PMSColor>>&
    polygon_vertices_with_new_color)
{
    for (const auto& [polygon_vertex_id, new_color] : polygon_vertices_with_new_color) {
        map_data_.polygons.at(polygon_vertex_id.first).vertices.at(polygon_vertex_id.second).color =
          new_color;
    }

    map_change_events_.modified_polygons.Notify(map_data_.polygons);
}

void Map::MovePolygonVerticesById(
  const std::vector<std::pair<std::pair<unsigned int, unsigned int>, glm::vec2>>&
    polygon_vertices_with_new_position)
{
    for (const auto& [polygon_vertex, new_position] : polygon_vertices_with_new_position) {
        map_data_.polygons.at(polygon_vertex.first).vertices.at(polygon_vertex.second).x =
          new_position.x;
        map_data_.polygons.at(polygon_vertex.first).vertices.at(polygon_vertex.second).y =
          new_position.y;
    }

    UpdateMinMaxPolygonPositions();
    FixPolygonIds();
    UpdateBoundaries();
    are_sectors_generated_ = false;

    map_change_events_.modified_polygons.Notify(map_data_.polygons);
}

void Map::SetPolygonsById(const std::vector<std::pair<unsigned int, PMSPolygon>>& polygons)
{
    for (const auto& [polygon_id, polygon] : polygons) {
        map_data_.polygons.at(polygon_id) = polygon;
    }

    UpdateMinMaxPolygonPositions();
    FixPolygonIds();
    UpdateBoundaries();
    are_sectors_generated_ = false;

    map_change_events_.modified_polygons.Notify(map_data_.polygons);
}

unsigned int Map::AddNewSpawnPoint(const PMSSpawnPoint& spawn_point)
{
    map_data_.spawn_points.push_back(spawn_point);
    unsigned int new_spawn_point_id = map_data_.spawn_points.size() - 1;
    map_change_events_.added_new_spawn_point.Notify(map_data_.spawn_points.back(),
                                                    new_spawn_point_id);
    return new_spawn_point_id;
}

PMSSpawnPoint Map::RemoveSpawnPointById(unsigned int id)
{
    PMSSpawnPoint removed_spawn_point = map_data_.spawn_points.at(id);
    map_data_.spawn_points.erase(map_data_.spawn_points.begin() + id);
    map_change_events_.removed_spawn_point.Notify(removed_spawn_point, id);
    return removed_spawn_point;
}

void Map::MoveSpawnPointsById(
  const std::vector<std::pair<unsigned int, glm::ivec2>>& spawn_point_ids_with_new_position)
{
    for (const auto& [spawn_point_id, new_position] : spawn_point_ids_with_new_position) {
        map_data_.spawn_points.at(spawn_point_id).x = new_position.x;
        map_data_.spawn_points.at(spawn_point_id).y = new_position.y;
    }

    map_change_events_.modified_spawn_points.Notify(map_data_.spawn_points);
}

void Map::SetSpawnPointsById(
  const std::vector<std::pair<unsigned int, PMSSpawnPoint>>& spawn_points)
{
    for (const auto& [spawn_point_id, spawn_point] : spawn_points) {
        map_data_.spawn_points.at(spawn_point_id) = spawn_point;
    }

    map_change_events_.modified_spawn_points.Notify(map_data_.spawn_points);
}

void Map::AddSpawnPoints(const std::vector<std::pair<unsigned int, PMSSpawnPoint>>& spawn_points)
{
    std::vector<std::pair<unsigned int, PMSSpawnPoint>> spawn_points_to_add = spawn_points;
    std::sort(spawn_points_to_add.begin(),
              spawn_points_to_add.end(),
              [](const std::pair<unsigned int, PMSSpawnPoint>& a,
                 const std::pair<unsigned int, PMSSpawnPoint>& b) { return a.first < b.first; });

    std::vector<PMSSpawnPoint> old_spawn_points = map_data_.spawn_points;
    map_data_.spawn_points.clear();
    unsigned int old_spawn_point_id = 0;
    unsigned int spawn_points_to_add_id = 0;

    while (old_spawn_point_id < old_spawn_points.size() ||
           spawn_points_to_add_id < spawn_points_to_add.size()) {

        if (spawn_points_to_add_id < spawn_points_to_add.size() &&
            spawn_points_to_add.at(spawn_points_to_add_id).first == map_data_.spawn_points.size()) {

            map_data_.spawn_points.push_back(spawn_points_to_add.at(spawn_points_to_add_id).second);
            ++spawn_points_to_add_id;
        } else {
            map_data_.spawn_points.push_back(old_spawn_points.at(old_spawn_point_id));
            ++old_spawn_point_id;
        }
    }

    map_change_events_.added_spawn_points.Notify(map_data_.spawn_points);
}

void Map::RemoveSpawnPointsById(const std::vector<unsigned int>& spawn_point_ids)
{
    std::vector<PMSSpawnPoint> old_spawn_points = map_data_.spawn_points;
    std::vector<unsigned int> spawn_point_ids_to_remove = spawn_point_ids;
    std::sort(spawn_point_ids_to_remove.begin(), spawn_point_ids_to_remove.end());
    map_data_.spawn_points.clear();
    unsigned int removal_id = 0;

    for (unsigned int i = 0; i < old_spawn_points.size(); ++i) {
        if (removal_id < spawn_point_ids_to_remove.size() &&
            i == spawn_point_ids_to_remove.at(removal_id)) {

            ++removal_id;
            continue;
        }

        map_data_.spawn_points.push_back(old_spawn_points.at(i));
    }

    map_change_events_.removed_spawn_points.Notify(map_data_.spawn_points);
}

unsigned int Map::AddNewScenery(const PMSScenery& scenery, const std::string& file_name)
{
    bool scenery_type_exists = false;
    unsigned short scenery_style = 0;

    for (const auto& scenery_type : GetSceneryTypes()) {
        if (scenery_type.name == file_name) {
            scenery_type_exists = true;
            break;
        }

        ++scenery_style;
    }

    if (!scenery_type_exists) {
        map_data_.scenery_types.push_back({ .name = file_name, .timestamp = {} });
        scenery_style = map_data_.scenery_types.size() - 1;
        map_change_events_.added_new_scenery_type.Notify(map_data_.scenery_types.back());
    }

    ++scenery_style;

    map_data_.scenery_instances.push_back(scenery);
    map_data_.scenery_instances.back().style = scenery_style;
    unsigned int new_scenery_id = map_data_.scenery_instances.size() - 1;

    map_change_events_.added_new_scenery.Notify(map_data_.scenery_instances.back(), new_scenery_id);

    return new_scenery_id;
}

PMSScenery Map::RemoveSceneryById(unsigned int id)
{
    PMSScenery removed_scenery = map_data_.scenery_instances.at(id);
    unsigned short removed_scenery_style = removed_scenery.style;

    bool other_scenery_with_same_style_exists = false;
    for (unsigned int i = 0; i < GetSceneryInstances().size(); ++i) {
        if (i == id) {
            continue;
        }

        if (GetSceneryInstances().at(i).style == removed_scenery_style) {
            other_scenery_with_same_style_exists = true;
            break;
        }
    }

    map_data_.scenery_instances.erase(map_data_.scenery_instances.begin() + id);

    if (!other_scenery_with_same_style_exists) {
        PMSSceneryType removed_scenery_type = map_data_.scenery_types.at(removed_scenery_style - 1);
        map_data_.scenery_types.erase(map_data_.scenery_types.begin() +
                                      (removed_scenery_style - 1));
        for (auto& scenery : map_data_.scenery_instances) {
            if (scenery.style > removed_scenery_style) {
                --scenery.style;
            }
        }

        map_change_events_.removed_scenery.Notify(removed_scenery, id, map_data_.scenery_instances);
        map_change_events_.removed_scenery_type.Notify(
          removed_scenery_type, removed_scenery_style, map_data_.scenery_types);
    } else {
        map_change_events_.removed_scenery.Notify(removed_scenery, id, map_data_.scenery_instances);
    }

    return removed_scenery;
}

void Map::AddSceneries(
  const std::vector<std::pair<unsigned int, std::pair<PMSScenery, std::string>>>& sceneries)
{
    auto sceneries_to_add = sceneries;
    std::sort(sceneries_to_add.begin(), sceneries_to_add.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });

    for (auto& scenery_to_add : sceneries_to_add) {
        std::string file_name = scenery_to_add.second.second;
        bool found = false;
        for (unsigned int i = 0; i < map_data_.scenery_types.size(); ++i) {
            if (map_data_.scenery_types.at(i).name == file_name) {
                scenery_to_add.second.first.style = i + 1;
                found = true;
                break;
            }
        }

        if (!found) {
            map_data_.scenery_types.push_back({ .name = file_name });
            scenery_to_add.second.first.style = map_data_.scenery_types.size();
            map_change_events_.added_new_scenery_type.Notify(map_data_.scenery_types.back());
        }
    }

    std::vector<PMSScenery> old_sceneries = map_data_.scenery_instances;
    map_data_.scenery_instances.clear();
    unsigned int old_scenery_id = 0;
    unsigned int sceneries_to_add_id = 0;

    while (old_scenery_id < old_sceneries.size() || sceneries_to_add_id < sceneries_to_add.size()) {

        if (sceneries_to_add_id < sceneries_to_add.size() &&
            sceneries_to_add.at(sceneries_to_add_id).first == map_data_.scenery_instances.size()) {

            map_data_.scenery_instances.push_back(
              sceneries_to_add.at(sceneries_to_add_id).second.first);
            ++sceneries_to_add_id;
        } else {
            map_data_.scenery_instances.push_back(old_sceneries.at(old_scenery_id));
            ++old_scenery_id;
        }
    }

    map_change_events_.added_sceneries.Notify(map_data_.scenery_instances);
}

void Map::RemoveSceneriesById(const std::vector<unsigned int>& scenery_ids)
{
    std::vector<PMSScenery> old_sceneries = map_data_.scenery_instances;
    std::vector<unsigned int> scenery_ids_to_remove = scenery_ids;
    std::sort(scenery_ids_to_remove.begin(), scenery_ids_to_remove.end());
    map_data_.scenery_instances.clear();
    unsigned int removal_id = 0;
    std::array<unsigned int, MAX_SCENERIES_COUNT + 10> scenery_type_usage_count{};
    scenery_type_usage_count.fill(0);

    for (unsigned int i = 0; i < old_sceneries.size(); ++i) {
        if (removal_id < scenery_ids_to_remove.size() &&
            i == scenery_ids_to_remove.at(removal_id)) {

            ++removal_id;
            continue;
        }

        map_data_.scenery_instances.push_back(old_sceneries.at(i));
        ++scenery_type_usage_count.at(map_data_.scenery_instances.back().style);
    }

    std::array<unsigned int, MAX_SCENERIES_COUNT + 10> scenery_type_new_style{};
    for (unsigned int i = 0; i < scenery_type_new_style.size(); ++i) {
        scenery_type_new_style.at(i) = i;
    }

    unsigned int acc = 0;
    for (unsigned int i = 1; i < scenery_type_new_style.size(); ++i) {
        if (scenery_type_usage_count.at(i) == 0) {
            scenery_type_new_style.at(i) = 0;
            acc++;
        }

        scenery_type_new_style.at(i) -= acc;
    }

    std::vector<PMSSceneryType> old_scenery_types = map_data_.scenery_types;
    std::vector<std::pair<unsigned short, PMSSceneryType>> removed_scenery_types;
    map_data_.scenery_types.clear();

    for (unsigned int i = 0; i < old_scenery_types.size(); ++i) {
        if (scenery_type_usage_count.at(i + 1) != 0) {
            map_data_.scenery_types.push_back(old_scenery_types.at(i));
        } else {
            removed_scenery_types.emplace_back(i + 1, old_scenery_types.at(i));
        }
    }

    for (auto& scenery_instance : map_data_.scenery_instances) {
        scenery_instance.style = scenery_type_new_style.at(scenery_instance.style);
    }

    map_change_events_.removed_scenery_types.Notify(removed_scenery_types);
    map_change_events_.removed_sceneries.Notify(map_data_.scenery_instances);
}

void Map::SetSceneriesColorById(
  const std::vector<std::pair<unsigned int, PMSColor>>& scenery_ids_with_new_color)
{
    for (const auto& [scenery_id, new_color] : scenery_ids_with_new_color) {
        map_data_.scenery_instances.at(scenery_id).color = new_color;
        map_data_.scenery_instances.at(scenery_id).alpha = new_color.alpha;
    }

    map_change_events_.modified_sceneries.Notify(map_data_.scenery_instances);
}

void Map::MoveSceneriesById(
  const std::vector<std::pair<unsigned int, glm::vec2>>& scenery_ids_with_new_position)
{
    for (const auto& [scenery_id, new_position] : scenery_ids_with_new_position) {
        map_data_.scenery_instances.at(scenery_id).x = new_position.x;
        map_data_.scenery_instances.at(scenery_id).y = new_position.y;
    }

    map_change_events_.modified_sceneries.Notify(map_data_.scenery_instances);
}

void Map::SetSceneriesById(const std::vector<std::pair<unsigned int, PMSScenery>>& sceneries)
{
    for (const auto& [scenery_id, scenery] : sceneries) {
        map_data_.scenery_instances.at(scenery_id) = scenery;
    }

    map_change_events_.modified_sceneries.Notify(map_data_.scenery_instances);
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

void Map::GenerateSectors()
{
    if (are_sectors_generated_) {
        return;
    }
    are_sectors_generated_ = true;

    FixPolygonIds();
    UpdateMinMaxPolygonPositions();
    UpdateBoundaries();
    for (auto& polygon : map_data_.polygons) {
        // Polygons' vertices have to be arranged in clock-wise order.
        if (!polygon.AreVerticesClockwise()) {
            PMSVertex tmp = polygon.vertices[1];
            polygon.vertices[1] = polygon.vertices[2];
            polygon.vertices[2] = tmp;
        }

        for (auto& vertex : polygon.vertices) {
            vertex.x -= map_data_.center_x;
            vertex.y -= map_data_.center_y;
        }
    }
    for (auto& scenery : map_data_.scenery_instances) {
        scenery.x -= map_data_.center_x;
        scenery.y -= map_data_.center_y;
    }
    for (auto& spawn_point : map_data_.spawn_points) {
        spawn_point.x -= map_data_.center_x;
        spawn_point.y -= map_data_.center_y;
    }
    UpdateMinMaxPolygonPositions();
    UpdateBoundaries();
    map_change_events_.modified_polygons.Notify(map_data_.polygons);
    map_change_events_.modified_sceneries.Notify(map_data_.scenery_instances);
    map_change_events_.modified_spawn_points.Notify(map_data_.spawn_points);

    map_data_.sectors_count = 25;
    int n = 2 * map_data_.sectors_count + 1;
    map_data_.sectors_poly = std::vector<std::vector<PMSSector>>(n, std::vector<PMSSector>(n));

    if (map_data_.width > map_data_.height) {
        map_data_.sectors_size = floor((map_data_.width + 2.0 * 100.0F) / (float)(n - 1));
    } else {
        map_data_.sectors_size = floor((map_data_.height + 2.0 * 100.0F) / (float)(n - 1));
    }

    for (int x = 0; x < n; ++x) {
        for (int y = 0; y < n; ++y) {
            map_data_.sectors_poly[x][y].boundaries[LeftBoundary] = std::floor(
              (float)map_data_.sectors_size * ((float)x - (float)map_data_.sectors_count - 0.5F) -
              1.0F + map_data_.center_x);
            map_data_.sectors_poly[x][y].boundaries[TopBoundary] = std::floor(
              (float)map_data_.sectors_size * ((float)y - (float)map_data_.sectors_count - 0.5F) -
              1.0F + map_data_.center_y);
            map_data_.sectors_poly[x][y].boundaries[RightBoundary] =
              map_data_.sectors_poly[x][y].boundaries[LeftBoundary] + (float)map_data_.sectors_size;
            map_data_.sectors_poly[x][y].boundaries[BottomBoundary] =
              map_data_.sectors_poly[x][y].boundaries[TopBoundary] + (float)map_data_.sectors_size;

            for (unsigned int i = 0; i < map_data_.polygons.size(); ++i) {
                if (IsPolygonInSector(i,
                                      floor((float)map_data_.sectors_size *
                                              ((float)x - (float)map_data_.sectors_count - 0.5F) -
                                            1.0F + map_data_.center_x),
                                      floor((float)map_data_.sectors_size *
                                              ((float)y - (float)map_data_.sectors_count - 0.5F) -
                                            1.0F + map_data_.center_y),
                                      (float)map_data_.sectors_size + 2)) {

                    map_data_.sectors_poly[x][y].polygons.push_back(i + 1);
                }
            }
        }
    }
}

bool Map::IsPolygonInSector(unsigned short polygon_index,
                            float sector_x,
                            float sector_y,
                            float sector_size)
{
    if (map_data_.polygons[polygon_index].polygon_type == PMSPolygonType::NoCollide) {
        return false;
    }

    if ((map_data_.polygons[polygon_index].vertices[0].x < sector_x &&
         map_data_.polygons[polygon_index].vertices[1].x < sector_x &&
         map_data_.polygons[polygon_index].vertices[2].x < sector_x) ||
        (map_data_.polygons[polygon_index].vertices[0].x > sector_x + sector_size &&
         map_data_.polygons[polygon_index].vertices[1].x > sector_x + sector_size &&
         map_data_.polygons[polygon_index].vertices[2].x > sector_x + sector_size) ||
        (map_data_.polygons[polygon_index].vertices[0].y < sector_y &&
         map_data_.polygons[polygon_index].vertices[1].y < sector_y &&
         map_data_.polygons[polygon_index].vertices[2].y < sector_y) ||
        (map_data_.polygons[polygon_index].vertices[0].y > sector_y + sector_size &&
         map_data_.polygons[polygon_index].vertices[1].y > sector_y + sector_size &&
         map_data_.polygons[polygon_index].vertices[2].y > sector_y + sector_size)) {
        return false;
    }

    // Check if any of the polygon's vertices is inside the sector.
    unsigned int i = 0;
    for (i = 0; i < 3; ++i) {
        if (map_data_.polygons[polygon_index].vertices.at(i).x >= sector_x &&
            map_data_.polygons[polygon_index].vertices.at(i).x <= sector_x + sector_size &&
            map_data_.polygons[polygon_index].vertices.at(i).y >= sector_y &&
            map_data_.polygons[polygon_index].vertices.at(i).y <= sector_y + sector_size) {
            return true;
        }
    }

    // Check if any of the 4 sector's corners is inside the polygon.
    if (PointInPoly({ sector_x, sector_y }, map_data_.polygons[polygon_index]) ||
        PointInPoly({ sector_x + sector_size, sector_y }, map_data_.polygons[polygon_index]) ||
        PointInPoly({ sector_x + sector_size, sector_y + sector_size },
                    map_data_.polygons[polygon_index]) ||
        PointInPoly({ sector_x, sector_y + sector_size }, map_data_.polygons[polygon_index])) {
        return true;
    }

    /**
     * Check intersections between polygon's sides and sector's sides.
     * AB is polygon's side, CD is sector's side.
     */
    unsigned int j = 0;
    glm::vec2 a;
    glm::vec2 b;
    glm::vec2 c;
    glm::vec2 d;
    for (i = 0; i < 3; ++i) {
        j = i + 1;
        if (j > 2) {
            j = 0;
        }

        a = { map_data_.polygons[polygon_index].vertices.at(i).x,
              map_data_.polygons[polygon_index].vertices.at(i).y };
        b = { map_data_.polygons[polygon_index].vertices.at(j).x,
              map_data_.polygons[polygon_index].vertices.at(j).y };

        // Top side of sector.
        c = { sector_x, sector_y };
        d = { sector_x + sector_size, sector_y };
        if (Calc::SegmentsIntersect(a, b, c, d)) {
            return true;
        }

        // Right side of sector.
        c = { sector_x + sector_size, sector_y };
        d = { sector_x + sector_size, sector_y + sector_size };
        if (Calc::SegmentsIntersect(a, b, c, d)) {
            return true;
        }

        // Bottom side of sector.
        c = { sector_x, sector_y + sector_size };
        d = { sector_x + sector_size, sector_y + sector_size };
        if (Calc::SegmentsIntersect(a, b, c, d)) {
            return true;
        }

        // Left side of sector.
        c = { sector_x, sector_y };
        d = { sector_x, sector_y + sector_size };
        if (Calc::SegmentsIntersect(a, b, c, d)) {
            return true;
        }
    }

    return false;
}

void Map::SetPolygonVerticesAndPerpendiculars(PMSPolygon& polygon)
{
    // Polygons' vertices have to be arranged in clock-wise order.
    if (!polygon.AreVerticesClockwise()) {
        PMSVertex tmp = polygon.vertices[1];
        polygon.vertices[1] = polygon.vertices[2];
        polygon.vertices[2] = tmp;
    }

    for (int i = 0; i < 3; ++i) {
        unsigned int j = i + 1;
        if (j > 2) {
            j = 0;
        }

        float diff_x = polygon.vertices.at(j).x - polygon.vertices.at(i).x;
        float diff_y = polygon.vertices.at(i).y - polygon.vertices.at(j).y;
        float length = NAN;
        if (fabs(diff_x) < 0.00001F && fabs(diff_y) < 0.00001F) {
            length = 1.0F;
        } else {
            length = hypotf(diff_x, diff_y);
        }

        float bounciness = 1.0F;

        if (polygon.polygon_type == PMSPolygonType::Bouncy) {
            if (bounciness < 1.0F) {
                bounciness = 1.0F;
            } else {
                bounciness = polygon.bounciness;
            }
        } else {
            bounciness = 1.0F;
        }

        polygon.perpendiculars.at(i).x = (diff_y / length);
        polygon.perpendiculars.at(i).y = (diff_x / length);
        polygon.perpendiculars.at(i).z = 1.0F;
    }
}

void Map::FixPolygonIds()
{
    unsigned int next_id = 0;
    for (auto& polygon : map_data_.polygons) {
        polygon.id = next_id;
        ++next_id;
    }
}

void Map::UpdateMinMaxPolygonPositions(const PMSPolygon& polygon, bool should_notify)
{
    for (unsigned int i = 0; i < 3; ++i) {
        const auto& vertex = polygon.vertices.at(i);

        if (vertex.x < map_data_.polygons_min_x) {
            map_data_.polygons_min_x = vertex.x;
        }
        if (vertex.x > map_data_.polygons_max_x) {
            map_data_.polygons_max_x = vertex.x;
        }

        if (vertex.y < map_data_.polygons_min_y) {
            map_data_.polygons_min_y = vertex.y;
        }
        if (vertex.y > map_data_.polygons_max_y) {
            map_data_.polygons_max_y = vertex.y;
        }
    }

    if (should_notify) {
        map_change_events_.changed_background_color.Notify(
          map_data_.background_top_color, map_data_.background_bottom_color, GetBoundaries());
    }
}

void Map::UpdateMinMaxPolygonPositions()
{
    map_data_.polygons_min_x = 0.0F;
    map_data_.polygons_max_x = 0.0F;
    map_data_.polygons_min_y = 0.0F;
    map_data_.polygons_max_y = 0.0F;

    for (const auto& polygon : map_data_.polygons) {
        UpdateMinMaxPolygonPositions(polygon, false);
    }

    map_change_events_.changed_background_color.Notify(
      map_data_.background_top_color, map_data_.background_bottom_color, GetBoundaries());
}
} // namespace Soldank
