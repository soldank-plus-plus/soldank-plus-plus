#include "map_editor/actions/AddObjectsMapEditorAction.hpp"
#include "core/map/PMSConstants.hpp"

namespace Soldank
{
AddObjectsMapEditorAction::AddObjectsMapEditorAction(
  const std::vector<PMSPolygon>& copied_polygons,
  const std::vector<std::pair<unsigned int, std::pair<PMSScenery, std::string>>>& copied_sceneries,
  const std::vector<std::pair<unsigned int, PMSSpawnPoint>>& copied_spawn_points)
    : copied_polygons_(copied_polygons)
    , copied_sceneries_(copied_sceneries)
    , copied_spawn_points_(copied_spawn_points)
{
    for (const auto& copied_polygon : copied_polygons) {
        polygon_ids_to_remove_.push_back(copied_polygon.id);
    }

    for (const auto& [copied_scenery_id, _] : copied_sceneries) {
        scenery_ids_to_remove_.push_back(copied_scenery_id);
    }

    for (const auto& [copied_spawn_point_id, _] : copied_spawn_points) {
        spawn_point_ids_to_remove_.push_back(copied_spawn_point_id);
    }
}

bool AddObjectsMapEditorAction::CanExecute(const ClientState& /*client_state*/,
                                           const State& game_state)
{
    if (copied_polygons_.size() + game_state.map.GetPolygonsCount() > MAX_POLYGONS_COUNT) {
        return false;
    }

    if (copied_sceneries_.size() + game_state.map.GetSceneryInstances().size() >
        MAX_SCENERIES_COUNT) {
        return false;
    }

    if (copied_spawn_points_.size() + game_state.map.GetSpawnPoints().size() >
        MAX_SPAWN_POINTS_COUNT) {
        return false;
    }

    return true;
}

void AddObjectsMapEditorAction::Execute(ClientState& /*client_state*/, State& game_state)
{
    game_state.map.AddPolygons(copied_polygons_);
    game_state.map.AddSceneries(copied_sceneries_);
    game_state.map.AddSpawnPoints(copied_spawn_points_);
}

void AddObjectsMapEditorAction::Undo(ClientState& /*client_state*/, State& game_state)
{
    game_state.map.RemovePolygonsById(polygon_ids_to_remove_);
    game_state.map.RemoveSpawnPointsById(spawn_point_ids_to_remove_);
    game_state.map.RemoveSceneriesById(scenery_ids_to_remove_);
}
} // namespace Soldank
