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
} // namespace Soldank
