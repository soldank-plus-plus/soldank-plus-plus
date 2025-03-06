#ifndef __ADD_OBJECTS_MAP_EDITOR_ACTION_HPP__
#define __ADD_OBJECTS_MAP_EDITOR_ACTION_HPP__

#include "map_editor/actions/MapEditorAction.hpp"

#include "core/map/PMSStructs.hpp"

namespace Soldank
{
class AddObjectsMapEditorAction final : public MapEditorAction
{
public:
    AddObjectsMapEditorAction(
      const std::vector<PMSPolygon>& copied_polygons,
      const std::vector<std::pair<unsigned int, std::pair<PMSScenery, std::string>>>&
        copied_sceneries,
      const std::vector<std::pair<unsigned int, PMSSpawnPoint>>& copied_spawn_points);

    bool CanExecute(const ClientState& client_state, const StateManager& game_state_manager) final;
    void Execute(ClientState& client_state, StateManager& game_state_manager) final;
    void Undo(ClientState& client_state, StateManager& game_state_manager) final;

private:
    std::vector<PMSPolygon> copied_polygons_;
    std::vector<std::pair<unsigned int, std::pair<PMSScenery, std::string>>> copied_sceneries_;
    std::vector<std::pair<unsigned int, PMSSpawnPoint>> copied_spawn_points_;
    std::vector<unsigned int> polygon_ids_to_remove_;
    std::vector<unsigned int> spawn_point_ids_to_remove_;
    std::vector<unsigned int> scenery_ids_to_remove_;
};
} // namespace Soldank

#endif
