#ifndef __REMOVE_SELECTION_MAP_EDITOR_ACTION_HPP__
#define __REMOVE_SELECTION_MAP_EDITOR_ACTION_HPP__

#include "map_editor/actions/MapEditorAction.hpp"

#include "core/map/PMSStructs.hpp"

namespace Soldank
{
class RemoveSelectionMapEditorAction final : public MapEditorAction
{
public:
    RemoveSelectionMapEditorAction(const ClientState& client_state, Map& map);

    void Execute(ClientState& client_state, Map& map) final;
    void Undo(ClientState& client_state, Map& map) final;

private:
    std::vector<unsigned int> polygon_ids_to_remove_;
    std::vector<PMSPolygon> removed_polygons_;
    std::vector<unsigned int> spawn_point_ids_to_remove_;
    std::vector<std::pair<unsigned int, PMSSpawnPoint>> removed_spawn_points_;
};
} // namespace Soldank

#endif
