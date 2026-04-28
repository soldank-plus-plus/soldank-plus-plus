module;

#include <functional>
#include <utility>
#include <vector>

export module TransformSpawnPointsMapEditorAction;

import MapEditorAction;
import ClientState;

import Shared.Core.State.StateManager;
import Shared.Core.Map.PMSStructs;

export namespace Soldank
{
class TransformSpawnPointsMapEditorAction final : public MapEditorAction
{
public:
    TransformSpawnPointsMapEditorAction(
      const std::vector<std::pair<unsigned int, PMSSpawnPoint>>& old_spawn_points,
      const std::function<PMSSpawnPoint(const PMSSpawnPoint&)>& transform_function)
        : old_spawn_points_(old_spawn_points)
        , transform_function_(transform_function)
    {
    }

    bool CanExecute(const ClientState& /*client_state*/,
                    const StateManager& /*game_state_manager*/) final
    {
        return true;
    }

    void Execute(ClientState& /*client_state*/, StateManager& game_state_manager) final
    {
        std::vector<std::pair<unsigned int, PMSSpawnPoint>> new_spawn_points;
        for (const auto& old_spawn_point : old_spawn_points_) {
            PMSSpawnPoint new_spawn_point = transform_function_(old_spawn_point.second);
            new_spawn_points.emplace_back(old_spawn_point.first, new_spawn_point);
        }
        game_state_manager.GetMap().SetSpawnPointsById(new_spawn_points);
    }

    void Undo(ClientState& /*client_state*/, StateManager& game_state_manager) final
    {
        game_state_manager.GetMap().SetSpawnPointsById(old_spawn_points_);
    }

private:
    std::vector<std::pair<unsigned int, PMSSpawnPoint>> old_spawn_points_;
    std::function<PMSSpawnPoint(const PMSSpawnPoint&)> transform_function_;
};
} // namespace Soldank
