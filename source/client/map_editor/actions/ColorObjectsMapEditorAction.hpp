#ifndef __COLOR_OBJECTS_MAP_EDITOR_ACTION_HPP__
#define __COLOR_OBJECTS_MAP_EDITOR_ACTION_HPP__

#include "map_editor/actions/MapEditorAction.hpp"

#include "core/map/PMSStructs.hpp"

#include <vector>

namespace Soldank
{
class ColorObjectsMapEditorAction final : public MapEditorAction
{
public:
    ColorObjectsMapEditorAction(
      const PMSColor& new_color,
      const std::vector<std::pair<std::pair<unsigned int, unsigned int>, PMSColor>>&
        polygon_vertices_with_old_color,
      const std::vector<std::pair<unsigned int, PMSColor>>& scenery_ids_with_old_color);

    bool CanExecute(const ClientState& client_state, const StateManager& game_state_manager) final;
    void Execute(ClientState& client_state, StateManager& game_state_manager) final;
    void Undo(ClientState& client_state, StateManager& game_state_manager) final;

private:
    PMSColor new_color_;
    std::vector<std::pair<std::pair<unsigned int, unsigned int>, PMSColor>>
      polygon_vertices_with_old_color_;
    std::vector<std::pair<unsigned int, PMSColor>> scenery_ids_with_old_color_;
};
} // namespace Soldank

#endif
