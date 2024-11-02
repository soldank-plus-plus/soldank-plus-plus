#include "map_editor/actions/TransformSpawnPointsMapEditorAction.hpp"
#include "core/map/PMSStructs.hpp"

namespace Soldank
{
TransformSpawnPointsMapEditorAction::TransformSpawnPointsMapEditorAction(
  const std::vector<std::pair<unsigned int, PMSSpawnPoint>>& old_spawn_points,
  const std::function<PMSSpawnPoint(const PMSSpawnPoint&)>& transform_function)
    : old_spawn_points_(old_spawn_points)
    , transform_function_(transform_function)
{
}

void TransformSpawnPointsMapEditorAction::Execute(ClientState& /*client_state*/, Map& map)
{
    std::vector<std::pair<unsigned int, PMSSpawnPoint>> new_spawn_points;
    for (const auto& old_spawn_point : old_spawn_points_) {
        PMSSpawnPoint new_spawn_point = transform_function_(old_spawn_point.second);
        new_spawn_points.emplace_back(old_spawn_point.first, new_spawn_point);
    }
    map.SetSpawnPointsById(new_spawn_points);
}

void TransformSpawnPointsMapEditorAction::Undo(ClientState& /*client_state*/, Map& map)
{
    map.SetSpawnPointsById(old_spawn_points_);
}
} // namespace Soldank
