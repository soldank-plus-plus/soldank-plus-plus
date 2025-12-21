module;

#include "core/map/PMSConstants.hpp"
#include "core/map/PMSStructs.hpp"
#include "core/state/StateManager.hpp"

export module AddObjectsMapEditorAction;

import MapEditorAction;
import ClientState;

export namespace Soldank
{
class AddObjectsMapEditorAction final : public MapEditorAction
{
public:
    AddObjectsMapEditorAction(
      const std::vector<PMSPolygon>& copied_polygons,
      const std::vector<std::pair<unsigned int, std::pair<PMSScenery, std::string>>>&
        copied_sceneries,
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

    bool CanExecute(const ClientState& /*client_state*/,
                    const StateManager& game_state_manager) final
    {
        if (copied_polygons_.size() + game_state_manager.GetConstMap().GetPolygonsCount() >
            MAX_POLYGONS_COUNT) {
            return false;
        }

        if (copied_sceneries_.size() +
              game_state_manager.GetConstMap().GetSceneryInstances().size() >
            MAX_SCENERIES_COUNT) {
            return false;
        }

        if (copied_spawn_points_.size() + game_state_manager.GetConstMap().GetSpawnPoints().size() >
            MAX_SPAWN_POINTS_COUNT) {
            return false;
        }

        return true;
    }

    void Execute(ClientState& /*client_state*/, StateManager& game_state_manager) final
    {
        game_state_manager.GetMap().AddPolygons(copied_polygons_);
        game_state_manager.GetMap().AddSceneries(copied_sceneries_);
        game_state_manager.GetMap().AddSpawnPoints(copied_spawn_points_);
    }

    void Undo(ClientState& /*client_state*/, StateManager& game_state_manager) final
    {
        game_state_manager.GetMap().RemovePolygonsById(polygon_ids_to_remove_);
        game_state_manager.GetMap().RemoveSpawnPointsById(spawn_point_ids_to_remove_);
        game_state_manager.GetMap().RemoveSceneriesById(scenery_ids_to_remove_);
    }

private:
    std::vector<PMSPolygon> copied_polygons_;
    std::vector<std::pair<unsigned int, std::pair<PMSScenery, std::string>>> copied_sceneries_;
    std::vector<std::pair<unsigned int, PMSSpawnPoint>> copied_spawn_points_;
    std::vector<unsigned int> polygon_ids_to_remove_;
    std::vector<unsigned int> spawn_point_ids_to_remove_;
    std::vector<unsigned int> scenery_ids_to_remove_;
};
} // namespace Soldank
