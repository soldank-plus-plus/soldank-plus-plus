module;

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <sstream>
#include <string>
#include <type_traits>
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

namespace
{
constexpr std::size_t MAX_SERIALIZED_COLLECTION_COUNT = 100000;

template<typename DataType>
void ReadValue(std::stringstream& buffer, DataType& value)
{
    static_assert(std::is_trivially_copyable_v<DataType>);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    if (!buffer.read(reinterpret_cast<char*>(&value), sizeof(DataType))) {
        throw std::runtime_error("Unexpected end of PMS map data");
    }
}

void ReadFixedString(std::stringstream& buffer, std::string& value, std::size_t max_size)
{
    unsigned char string_size = 0;
    ReadValue(buffer, string_size);
    if (string_size > max_size) {
        throw std::runtime_error("PMS string length exceeds its fixed-width field");
    }

    std::vector<char> bytes(max_size);
    if (!buffer.read(bytes.data(), static_cast<std::streamsize>(bytes.size()))) {
        throw std::runtime_error("Unexpected end of PMS string data");
    }
    value.assign(bytes.data(), string_size);
}

template<typename DataType>
void AppendValue(std::shared_ptr<Soldank::IFileWriter>& file_writer, const DataType& data)
{
    static_assert(std::is_trivially_copyable_v<DataType>);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    if (file_writer->AppendData(reinterpret_cast<const char*>(&data), sizeof(DataType)) !=
        Soldank::FileWriterError::NoError) {
        throw std::runtime_error("Could not append PMS map data");
    }
}

void AppendFixedString(std::shared_ptr<Soldank::IFileWriter>& file_writer,
                       const std::string& data,
                       std::size_t max_size)
{
    if (data.size() > max_size || data.size() > std::numeric_limits<unsigned char>::max()) {
        throw std::length_error("String does not fit in its PMS fixed-width field");
    }

    const auto string_size = static_cast<unsigned char>(data.size());
    AppendValue(file_writer, string_size);
    if (!data.empty() &&
        file_writer->AppendData(data.data(), static_cast<std::streamsize>(data.size())) !=
          Soldank::FileWriterError::NoError) {
        throw std::runtime_error("Could not append PMS string data");
    }
    std::vector<char> filler(max_size - data.size());
    if (!filler.empty() &&
        file_writer->AppendData(filler.data(), static_cast<std::streamsize>(filler.size())) !=
          Soldank::FileWriterError::NoError) {
        throw std::runtime_error("Could not append PMS string padding");
    }
}

template<typename DataType>
void ReadCollection(std::stringstream& buffer,
                    std::vector<DataType>& values,
                    std::size_t max_count = MAX_SERIALIZED_COLLECTION_COUNT)
{
    std::uint32_t count = 0;
    ReadValue(buffer, count);
    if (count > max_count) {
        throw std::runtime_error("PMS collection count exceeds the supported limit");
    }

    std::vector<DataType> loaded_values(count);
    for (auto& value : loaded_values) {
        ReadValue(buffer, value);
    }
    values = std::move(loaded_values);
}

template<typename DataType, typename Transform = std::identity>
void AppendCollection(std::shared_ptr<Soldank::IFileWriter>& file_writer,
                      const std::vector<DataType>& values,
                      Transform transform = {})
{
    if (values.size() > std::numeric_limits<std::uint32_t>::max()) {
        throw std::length_error("PMS collection is too large to serialize");
    }

    AppendValue(file_writer, static_cast<std::uint32_t>(values.size()));
    for (const auto& value : values) {
        AppendValue(file_writer, std::invoke(transform, value));
    }
}
} // namespace

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

    UpdateBoundaries(false);
    GenerateSectors();
}

void Map::LoadMap(const std::filesystem::path& map_path, const IFileReader& file_reader)
{
    auto file_data = file_reader.Read(map_path.string(), std::ios::in | std::ios::binary);
    if (!file_data.has_value()) {
        spdlog::critical("Map not found {}", map_path.string());
        // TODO: should return an error
        return;
    }
    std::stringstream data_buffer{ *file_data };
    Map loaded_map;
    loaded_map.map_data_.name = map_path.filename().string();

    ReadValue(data_buffer, loaded_map.map_data_.version);
    ReadFixedString(data_buffer, loaded_map.map_data_.description, DESCRIPTION_MAX_LENGTH);
    ReadFixedString(data_buffer, loaded_map.map_data_.texture_name, TEXTURE_NAME_MAX_LENGTH);

    ReadValue(data_buffer, loaded_map.map_data_.background_top_color);
    ReadValue(data_buffer, loaded_map.map_data_.background_bottom_color);
    ReadValue(data_buffer, loaded_map.map_data_.jet_count);
    ReadValue(data_buffer, loaded_map.map_data_.grenades_count);
    ReadValue(data_buffer, loaded_map.map_data_.medikits_count);
    ReadValue(data_buffer, loaded_map.map_data_.weather_type);
    ReadValue(data_buffer, loaded_map.map_data_.step_type);
    ReadValue(data_buffer, loaded_map.map_data_.random_id);

    loaded_map.ReadPolygonsFromBuffer(data_buffer);
    loaded_map.ReadSectorsFromBuffer(data_buffer);
    loaded_map.ReadSceneryInstancesFromBuffer(data_buffer);
    loaded_map.ReadSceneryTypesFromBuffer(data_buffer);
    loaded_map.ReadCollidersFromBuffer(data_buffer);
    loaded_map.ReadSpawnPointsFromBuffer(data_buffer);
    loaded_map.ReadWayPointsFromBuffer(data_buffer);
    loaded_map.UpdateBoundaries(false);
    loaded_map.are_sectors_generated_ = true;

    map_data_ = std::move(loaded_map.map_data_);
    are_sectors_generated_ = true;
    map_change_events_.changed_background_color.Notify(
      map_data_.background_top_color, map_data_.background_bottom_color, GetBoundaries());
}

void Map::SaveMap(const std::filesystem::path& map_path,
                  std::shared_ptr<IFileWriter> file_writer) const
{
    Map serializable_map = *this;
    serializable_map.NormalizeCoordinatesForRuntime();
    serializable_map.GenerateSectors();

    AppendValue(file_writer, serializable_map.map_data_.version);
    AppendFixedString(file_writer, serializable_map.map_data_.description, DESCRIPTION_MAX_LENGTH);
    AppendFixedString(
      file_writer, serializable_map.map_data_.texture_name, TEXTURE_NAME_MAX_LENGTH);

    AppendValue(file_writer, serializable_map.map_data_.background_top_color);
    AppendValue(file_writer, serializable_map.map_data_.background_bottom_color);
    AppendValue(file_writer, serializable_map.map_data_.jet_count);
    AppendValue(file_writer, serializable_map.map_data_.grenades_count);
    AppendValue(file_writer, serializable_map.map_data_.medikits_count);
    AppendValue(file_writer, serializable_map.map_data_.weather_type);
    AppendValue(file_writer, serializable_map.map_data_.step_type);
    AppendValue(file_writer, serializable_map.map_data_.random_id);

    serializable_map.AppendPolygonsToFileWriter(file_writer);
    serializable_map.AppendSectorsToFileWriter(file_writer);
    serializable_map.AppendSceneryInstancesToFileWriter(file_writer);
    serializable_map.AppendSceneryTypesToFileWriter(file_writer);
    serializable_map.AppendCollidersToFileWriter(file_writer);
    serializable_map.AppendSpawnPointsToFileWriter(file_writer);
    serializable_map.AppendWayPointsToFileWriter(file_writer);

    int zero = 0;
    AppendValue(file_writer, zero);
    AppendValue(file_writer, zero);

    auto error = file_writer->Write(map_path, std::ios::out | std::ios::binary | std::ios::trunc);
    if (error != FileWriterError::NoError) {
        spdlog::critical("Could not save map: {}", map_path.string());
        return;
    }
}

void Map::ReadPolygonsFromBuffer(std::stringstream& buffer)
{
    std::uint32_t polygons_count = 0;
    ReadValue(buffer, polygons_count);
    if (polygons_count > MAX_POLYGONS_COUNT) {
        throw std::runtime_error("PMS polygon count exceeds the supported limit");
    }
    map_data_.polygons.clear();
    map_data_.polygons.reserve(polygons_count);
    for (std::uint32_t i = 0; i < polygons_count; ++i) {
        PMSPolygon new_polygon;
        new_polygon.id = i;

        for (unsigned int j = 0; j < 3; ++j) {
            ReadValue(buffer, new_polygon.vertices.at(j));

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
        for (unsigned int j = 0; j < 3; ++j) {
            ReadValue(buffer, new_polygon.perpendiculars.at(j));
        }
        new_polygon.bounciness =
          glm::length(glm::vec2(new_polygon.perpendiculars[2].x, new_polygon.perpendiculars[2].y));

        for (unsigned int j = 0; j < 3; j++) {
            glm::vec2 normalized_perpendiculars = Calc::Vec2Normalize(
              glm::vec2(new_polygon.perpendiculars.at(j).x, new_polygon.perpendiculars.at(j).y));
            new_polygon.perpendiculars.at(j).x = normalized_perpendiculars.x;
            new_polygon.perpendiculars.at(j).y = normalized_perpendiculars.y;
        }

        ReadValue(buffer, new_polygon.polygon_type);

        map_data_.polygons.push_back(new_polygon);
    }
}

void Map::ReadSectorsFromBuffer(std::stringstream& buffer)
{
    ReadValue(buffer, map_data_.sectors_size);
    ReadValue(buffer, map_data_.sectors_count);
    if (map_data_.sectors_size <= 0 || map_data_.sectors_count < 0 ||
        map_data_.sectors_count > (SECTORS_COUNT - 1) / 2) {
        throw std::runtime_error("Invalid PMS sector dimensions");
    }

    int n = 2 * map_data_.sectors_count + 1;
    map_data_.sectors_poly = std::vector<std::vector<PMSSector>>(n, std::vector<PMSSector>(n));

    for (auto& sec_i : map_data_.sectors_poly) {
        for (auto& sec_ij : sec_i) {
            unsigned short sector_polygons_count = 0;
            ReadValue(buffer, sector_polygons_count);
            if (sector_polygons_count > map_data_.polygons.size()) {
                throw std::runtime_error("PMS sector contains too many polygon references");
            }

            sec_ij.polygons.resize(sector_polygons_count);
            for (auto& polygon_id : sec_ij.polygons) {
                ReadValue(buffer, polygon_id);
                if (polygon_id == 0 || polygon_id > map_data_.polygons.size()) {
                    throw std::runtime_error("PMS sector references an invalid polygon");
                }
            }
        }
    }
}

void Map::ReadSceneryInstancesFromBuffer(std::stringstream& buffer)
{
    ReadCollection(buffer, map_data_.scenery_instances, MAX_SCENERIES_COUNT);
}

void Map::ReadSceneryTypesFromBuffer(std::stringstream& buffer)
{
    std::uint32_t scenery_types_count = 0;
    ReadValue(buffer, scenery_types_count);
    if (scenery_types_count > MAX_SCENERIES_COUNT) {
        throw std::runtime_error("PMS scenery type count exceeds the supported limit");
    }
    map_data_.scenery_types.clear();
    map_data_.scenery_types.reserve(scenery_types_count);
    for (std::uint32_t i = 0; i < scenery_types_count; ++i) {
        map_data_.scenery_types.push_back({});
        ReadFixedString(buffer, map_data_.scenery_types.back().name, SCENERY_NAME_MAX_LENGTH);
        ReadValue(buffer, map_data_.scenery_types.back().timestamp);
    }
}

void Map::ReadCollidersFromBuffer(std::stringstream& buffer)
{
    ReadCollection(buffer, map_data_.colliders);
}

void Map::ReadSpawnPointsFromBuffer(std::stringstream& buffer)
{
    ReadCollection(buffer, map_data_.spawn_points, MAX_SPAWN_POINTS_COUNT);
}

void Map::ReadWayPointsFromBuffer(std::stringstream& buffer)
{
    ReadCollection(buffer, map_data_.way_points);
}

void Map::AppendPolygonsToFileWriter(std::shared_ptr<IFileWriter>& file_writer) const
{
    if (map_data_.polygons.size() > MAX_POLYGONS_COUNT) {
        throw std::length_error("Too many polygons to serialize");
    }
    AppendValue(file_writer, static_cast<std::uint32_t>(map_data_.polygons.size()));

    for (const auto& source_polygon : map_data_.polygons) {
        auto polygon = source_polygon;
        // Polygons' vertices have to be arranged in clock-wise order.
        if (!polygon.AreVerticesClockwise()) {
            std::swap(polygon.vertices[1], polygon.vertices[2]);
        }

        for (const auto& vertex : polygon.vertices) {
            auto vertex_copy = vertex;
            vertex_copy.x -= map_data_.center_x;
            vertex_copy.y -= map_data_.center_y;
            vertex_copy.z = 1.0F;

            AppendValue(file_writer, vertex_copy);
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

            AppendValue(file_writer, polygon.perpendiculars.at(i));
        }
        AppendValue(file_writer, polygon.polygon_type);
    }
}

void Map::AppendSectorsToFileWriter(std::shared_ptr<IFileWriter>& file_writer) const
{
    AppendValue(file_writer, map_data_.sectors_size);
    /**
     * In VB6/Pascal, an array can have negative indexes. Basically, Soldat creates an
     * array like this: [-sectorsCount...sectorsCount, -sectorsCount...sectorsCount].
     * This is a 2-dimensional array equal to C++'s [sectorsCount * 2 + 1][sectorsCount * 2 +
     * 1].
     */
    AppendValue(file_writer, map_data_.sectors_count);
    for (int x = 0; x <= map_data_.sectors_count * 2; ++x) {
        for (int y = 0; y <= map_data_.sectors_count * 2; ++y) {
            if (map_data_.sectors_poly[x][y].polygons.size() >
                std::numeric_limits<unsigned short>::max()) {
                throw std::length_error("Too many polygon references in PMS sector");
            }
            const auto sector_polygons_count =
              static_cast<unsigned short>(map_data_.sectors_poly[x][y].polygons.size());
            AppendValue(file_writer, sector_polygons_count);
            for (const auto& polygon_id : map_data_.sectors_poly[x][y].polygons) {
                AppendValue(file_writer, polygon_id);
            }
        }
    }
}

void Map::AppendSceneryInstancesToFileWriter(std::shared_ptr<IFileWriter>& file_writer) const
{
    if (map_data_.scenery_instances.size() > MAX_SCENERIES_COUNT) {
        throw std::length_error("Too many scenery instances to serialize");
    }
    AppendCollection(file_writer, map_data_.scenery_instances, [this](const auto& scenery) {
        auto scenery_copy = scenery;
        scenery_copy.x -= map_data_.center_x;
        scenery_copy.y -= map_data_.center_y;
        return scenery_copy;
    });
}

void Map::AppendSceneryTypesToFileWriter(std::shared_ptr<IFileWriter>& file_writer) const
{
    if (map_data_.scenery_types.size() > MAX_SCENERIES_COUNT) {
        throw std::length_error("Too many scenery types to serialize");
    }
    AppendValue(file_writer, static_cast<std::uint32_t>(map_data_.scenery_types.size()));
    for (const auto& scenery_type : map_data_.scenery_types) {
        AppendFixedString(file_writer, scenery_type.name, SCENERY_NAME_MAX_LENGTH);
        AppendValue(file_writer, scenery_type.timestamp);
    }
}

void Map::AppendCollidersToFileWriter(std::shared_ptr<IFileWriter>& file_writer) const
{
    AppendCollection(file_writer, map_data_.colliders, [this](const auto& collider) {
        auto collider_copy = collider;
        collider_copy.x -= map_data_.center_x;
        collider_copy.y -= map_data_.center_y;
        return collider_copy;
    });
}

void Map::AppendSpawnPointsToFileWriter(std::shared_ptr<IFileWriter>& file_writer) const
{
    if (map_data_.spawn_points.size() > MAX_SPAWN_POINTS_COUNT) {
        throw std::length_error("Too many spawn points to serialize");
    }
    AppendCollection(file_writer, map_data_.spawn_points, [this](const auto& spawn_point) {
        auto spawn_point_copy = spawn_point;
        spawn_point_copy.x -= static_cast<int>(map_data_.center_x);
        spawn_point_copy.y -= static_cast<int>(map_data_.center_y);
        return spawn_point_copy;
    });
}

void Map::AppendWayPointsToFileWriter(std::shared_ptr<IFileWriter>& file_writer) const
{
    AppendCollection(file_writer, map_data_.way_points, [this](const auto& way_point) {
        auto way_point_copy = way_point;
        way_point_copy.x -= static_cast<int>(map_data_.center_x);
        way_point_copy.y -= static_cast<int>(map_data_.center_y);
        return way_point_copy;
    });
}
} // namespace Soldank
