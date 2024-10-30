#include "map_editor/actions/RotateSelectionMapEditorAction.hpp"

#include "core/map/PMSStructs.hpp"

#include "core/math/Calc.hpp"

#include <cmath>
#include <spdlog/spdlog.h>

namespace Soldank
{
RotateSelectionMapEditorAction::RotateSelectionMapEditorAction(
  const std::vector<std::pair<std::pair<unsigned int, std::bitset<3>>, PMSPolygon>>&
    original_polygon_vertices,
  const std::vector<std::pair<unsigned int, PMSScenery>>& original_sceneries,
  const std::vector<std::pair<unsigned int, PMSSpawnPoint>>& original_spawn_points,
  const glm::vec2& origin,
  const glm::vec2& reference_position)
    : original_polygon_vertices_(original_polygon_vertices)
    , original_sceneries_(original_sceneries)
    , original_spawn_points_(original_spawn_points)
    , origin_(origin)
    , reference_position_(reference_position)
    , current_mouse_position_(reference_position)
{
    for (const auto& [polygon_vertices, polygon] : original_polygon_vertices) {
        original_polygons_.emplace_back(polygon_vertices.first, polygon);
    }
}

void RotateSelectionMapEditorAction::Execute(ClientState& /*client_state*/, Map& map)
{
    float reference_rotation = Calc::GetAngle(origin_, reference_position_);
    float new_rotation = Calc::GetAngle(origin_, current_mouse_position_);
    float rotation_factor = new_rotation - reference_rotation;

    std::vector<std::pair<unsigned int, PMSPolygon>> new_polygons;
    new_polygons.reserve(original_polygon_vertices_.size());
    for (const auto& [polygon_vertices, polygon] : original_polygon_vertices_) {
        PMSPolygon new_polygon = polygon;
        for (unsigned int i = 0; i < 3; ++i) {
            if (!polygon_vertices.second[i]) {
                continue;
            }

            glm::vec2 new_position =
              Calc::RotatePoint({ new_polygon.vertices.at(i).x, new_polygon.vertices.at(i).y },
                                origin_,
                                rotation_factor);
            new_polygon.vertices.at(i).x = new_position.x;
            new_polygon.vertices.at(i).y = new_position.y;
        }
        new_polygons.emplace_back(polygon_vertices.first, new_polygon);
    }
    map.SetPolygonsById(new_polygons);

    std::vector<std::pair<unsigned int, PMSScenery>> new_sceneries;
    new_sceneries.reserve(original_sceneries_.size());
    for (const auto& [scenery_id, scenery] : original_sceneries_) {
        PMSScenery new_scenery = scenery;
        glm::vec2 new_position =
          Calc::RotatePoint({ new_scenery.x, new_scenery.y }, origin_, rotation_factor);
        new_scenery.x = new_position.x;
        new_scenery.y = new_position.y;

        auto corners = Map::GetSceneryVertexPositions(scenery);
        glm::vec2 top_right_corner_position = corners.at(2);

        top_right_corner_position =
          Calc::RotatePoint(top_right_corner_position, origin_, rotation_factor);

        new_scenery.rotation = -Calc::GetAngle(top_right_corner_position, new_position);

        new_sceneries.emplace_back(scenery_id, new_scenery);
    }
    map.SetSceneriesById(new_sceneries);

    std::vector<std::pair<unsigned int, PMSSpawnPoint>> new_spawn_points;
    new_spawn_points.reserve(original_spawn_points_.size());
    for (const auto& [spawn_point_id, spawn_point] : original_spawn_points_) {
        PMSSpawnPoint new_spawn_point = spawn_point;
        glm::ivec2 new_position =
          Calc::RotatePoint({ new_spawn_point.x, new_spawn_point.y }, origin_, rotation_factor);
        new_spawn_point.x = new_position.x;
        new_spawn_point.y = new_position.y;
        new_spawn_points.emplace_back(spawn_point_id, new_spawn_point);
    }
    map.SetSpawnPointsById(new_spawn_points);
}

void RotateSelectionMapEditorAction::Undo(ClientState& /*client_state*/, Map& map)
{
    map.SetPolygonsById(original_polygons_);
    map.SetSceneriesById(original_sceneries_);
    map.SetSpawnPointsById(original_spawn_points_);
}
} // namespace Soldank
