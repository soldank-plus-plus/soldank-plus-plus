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
void Map::CreateEmptyMap()
{
    map_data_ = MapData{};
    are_sectors_generated_ = false;

    map_data_.boundaries_xy[TopBoundary] = -MAP_BOUNDARY;
    map_data_.boundaries_xy[BottomBoundary] = MAP_BOUNDARY;
    map_data_.boundaries_xy[LeftBoundary] = -MAP_BOUNDARY;
    map_data_.boundaries_xy[RightBoundary] = MAP_BOUNDARY;

    map_data_.name = std::nullopt;
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

template<typename DataType>
void Map::ReadFromBuffer(std::stringstream& buffer, DataType& variable_to_save_data)
{
    buffer.read((char*)&variable_to_save_data, sizeof(DataType));
}

void Map::ReadStringFromBuffer(std::stringstream& buffer,
                               std::string& string_to_save_data,
                               unsigned int max_string_size)
{
    unsigned char string_size = 0;
    ReadFromBuffer(buffer, string_size);
    // We need an array with dynamic size here
    // NOLINTNEXTLINE(modernize-avoid-c-arrays,cppcoreguidelines-avoid-c-arrays)
    auto bytes = std::make_unique<char[]>(string_size);
    buffer.read(bytes.get(), string_size);
    string_to_save_data.assign(bytes.get(), string_size);
    // We need an array with dynamic size here
    // NOLINTNEXTLINE(modernize-avoid-c-arrays,cppcoreguidelines-avoid-c-arrays)
    auto filler = std::make_unique<char[]>(max_string_size - string_size);
    buffer.read(filler.get(), max_string_size - string_size);
}

template<typename DataType>
void Map::AppendToFileWriter(std::shared_ptr<IFileWriter>& file_writer, const DataType& data)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    file_writer->AppendData(reinterpret_cast<const char*>(&data), sizeof(data));
}

void Map::AppendStringToFileWriter(std::shared_ptr<IFileWriter>& file_writer,
                                   const std::string& data,
                                   unsigned int max_string_size)
{
    unsigned char string_size = data.length();
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    file_writer->AppendData(reinterpret_cast<char*>(&string_size), sizeof(string_size));
    unsigned long long data_size = data.length() * sizeof(char);
    file_writer->AppendData(data.c_str(), (std::streamsize)data_size);
    std::array<char, 64> filler{};
    filler.fill(0);
    data_size = (max_string_size - data.length()) * sizeof(char);
    file_writer->AppendData(filler.data(), (std::streamsize)data_size);
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
    map_data_.way_points.clear();
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
     * This is a 2-dimensional array equal to C++'s [sectorsCount * 2 + 1][sectorsCount * 2 +
     * 1].
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
} // namespace Soldank
