module;

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
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

namespace
{
template<typename Index, typename Value>
void SortAndValidateInsertions(std::size_t existing_size,
                               std::vector<std::pair<Index, Value>>& insertions)
{
    std::sort(insertions.begin(), insertions.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });

    const std::size_t final_size = existing_size + insertions.size();
    for (std::size_t i = 0; i < insertions.size(); ++i) {
        const std::size_t insertion_index = static_cast<std::size_t>(insertions.at(i).first);
        if (insertion_index >= final_size) {
            throw std::out_of_range("Insertion index is outside the resulting collection");
        }
        if (i > 0 && insertion_index == static_cast<std::size_t>(insertions.at(i - 1).first)) {
            throw std::invalid_argument("Insertion indices must be unique");
        }
    }
}

template<typename Index, typename Value>
std::vector<std::size_t> InsertAtIndices(std::vector<Value>& values,
                                         std::vector<std::pair<Index, Value>> insertions)
{
    SortAndValidateInsertions(values.size(), insertions);

    std::vector<Value> result;
    result.reserve(values.size() + insertions.size());
    std::vector<std::size_t> inserted_indices;
    inserted_indices.reserve(insertions.size());
    std::size_t old_value_index = 0;
    std::size_t insertion_index = 0;

    while (result.size() < values.size() + insertions.size()) {
        if (insertion_index < insertions.size() &&
            static_cast<std::size_t>(insertions.at(insertion_index).first) == result.size()) {
            inserted_indices.push_back(result.size());
            result.push_back(std::move(insertions.at(insertion_index).second));
            ++insertion_index;
        } else {
            result.push_back(std::move(values.at(old_value_index)));
            ++old_value_index;
        }
    }

    values = std::move(result);
    return inserted_indices;
}

template<typename Index, typename Value>
std::vector<Value> RemoveAtIndices(std::vector<Value>& values, std::vector<Index> removal_indices)
{
    std::sort(removal_indices.begin(), removal_indices.end());
    for (std::size_t i = 0; i < removal_indices.size(); ++i) {
        const std::size_t removal_index = static_cast<std::size_t>(removal_indices.at(i));
        if (removal_index >= values.size()) {
            throw std::out_of_range("Removal index is outside the collection");
        }
        if (i > 0 && removal_index == static_cast<std::size_t>(removal_indices.at(i - 1))) {
            throw std::invalid_argument("Removal indices must be unique");
        }
    }

    std::vector<Value> remaining_values;
    remaining_values.reserve(values.size() - removal_indices.size());
    std::vector<Value> removed_values;
    removed_values.reserve(removal_indices.size());
    std::size_t removal_index = 0;

    for (std::size_t value_index = 0; value_index < values.size(); ++value_index) {
        if (removal_index < removal_indices.size() &&
            value_index == static_cast<std::size_t>(removal_indices.at(removal_index))) {
            removed_values.push_back(std::move(values.at(value_index)));
            ++removal_index;
        } else {
            remaining_values.push_back(std::move(values.at(value_index)));
        }
    }

    values = std::move(remaining_values);
    return removed_values;
}
} // namespace

namespace Soldank
{
PMSPolygon Map::AddNewPolygon(const PMSPolygon& polygon)
{

    PMSPolygon new_polygon = polygon;

    new_polygon.id = map_data_.polygons.size();

    map_data_.polygons.push_back(new_polygon);
    RefreshPolygonDerivedState(PolygonIdPolicy::Rebuild);
    new_polygon = map_data_.polygons.back();

    map_change_events_.added_new_polygon.Notify(new_polygon);

    return new_polygon;
}

void Map::ReplaceContents(const Map& source_map)
{
    map_data_ = source_map.map_data_;
    are_sectors_generated_ = source_map.are_sectors_generated_;

    map_change_events_.changed_background_color.Notify(
      map_data_.background_top_color, map_data_.background_bottom_color, GetBoundaries());
    map_change_events_.changed_texture_name.Notify(map_data_.texture_name);
    map_change_events_.modified_polygons.Notify(map_data_.polygons);
    map_change_events_.modified_sceneries.Notify(map_data_.scenery_instances);
    map_change_events_.modified_spawn_points.Notify(map_data_.spawn_points);
}

void Map::AddPolygons(const std::vector<PMSPolygon>& polygons)
{
    std::vector<std::pair<unsigned int, PMSPolygon>> polygon_insertions;
    polygon_insertions.reserve(polygons.size());
    for (const auto& polygon : polygons) {
        polygon_insertions.emplace_back(polygon.id, polygon);
    }
    const auto inserted_indices =
      InsertAtIndices(map_data_.polygons, std::move(polygon_insertions));

    RefreshPolygonDerivedState(PolygonIdPolicy::Rebuild);
    std::vector<PMSPolygon> polygons_to_add;
    polygons_to_add.reserve(inserted_indices.size());
    for (const auto inserted_index : inserted_indices) {
        polygons_to_add.push_back(map_data_.polygons.at(inserted_index));
    }

    map_change_events_.added_new_polygons.Notify(polygons_to_add, map_data_.polygons);
}

PMSPolygon Map::RemovePolygonById(unsigned int id)
{
    PMSPolygon removed_polygon = map_data_.polygons.at(id);
    map_data_.polygons.erase(map_data_.polygons.begin() + id);
    RefreshPolygonDerivedState(PolygonIdPolicy::Rebuild);

    map_change_events_.removed_polygon.Notify(removed_polygon, map_data_.polygons);

    return removed_polygon;
}

void Map::RemovePolygonsById(const std::vector<unsigned int>& polygon_ids)
{
    auto removed_polygons = RemoveAtIndices(map_data_.polygons, polygon_ids);

    RefreshPolygonDerivedState(PolygonIdPolicy::Rebuild);

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

    RefreshPolygonDerivedState(PolygonIdPolicy::Preserve);

    map_change_events_.modified_polygons.Notify(map_data_.polygons);
}

void Map::SetPolygonsById(const std::vector<std::pair<unsigned int, PMSPolygon>>& polygons)
{
    for (const auto& [polygon_id, polygon] : polygons) {
        map_data_.polygons.at(polygon_id) = polygon;
    }

    RefreshPolygonDerivedState(PolygonIdPolicy::Rebuild);

    map_change_events_.modified_polygons.Notify(map_data_.polygons);
}

void Map::ReplacePolygons(std::vector<PMSPolygon> polygons)
{
    map_data_.polygons = std::move(polygons);
    RefreshPolygonDerivedState(PolygonIdPolicy::Rebuild);
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

void Map::AddSpawnPoints(const std::vector<std::pair<unsigned int, PMSSpawnPoint>>& spawn_points)
{
    InsertAtIndices(map_data_.spawn_points, spawn_points);

    map_change_events_.added_spawn_points.Notify(map_data_.spawn_points);
}

void Map::RemoveSpawnPointsById(const std::vector<unsigned int>& spawn_point_ids)
{
    RemoveAtIndices(map_data_.spawn_points, spawn_point_ids);

    map_change_events_.removed_spawn_points.Notify(map_data_.spawn_points);
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

void Map::ReplaceSpawnPoints(std::vector<PMSSpawnPoint> spawn_points)
{
    map_data_.spawn_points = std::move(spawn_points);
    map_change_events_.modified_spawn_points.Notify(map_data_.spawn_points);
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
    SortAndValidateInsertions(map_data_.scenery_instances.size(), sceneries_to_add);

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
            map_data_.scenery_types.push_back({ .name = file_name, .timestamp = {} });
            scenery_to_add.second.first.style = map_data_.scenery_types.size();
            map_change_events_.added_new_scenery_type.Notify(map_data_.scenery_types.back());
        }
    }

    std::vector<std::pair<unsigned int, PMSScenery>> scenery_insertions;
    scenery_insertions.reserve(sceneries_to_add.size());
    for (auto& scenery_to_add : sceneries_to_add) {
        scenery_insertions.emplace_back(scenery_to_add.first,
                                        std::move(scenery_to_add.second.first));
    }
    InsertAtIndices(map_data_.scenery_instances, std::move(scenery_insertions));

    map_change_events_.added_sceneries.Notify(map_data_.scenery_instances);
}

void Map::RemoveSceneriesById(const std::vector<unsigned int>& scenery_ids)
{
    RemoveAtIndices(map_data_.scenery_instances, scenery_ids);
    std::array<unsigned int, MAX_SCENERIES_COUNT + 10> scenery_type_usage_count{};
    scenery_type_usage_count.fill(0);

    for (const auto& scenery : map_data_.scenery_instances) {
        ++scenery_type_usage_count.at(scenery.style);
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

void Map::ReplaceSceneries(std::vector<PMSScenery> sceneries)
{
    map_data_.scenery_instances = std::move(sceneries);
    map_change_events_.modified_sceneries.Notify(map_data_.scenery_instances);
}
} // namespace Soldank
