module;

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

module Shared.Core.Map.Map;

import Extern.Glm;

import Shared.Core.Map.PMSConstants;
import Shared.Core.Map.PMSEnums;
import Shared.Core.Map.PMSStructs;
import Shared.Core.Math.Calc;

namespace Soldank
{
void Map::GenerateSectors()
{
    if (are_sectors_generated_) {
        return;
    }

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

    are_sectors_generated_ = true;
}

glm::vec2 Map::NormalizeCoordinatesForRuntime()
{
    NormalizePolygonsAndPerpendiculars();
    FixPolygonIds();
    RecalculatePolygonBounds();
    UpdateBoundaries(false);

    const glm::vec2 offset{ -map_data_.center_x, -map_data_.center_y };
    for (auto& polygon : map_data_.polygons) {
        for (auto& vertex : polygon.vertices) {
            vertex.x += offset.x;
            vertex.y += offset.y;
        }
    }
    for (auto& scenery : map_data_.scenery_instances) {
        scenery.x += offset.x;
        scenery.y += offset.y;
    }
    for (auto& collider : map_data_.colliders) {
        collider.x += offset.x;
        collider.y += offset.y;
    }
    for (auto& spawn_point : map_data_.spawn_points) {
        spawn_point.x += static_cast<int>(offset.x);
        spawn_point.y += static_cast<int>(offset.y);
    }
    for (auto& way_point : map_data_.way_points) {
        way_point.x += static_cast<int>(offset.x);
        way_point.y += static_cast<int>(offset.y);
    }

    RecalculatePolygonBounds();
    UpdateBoundaries(false);
    are_sectors_generated_ = false;
    return offset;
}

void Map::UpdateBoundaries(bool should_notify)
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

    if (should_notify) {
        map_change_events_.changed_background_color.Notify(
          map_data_.background_top_color, map_data_.background_bottom_color, GetBoundaries());
    }
}

bool Map::IsPolygonInSector(unsigned short polygon_index,
                            float sector_x,
                            float sector_y,
                            float sector_size)
{
    const auto& polygon = map_data_.polygons.at(polygon_index);
    if (polygon.polygon_type == PMSPolygonType::NoCollide) {
        return false;
    }

    const float sector_right = sector_x + sector_size;
    const float sector_bottom = sector_y + sector_size;
    const bool left = std::ranges::all_of(
      polygon.vertices, [sector_x](const auto& vertex) { return vertex.x < sector_x; });
    const bool right = std::ranges::all_of(
      polygon.vertices, [sector_right](const auto& vertex) { return vertex.x > sector_right; });
    const bool above = std::ranges::all_of(
      polygon.vertices, [sector_y](const auto& vertex) { return vertex.y < sector_y; });
    const bool below = std::ranges::all_of(
      polygon.vertices, [sector_bottom](const auto& vertex) { return vertex.y > sector_bottom; });
    if (left || right || above || below) {
        return false;
    }

    const auto vertex_in_sector = [=](const auto& vertex) {
        return vertex.x >= sector_x && vertex.x <= sector_right && vertex.y >= sector_y &&
               vertex.y <= sector_bottom;
    };
    if (std::ranges::any_of(polygon.vertices, vertex_in_sector)) {
        return true;
    }

    const std::array sector_corners{
        glm::vec2{ sector_x, sector_y },
        glm::vec2{ sector_right, sector_y },
        glm::vec2{ sector_right, sector_bottom },
        glm::vec2{ sector_x, sector_bottom },
    };
    if (std::ranges::any_of(sector_corners, [&polygon](const auto& corner) {
            return PointInPoly(corner, polygon);
        })) {
        return true;
    }

    for (std::size_t polygon_edge = 0; polygon_edge < polygon.vertices.size(); ++polygon_edge) {
        const auto& vertex_a = polygon.vertices.at(polygon_edge);
        const auto& vertex_b = polygon.vertices.at((polygon_edge + 1) % polygon.vertices.size());
        const glm::vec2 a{ vertex_a.x, vertex_a.y };
        const glm::vec2 b{ vertex_b.x, vertex_b.y };

        for (std::size_t sector_edge = 0; sector_edge < sector_corners.size(); ++sector_edge) {
            const auto& c = sector_corners.at(sector_edge);
            const auto& d = sector_corners.at((sector_edge + 1) % sector_corners.size());
            if (Calc::SegmentsIntersect(a, b, c, d)) {
                return true;
            }
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

        polygon.perpendiculars.at(i).x = (diff_y / length);
        polygon.perpendiculars.at(i).y = (diff_x / length);
        polygon.perpendiculars.at(i).z = 1.0F;
    }
}

void Map::RefreshPolygonDerivedState(PolygonIdPolicy polygon_id_policy)
{
    NormalizePolygonsAndPerpendiculars();
    if (polygon_id_policy == PolygonIdPolicy::Rebuild) {
        FixPolygonIds();
    }
    RecalculatePolygonBounds();
    UpdateBoundaries();
    are_sectors_generated_ = false;
}

void Map::NormalizePolygonsAndPerpendiculars()
{
    for (auto& polygon : map_data_.polygons) {
        SetPolygonVerticesAndPerpendiculars(polygon);
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

void Map::RecalculatePolygonBounds()
{
    map_data_.polygons_min_x = 0.0F;
    map_data_.polygons_max_x = 0.0F;
    map_data_.polygons_min_y = 0.0F;
    map_data_.polygons_max_y = 0.0F;

    for (const auto& polygon : map_data_.polygons) {
        for (const auto& vertex : polygon.vertices) {
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
    }
}
} // namespace Soldank
