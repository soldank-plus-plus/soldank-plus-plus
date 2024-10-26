#ifndef __MOVE_SELECTION_MAP_EDITOR_ACTION_HPP__
#define __MOVE_SELECTION_MAP_EDITOR_ACTION_HPP__

#include "map_editor/actions/MapEditorAction.hpp"

#include "core/map/PMSStructs.hpp"

namespace Soldank
{
class MoveSelectionMapEditorAction final : public MapEditorAction
{
public:
    MoveSelectionMapEditorAction(
      const std::vector<std::pair<unsigned int, glm::ivec2>>& spawn_point_ids_with_position);

    void Execute(ClientState& client_state, Map& map) final;
    void Undo(ClientState& client_state, Map& map) final;

    void SetMoveOffset(const glm::vec2& new_move_offset) { move_offset_ = new_move_offset; }

private:
    std::vector<std::pair<unsigned int, glm::ivec2>> spawn_point_ids_with_old_position_;

    glm::vec2 move_offset_;
};
} // namespace Soldank

#endif
