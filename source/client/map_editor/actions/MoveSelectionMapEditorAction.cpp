#include "map_editor/actions/MoveSelectionMapEditorAction.hpp"

#include "core/physics/SoldierSkeletonPhysics.hpp"

namespace Soldank
{
MoveSelectionMapEditorAction::MoveSelectionMapEditorAction(
  const std::vector<std::pair<std::pair<unsigned int, unsigned int>, glm::vec2>>&
    polygon_vertices_with_position,
  const std::vector<std::pair<unsigned int, glm::vec2>>& scenery_ids_with_position,
  const std::vector<std::pair<unsigned int, glm::ivec2>>& spawn_point_ids_with_position,
  const std::vector<std::pair<unsigned int, glm::vec2>>& original_soldier_positions)
    : polygon_vertices_with_old_position_(polygon_vertices_with_position)
    , scenery_ids_with_position_(scenery_ids_with_position)
    , spawn_point_ids_with_old_position_(spawn_point_ids_with_position)
    , original_soldier_positions_(original_soldier_positions)
    , move_offset_()
{
}

void MoveSelectionMapEditorAction::Execute(ClientState& /*client_state*/, State& game_state)
{
    std::vector<std::pair<std::pair<unsigned int, unsigned int>, glm::vec2>>
      polygon_vertices_with_new_position;
    polygon_vertices_with_new_position.reserve(polygon_vertices_with_old_position_.size());
    for (const auto& [polygon_vertex, old_position] : polygon_vertices_with_old_position_) {
        polygon_vertices_with_new_position.emplace_back(polygon_vertex,
                                                        old_position + move_offset_);
    }
    game_state.map.MovePolygonVerticesById(polygon_vertices_with_new_position);

    std::vector<std::pair<unsigned int, glm::vec2>> scenery_ids_with_new_position;
    scenery_ids_with_new_position.reserve(scenery_ids_with_position_.size());
    for (const auto& [scenery_id, old_position] : scenery_ids_with_position_) {
        scenery_ids_with_new_position.emplace_back(scenery_id, old_position + move_offset_);
    }
    game_state.map.MoveSceneriesById(scenery_ids_with_new_position);

    std::vector<std::pair<unsigned int, glm::ivec2>> spawn_point_ids_with_new_position;
    glm::ivec2 spawn_points_move_offset = move_offset_;
    spawn_point_ids_with_new_position.reserve(spawn_point_ids_with_old_position_.size());
    for (const auto& [spawn_point_id, old_position] : spawn_point_ids_with_old_position_) {
        spawn_point_ids_with_new_position.emplace_back(spawn_point_id,
                                                       old_position + spawn_points_move_offset);
    }
    game_state.map.MoveSpawnPointsById(spawn_point_ids_with_new_position);

    // TODO: start using StateManager to avoid looping over all soldiers all the time
    // TODO: StateManager should handle moving and repositioning skeleton parts
    //       and then should notify Observers about the move
    for (const auto& [soldier_id, soldier_position] : original_soldier_positions_) {
        for (auto& soldier : game_state.soldiers) {
            if (soldier.id != soldier_id) {
                continue;
            }

            glm::vec2 new_soldier_position = soldier_position + move_offset_;

            soldier.particle.position = new_soldier_position;
            soldier.particle.old_position = new_soldier_position;
            RepositionSoldierSkeletonParts(soldier);
        }
    }
}

void MoveSelectionMapEditorAction::Undo(ClientState& /*client_state*/, State& game_state)
{
    game_state.map.MovePolygonVerticesById(polygon_vertices_with_old_position_);
    game_state.map.MoveSceneriesById(scenery_ids_with_position_);
    game_state.map.MoveSpawnPointsById(spawn_point_ids_with_old_position_);
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
