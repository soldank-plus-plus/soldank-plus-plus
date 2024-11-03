#include "map_editor/actions/RemoveSelectionMapEditorAction.hpp"
#include <string>

namespace Soldank
{
RemoveSelectionMapEditorAction::RemoveSelectionMapEditorAction(const ClientState& client_state,
                                                               Map& map)
{
    for (const auto& selected_polygon_vertices :
         client_state.map_editor_state.selected_polygon_vertices) {
        if (selected_polygon_vertices.second.all()) {
            polygon_ids_to_remove_.push_back(selected_polygon_vertices.first);
            removed_polygons_.push_back(map.GetPolygons().at(selected_polygon_vertices.first));
        }
    }

    for (unsigned int selected_spawn_point_id :
         client_state.map_editor_state.selected_spawn_point_ids) {

        spawn_point_ids_to_remove_.push_back(selected_spawn_point_id);
        removed_spawn_points_.emplace_back(selected_spawn_point_id,
                                           map.GetSpawnPoints().at(selected_spawn_point_id));
    }

    for (unsigned int selected_scenery_id : client_state.map_editor_state.selected_scenery_ids) {
        scenery_ids_to_remove_.push_back(selected_scenery_id);
        std::string scenery_file_name;
        for (unsigned int i = 0; const auto& scenery_type : map.GetSceneryTypes()) {
            if (i == map.GetSceneryInstances().at(selected_scenery_id).style - 1) {
                scenery_file_name = scenery_type.name;
            }
            ++i;
        }
        removed_sceneries_.push_back(
          { selected_scenery_id,
            { map.GetSceneryInstances().at(selected_scenery_id), scenery_file_name } });
    }
}

void RemoveSelectionMapEditorAction::Execute(ClientState& client_state, State& game_state)
{
    game_state.map.RemovePolygonsById(polygon_ids_to_remove_);
    game_state.map.RemoveSpawnPointsById(spawn_point_ids_to_remove_);
    game_state.map.RemoveSceneriesById(scenery_ids_to_remove_);

    client_state.map_editor_state.selected_polygon_vertices.clear();
    client_state.map_editor_state.selected_scenery_ids.clear();
    client_state.map_editor_state.selected_spawn_point_ids.clear();
}

void RemoveSelectionMapEditorAction::Undo(ClientState& /*client_state*/, State& game_state)
{
    game_state.map.AddPolygons(removed_polygons_);
    game_state.map.AddSpawnPoints(removed_spawn_points_);
    game_state.map.AddSceneries(removed_sceneries_);
}
} // namespace Soldank
