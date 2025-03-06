#include "map_editor/actions/RemoveSelectionMapEditorAction.hpp"
#include <string>

namespace Soldank
{
RemoveSelectionMapEditorAction::RemoveSelectionMapEditorAction(
  const ClientState& client_state,
  const StateManager& game_state_manager)
    : soldier_ids_to_remove_(client_state.map_editor_state.selected_soldier_ids)
{
    for (const auto& selected_polygon_vertices :
         client_state.map_editor_state.selected_polygon_vertices) {
        if (selected_polygon_vertices.second.all()) {
            polygon_ids_to_remove_.push_back(selected_polygon_vertices.first);
            removed_polygons_.push_back(
              game_state_manager.GetConstMap().GetPolygons().at(selected_polygon_vertices.first));
        }
    }

    for (unsigned int selected_spawn_point_id :
         client_state.map_editor_state.selected_spawn_point_ids) {

        spawn_point_ids_to_remove_.push_back(selected_spawn_point_id);
        removed_spawn_points_.emplace_back(
          selected_spawn_point_id,
          game_state_manager.GetConstMap().GetSpawnPoints().at(selected_spawn_point_id));
    }

    for (unsigned int selected_scenery_id : client_state.map_editor_state.selected_scenery_ids) {
        scenery_ids_to_remove_.push_back(selected_scenery_id);
        std::string scenery_file_name;
        for (unsigned int i = 0;
             const auto& scenery_type : game_state_manager.GetConstMap().GetSceneryTypes()) {
            if (i == game_state_manager.GetConstMap()
                         .GetSceneryInstances()
                         .at(selected_scenery_id)
                         .style -
                       1) {
                scenery_file_name = scenery_type.name;
            }
            ++i;
        }
        removed_sceneries_.push_back(
          { selected_scenery_id,
            { game_state_manager.GetConstMap().GetSceneryInstances().at(selected_scenery_id),
              scenery_file_name } });
    }
}

bool RemoveSelectionMapEditorAction::CanExecute(const ClientState& /*client_state*/,
                                                const StateManager& /*game_state_manager*/)
{
    return true;
}

void RemoveSelectionMapEditorAction::Execute(ClientState& client_state,
                                             StateManager& game_state_manager)
{
    game_state_manager.GetMap().RemovePolygonsById(polygon_ids_to_remove_);
    game_state_manager.GetMap().RemoveSpawnPointsById(spawn_point_ids_to_remove_);
    game_state_manager.GetMap().RemoveSceneriesById(scenery_ids_to_remove_);
    for (unsigned int selected_soldier_id : soldier_ids_to_remove_) {
        game_state_manager.RemoveSoldier(selected_soldier_id);
    }

    client_state.map_editor_state.selected_polygon_vertices.clear();
    client_state.map_editor_state.selected_scenery_ids.clear();
    client_state.map_editor_state.selected_spawn_point_ids.clear();
    client_state.map_editor_state.selected_soldier_ids.clear();
}

void RemoveSelectionMapEditorAction::Undo(ClientState& /*client_state*/,
                                          StateManager& game_state_manager)
{
    game_state_manager.GetMap().AddPolygons(removed_polygons_);
    game_state_manager.GetMap().AddSpawnPoints(removed_spawn_points_);
    game_state_manager.GetMap().AddSceneries(removed_sceneries_);
    for (unsigned int selected_soldier_id : soldier_ids_to_remove_) {
        game_state_manager.TransformSoldier(selected_soldier_id, [&](auto& soldier) {
            // TODO: in the future there should be a StateManager method that brings Soldier back
            soldier.active = true;
        });
    }
}
} // namespace Soldank
