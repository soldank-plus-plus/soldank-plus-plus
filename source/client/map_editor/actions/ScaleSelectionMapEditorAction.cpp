#include "map_editor/actions/ScaleSelectionMapEditorAction.hpp"

#include "core/map/PMSStructs.hpp"

#include "core/math/Calc.hpp"
#include "core/physics/SoldierSkeletonPhysics.hpp"

#include <cmath>
#include <spdlog/spdlog.h>

namespace Soldank
{
ScaleSelectionMapEditorAction::ScaleSelectionMapEditorAction(
  const std::vector<std::pair<std::pair<unsigned int, std::bitset<3>>, PMSPolygon>>&
    original_polygon_vertices,
  const std::vector<std::pair<unsigned int, PMSScenery>>& original_sceneries,
  const std::vector<std::pair<unsigned int, PMSSpawnPoint>>& original_spawn_points,
  const std::vector<std::pair<unsigned int, glm::vec2>>& original_soldier_positions,
  const glm::vec2& origin,
  const glm::vec2& reference_position,
  bool is_scale_horizontal,
  bool is_scale_vertical)
    : original_polygon_vertices_(original_polygon_vertices)
    , original_sceneries_(original_sceneries)
    , original_spawn_points_(original_spawn_points)
    , original_soldier_positions_(original_soldier_positions)
    , origin_(origin)
    , reference_position_(reference_position)
    , current_mouse_position_(reference_position)
    , is_scale_horizontal_(is_scale_horizontal)
    , is_scale_vertical_(is_scale_vertical)
{
    for (const auto& [polygon_vertices, polygon] : original_polygon_vertices) {
        original_polygons_.emplace_back(polygon_vertices.first, polygon);
    }
}

bool ScaleSelectionMapEditorAction::CanExecute(const ClientState& /*client_state*/,
                                               const State& /*game_state*/)
{
    return true;
}

void ScaleSelectionMapEditorAction::Execute(ClientState& /*client_state*/, State& game_state)
{
    glm::vec2 scale_factor;

    glm::vec2 denominator = reference_position_ - origin_;
    if (std::abs(denominator.x) < 0.0001F) {
        denominator.x = 1.0F;
    }
    if (std::abs(denominator.y) < 0.0001F) {
        denominator.y = 1.0F;
    }

    glm::vec2 numerator = current_mouse_position_ - origin_;
    if (std::abs(numerator.x) < 0.0001F) {
        numerator.x = 1.0F;
    }
    if (std::abs(numerator.y) < 0.0001F) {
        numerator.y = 1.0F;
    }

    scale_factor = numerator / denominator;

    if (!is_scale_horizontal_) {
        scale_factor.x = 1.0F;
    }
    if (!is_scale_vertical_) {
        scale_factor.y = 1.0F;
    }

    std::vector<std::pair<unsigned int, PMSPolygon>> new_polygons;
    new_polygons.reserve(original_polygon_vertices_.size());
    for (const auto& [polygon_vertices, polygon] : original_polygon_vertices_) {
        PMSPolygon new_polygon = polygon;
        for (unsigned int i = 0; i < 3; ++i) {
            if (!polygon_vertices.second[i]) {
                continue;
            }

            glm::vec2 new_position =
              Calc::ScalePoint({ new_polygon.vertices.at(i).x, new_polygon.vertices.at(i).y },
                               origin_,
                               scale_factor);
            new_polygon.vertices.at(i).x = new_position.x;
            new_polygon.vertices.at(i).y = new_position.y;
        }
        new_polygons.emplace_back(polygon_vertices.first, new_polygon);
    }
    game_state.map.SetPolygonsById(new_polygons);

    std::vector<std::pair<unsigned int, PMSScenery>> new_sceneries;
    new_sceneries.reserve(original_sceneries_.size());
    for (const auto& [scenery_id, scenery] : original_sceneries_) {
        PMSScenery new_scenery = scenery;
        glm::vec2 new_position =
          Calc::ScalePoint({ new_scenery.x, new_scenery.y }, origin_, scale_factor);
        new_scenery.x = new_position.x;
        new_scenery.y = new_position.y;

        glm::vec2 top_right_corner_position;
        top_right_corner_position.x = scenery.x + (float)scenery.width * scenery.scale_x;
        top_right_corner_position.y = scenery.y;
        top_right_corner_position =
          Calc::RotatePoint(top_right_corner_position, { scenery.x, scenery.y }, scenery.rotation);
        top_right_corner_position =
          Calc::ScalePoint(top_right_corner_position, origin_, scale_factor);

        new_scenery.rotation = Calc::GetAngle(top_right_corner_position, new_position);

        top_right_corner_position =
          Calc::RotatePoint(top_right_corner_position, new_position, -new_scenery.rotation);
        new_scenery.scale_x =
          (top_right_corner_position.x - new_scenery.x) / (float)new_scenery.width;

        glm::vec2 bottom_right_corner_position;
        bottom_right_corner_position.x = scenery.x + (float)scenery.width * scenery.scale_x;
        bottom_right_corner_position.y = scenery.y + (float)scenery.height * scenery.scale_y;
        bottom_right_corner_position = Calc::RotatePoint(
          bottom_right_corner_position, { scenery.x, scenery.y }, scenery.rotation);
        bottom_right_corner_position =
          Calc::ScalePoint(bottom_right_corner_position, origin_, scale_factor);
        bottom_right_corner_position =
          Calc::RotatePoint(bottom_right_corner_position, new_position, -new_scenery.rotation);
        new_scenery.scale_y =
          (bottom_right_corner_position.y - new_scenery.y) / (float)new_scenery.height;

        new_sceneries.emplace_back(scenery_id, new_scenery);
    }
    game_state.map.SetSceneriesById(new_sceneries);

    std::vector<std::pair<unsigned int, PMSSpawnPoint>> new_spawn_points;
    new_spawn_points.reserve(original_spawn_points_.size());
    for (const auto& [spawn_point_id, spawn_point] : original_spawn_points_) {
        PMSSpawnPoint new_spawn_point = spawn_point;
        glm::ivec2 new_position =
          Calc::ScalePoint({ new_spawn_point.x, new_spawn_point.y }, origin_, scale_factor);
        new_spawn_point.x = new_position.x;
        new_spawn_point.y = new_position.y;
        new_spawn_points.emplace_back(spawn_point_id, new_spawn_point);
    }
    game_state.map.SetSpawnPointsById(new_spawn_points);

    // TODO: start using StateManager to avoid looping over all soldiers all the time
    // TODO: StateManager should handle moving and repositioning skeleton parts
    //       and then should notify Observers about the move
    for (const auto& [soldier_id, soldier_position] : original_soldier_positions_) {
        for (auto& soldier : game_state.soldiers) {
            if (soldier.id != soldier_id) {
                continue;
            }

            glm::vec2 new_soldier_position =
              Calc::ScalePoint(soldier_position, origin_, scale_factor);

            soldier.particle.position = new_soldier_position;
            soldier.particle.old_position = new_soldier_position;
            RepositionSoldierSkeletonParts(soldier);
        }
    }
}

void ScaleSelectionMapEditorAction::Undo(ClientState& /*client_state*/, State& game_state)
{
    game_state.map.SetPolygonsById(original_polygons_);
    game_state.map.SetSceneriesById(original_sceneries_);
    game_state.map.SetSpawnPointsById(original_spawn_points_);
    for (const auto& [soldier_id, soldier_position] : original_soldier_positions_) {
        for (auto& soldier : game_state.soldiers) {
            if (soldier.id != soldier_id) {
                continue;
            }

            soldier.particle.position = soldier_position;
            soldier.particle.old_position = soldier_position;
            RepositionSoldierSkeletonParts(soldier);
        }
    }
}
} // namespace Soldank
