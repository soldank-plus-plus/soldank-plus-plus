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
