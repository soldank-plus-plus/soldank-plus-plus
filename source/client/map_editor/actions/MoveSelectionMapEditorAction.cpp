#include "map_editor/actions/MoveSelectionMapEditorAction.hpp"
#include <spdlog/spdlog.h>

namespace Soldank
{
MoveSelectionMapEditorAction::MoveSelectionMapEditorAction(
  const std::vector<std::pair<unsigned int, glm::ivec2>>& spawn_point_ids_with_position)
    : spawn_point_ids_with_old_position_(spawn_point_ids_with_position)
    , move_offset_()
{
}

void MoveSelectionMapEditorAction::Execute(ClientState& /*client_state*/, Map& map)
{
    std::vector<std::pair<unsigned int, glm::ivec2>> spawn_point_ids_with_new_position;
    glm::ivec2 spawn_points_move_offset = move_offset_;
    spawn_point_ids_with_new_position.reserve(spawn_point_ids_with_old_position_.size());
    for (const auto& [spawn_point_id, old_position] : spawn_point_ids_with_old_position_) {
        spawn_point_ids_with_new_position.emplace_back(spawn_point_id,
                                                       old_position + spawn_points_move_offset);
    }
    map.MoveSpawnPointsById(spawn_point_ids_with_new_position);
}

void MoveSelectionMapEditorAction::Undo(ClientState& /*client_state*/, Map& map)
{
    map.MoveSpawnPointsById(spawn_point_ids_with_old_position_);
}
} // namespace Soldank
