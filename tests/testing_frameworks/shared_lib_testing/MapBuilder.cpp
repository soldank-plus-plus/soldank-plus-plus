module;

#include "core/math/Glm.hpp"

#include <algorithm>
#include <cmath>
#include <memory>

export module Testing.Framework.Shared.MapBuilder;

import Shared.Core.Map.Map;
import Shared.Core.Map.PMSEnums;
import Shared.Core.Map.PMSStructs;
import Shared.Core.Map.PMSConstants;

export namespace SoldankTesting
{
class MapBuilder
{
public:
    static std::unique_ptr<MapBuilder> Empty()
    {
        auto map_builder_instance = std::unique_ptr<MapBuilder>(new MapBuilder());
        map_builder_instance->map_data_.description = "Test map";
        map_builder_instance->polygons_min_x_ = 0.0F;
        map_builder_instance->polygons_max_x_ = 0.0F;
        map_builder_instance->polygons_min_y_ = 0.0F;
        map_builder_instance->polygons_max_y_ = 0.0F;
        map_builder_instance->UpdateBoundaries();
        map_builder_instance->map_data_.sectors_poly = std::vector<std::vector<Soldank::PMSSector>>(
          Soldank::SECTORS_COUNT, std::vector<Soldank::PMSSector>(Soldank::SECTORS_COUNT));
        map_builder_instance->map_data_.sectors_count = 25;
        return map_builder_instance;
    }

    MapBuilder* AddPolygon(glm::vec2 vertex1,
                           glm::vec2 vertex2,
                           glm::vec2 vertex3,
                           Soldank::PMSPolygonType polygon_type)
    {
        map_data_.polygons.push_back({});
        Soldank::PMSPolygon& new_polygon = map_data_.polygons.back();
        new_polygon.vertices.at(0).x = vertex1.x;
        new_polygon.vertices.at(0).y = vertex1.y;
        new_polygon.vertices.at(1).x = vertex2.x;
        new_polygon.vertices.at(1).y = vertex2.y;
        new_polygon.vertices.at(2).x = vertex3.x;
        new_polygon.vertices.at(2).y = vertex3.y;
        new_polygon.polygon_type = polygon_type;

        if (!new_polygon.AreVerticesClockwise()) {
            std::swap(new_polygon.vertices[1], new_polygon.vertices[2]);
        }

        for (unsigned int j = 0; j < 3; ++j) {
            unsigned int k = j + 1;
            if (k > 2) {
                k = 0;
            }

            float diff_x = new_polygon.vertices.at(k).x - new_polygon.vertices.at(j).x;
            float diff_y = new_polygon.vertices.at(j).y - new_polygon.vertices.at(k).y;
            float length = NAN;
            float epsilon = 0.00001F;
            if (std::fabs(diff_x) < epsilon && std::fabs(diff_y) < epsilon) {
                length = 1.0F;
            } else {
                length = hypotf(diff_x, diff_y);
            }

            if (new_polygon.polygon_type == Soldank::PMSPolygonType::Bouncy) {
                if (new_polygon.perpendiculars.at(j).z < 1.0F) {
                    new_polygon.perpendiculars.at(j).z = 1.0F;
                }
            } else {
                new_polygon.perpendiculars.at(j).z = 1.0F;
            }

            new_polygon.perpendiculars.at(j).x =
              (diff_y / length) * new_polygon.perpendiculars.at(j).z;
            new_polygon.perpendiculars.at(j).y =
              (diff_x / length) * new_polygon.perpendiculars.at(j).z;
            new_polygon.perpendiculars.at(j).z = 1.0F;
        }

        return this;
    }
    std::unique_ptr<Soldank::Map> Build()
    {
        polygons_min_x_ = 0.0F;
        polygons_max_x_ = 0.0F;
        polygons_min_y_ = 0.0F;
        polygons_max_y_ = 0.0F;
        if (!map_data_.polygons.empty()) {
            polygons_min_x_ = map_data_.polygons.at(0).vertices.at(0).x;
            polygons_max_x_ = map_data_.polygons.at(0).vertices.at(0).x;
            polygons_min_y_ = map_data_.polygons.at(0).vertices.at(0).y;
            polygons_max_y_ = map_data_.polygons.at(0).vertices.at(0).y;
        }
        for (const auto& polygon : map_data_.polygons) {
            for (unsigned int i = 0; i < 3; ++i) {
                if (polygon.vertices.at(i).x < polygons_min_x_) {
                    polygons_min_x_ = polygon.vertices.at(i).x;
                }
                if (polygon.vertices.at(i).x > polygons_max_x_) {
                    polygons_max_x_ = polygon.vertices.at(i).x;
                }
                if (polygon.vertices.at(i).y < polygons_min_y_) {
                    polygons_min_y_ = polygon.vertices.at(i).y;
                }
                if (polygon.vertices.at(i).y > polygons_max_y_) {
                    polygons_max_y_ = polygon.vertices.at(i).y;
                }
            }
        }
        float center_x = std::floor((polygons_min_x_ + polygons_max_x_) / 2.0F);
        float center_y = std::floor((polygons_min_y_ + polygons_max_y_) / 2.0F);
        width_ = polygons_max_x_ - center_x;
        height_ = polygons_max_y_ - center_y;
        UpdateBoundaries();

        GenerateSectors();
        for (auto& polygon : map_data_.polygons) {
            for (unsigned int i = 0; i < 3; i++) {
                polygon.vertices.at(i).x -= center_x;
                polygon.vertices.at(i).y -= center_y;
            }
        }
        auto map = std::make_unique<Soldank::Map>(map_data_);
        return map;
    }

private:
    MapBuilder() = default;

    void UpdateBoundaries()
    {
        // width_ = std::fabs(polygons_max_x_ - polygons_min_x_);
        // height_ = std::fabs(polygons_max_y_ - polygons_min_y_);

        map_data_.boundaries_xy[Soldank::Map::TopBoundary] = polygons_min_y_;
        map_data_.boundaries_xy[Soldank::Map::BottomBoundary] = polygons_max_y_;
        map_data_.boundaries_xy[Soldank::Map::LeftBoundary] = polygons_min_x_;
        map_data_.boundaries_xy[Soldank::Map::RightBoundary] = polygons_max_x_;

        if (height_ > width_) {
            map_data_.boundaries_xy[Soldank::Map::LeftBoundary] -= (height_ - width_) / 2.0F;
            map_data_.boundaries_xy[Soldank::Map::RightBoundary] += (height_ - width_) / 2.0F;
        } else {
            map_data_.boundaries_xy[Soldank::Map::TopBoundary] -= (width_ - height_) / 2.0F;
            map_data_.boundaries_xy[Soldank::Map::BottomBoundary] += (width_ - height_) / 2.0F;
        }

        map_data_.boundaries_xy[Soldank::Map::TopBoundary] -= Soldank::MAP_BOUNDARY;
        map_data_.boundaries_xy[Soldank::Map::BottomBoundary] += Soldank::MAP_BOUNDARY;
        map_data_.boundaries_xy[Soldank::Map::LeftBoundary] -= Soldank::MAP_BOUNDARY;
        map_data_.boundaries_xy[Soldank::Map::RightBoundary] += Soldank::MAP_BOUNDARY;
    }
    void GenerateSectors()
    {
        float center_x = std::floor((polygons_min_x_ + polygons_max_x_) / 2.0);
        float center_y = std::floor((polygons_min_y_ + polygons_max_y_) / 2.0);

        map_data_.sectors_poly = std::vector<std::vector<Soldank::PMSSector>>(
          Soldank::SECTORS_COUNT, std::vector<Soldank::PMSSector>(Soldank::SECTORS_COUNT));

        if (width_ > height_) {
            map_data_.sectors_size =
              std::floor((width_ + 2.0 * Soldank::MAP_BOUNDARY) / (float)map_data_.sectors_count);
        } else {
            map_data_.sectors_size =
              std::floor((height_ + 2.0 * Soldank::MAP_BOUNDARY) / (float)map_data_.sectors_count);
        }

        for (int x = -25; x <= 25; ++x) {
            for (int y = -25; y <= 25; ++y) {
                map_data_.sectors_poly[x + 25][y + 25].polygons.clear();

                for (unsigned int i = 0; i < map_data_.polygons.size(); ++i) {
                    float sector_x = std::floor((float)map_data_.sectors_size * ((float)x - 0.5F) -
                                                1.0F + center_x);
                    float sector_y = std::floor((float)map_data_.sectors_size * ((float)y - 0.5F) -
                                                1.0F + center_y);

                    if (IsPolygonInSector(
                          i, sector_x, sector_y, (float)map_data_.sectors_size + 2)) {
                        map_data_.sectors_poly[x + 25][y + 25].polygons.push_back(i + 1);
                    }
                }
            }
        }
    }
    bool IsPolygonInSector(unsigned short polygon_index,
                           float sector_x,
                           float sector_y,
                           float sector_size)
    {
        if (map_data_.polygons[polygon_index].polygon_type == Soldank::PMSPolygonType::NoCollide) {
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
        for (unsigned int i = 0; i < 3; ++i) {
            if (map_data_.polygons[polygon_index].vertices.at(i).x >= sector_x &&
                map_data_.polygons[polygon_index].vertices.at(i).x <= sector_x + sector_size &&
                map_data_.polygons[polygon_index].vertices.at(i).y >= sector_y &&
                map_data_.polygons[polygon_index].vertices.at(i).y <= sector_y + sector_size) {
                return true;
            }
        }

        // Check if any of the 4 sector's corners is inside the polygon.
        if (Soldank::Map::PointInPoly({ sector_x, sector_y }, map_data_.polygons[polygon_index]) ||
            Soldank::Map::PointInPoly({ sector_x + sector_size, sector_y },
                                      map_data_.polygons[polygon_index]) ||
            Soldank::Map::PointInPoly({ sector_x + sector_size, sector_y + sector_size },
                                      map_data_.polygons[polygon_index]) ||
            Soldank::Map::PointInPoly({ sector_x, sector_y + sector_size },
                                      map_data_.polygons[polygon_index])) {
            return true;
        }

        for (unsigned int i = 0; i < 3; ++i) {
            unsigned int j = i + 1;
            if (j > 2) {
                j = 0;
            }

            glm::vec2 a1 = { map_data_.polygons[polygon_index].vertices.at(i).x,
                             map_data_.polygons[polygon_index].vertices.at(i).y };

            glm::vec2 b1 = { map_data_.polygons[polygon_index].vertices.at(j).x,
                             map_data_.polygons[polygon_index].vertices.at(j).y };

            // Top side of sector.
            glm::vec2 a2 = { sector_x, sector_y };
            glm::vec2 b2 = { sector_x + sector_size, sector_y };
            if (SegmentsIntersect(a1, b1, a2, b2)) {
                return true;
            }

            // Right side of sector.
            a2 = { sector_x + sector_size, sector_y };
            b2 = { sector_x + sector_size, sector_y + sector_size };
            if (SegmentsIntersect(a1, b1, a2, b2)) {
                return true;
            }

            // Bottom side of sector.
            a2 = { sector_x, sector_y + sector_size };
            b2 = { sector_x + sector_size, sector_y + sector_size };
            if (SegmentsIntersect(a1, b1, a2, b2)) {
                return true;
            }

            // Left side of sector.
            a2 = { sector_x, sector_y };
            b2 = { sector_x, sector_y + sector_size };
            if (SegmentsIntersect(a1, b1, a2, b2)) {
                return true;
            }
        }

        return false;
    }
    static bool SegmentsIntersect(glm::vec2 a1, glm::vec2 b1, glm::vec2 a2, glm::vec2 b2)
    {
        glm::vec2 u = b1 - a1;
        glm::vec2 v = b2 - a2;
        float d = u.x * v.y - u.y * v.x;

        if (std::abs(d) < 0.00001F) {
            return false;
        }

        glm::vec2 w = a1 - a2;

        float s = (v.x * w.y - v.y * w.x) / d;
        float t = (u.x * w.y - u.y * w.x) / d;

        return s > 0 && s < 1 && t > 0 && t < 1;
    }

    Soldank::MapData map_data_;
    float polygons_min_x_{};
    float polygons_max_x_{};
    float polygons_min_y_{};
    float polygons_max_y_{};
    float width_{};
    float height_{};
};
} // namespace SoldankTesting
