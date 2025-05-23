#ifndef __REMOVE_SELECTION_MAP_EDITOR_ACTION_HPP__
#define __REMOVE_SELECTION_MAP_EDITOR_ACTION_HPP__

#include "map_editor/actions/MapEditorAction.hpp"

#include "core/map/PMSStructs.hpp"

namespace Soldank
{
class RemoveSelectionMapEditorAction final : public MapEditorAction
{
public:
    RemoveSelectionMapEditorAction(const ClientState& client_state,
                                   const StateManager& game_state_manager);

    bool CanExecute(const ClientState& client_state, const StateManager& game_state_manager) final;
    void Execute(ClientState& client_state, StateManager& game_state_manager) final;
    void Undo(ClientState& client_state, StateManager& game_state_manager) final;

private:
    std::vector<unsigned int> polygon_ids_to_remove_;
    std::vector<PMSPolygon> removed_polygons_;
    std::vector<unsigned int> spawn_point_ids_to_remove_;
    std::vector<std::pair<unsigned int, PMSSpawnPoint>> removed_spawn_points_;
    std::vector<unsigned int> scenery_ids_to_remove_;
    std::vector<std::pair<unsigned int, std::pair<PMSScenery, std::string>>> removed_sceneries_;
    std::vector<unsigned int> soldier_ids_to_remove_;
};
} // namespace Soldank

#endif
