#ifndef __TRANSFORM_SPAWN_POINTS_MAP_EDITOR_ACTION_HPP__
#define __TRANSFORM_SPAWN_POINTS_MAP_EDITOR_ACTION_HPP__

#include "map_editor/actions/MapEditorAction.hpp"

#include "core/map/PMSStructs.hpp"

namespace Soldank
{
class TransformSpawnPointsMapEditorAction final : public MapEditorAction
{
public:
    TransformSpawnPointsMapEditorAction(
      const std::vector<std::pair<unsigned int, PMSSpawnPoint>>& old_spawn_points,
      const std::function<PMSSpawnPoint(const PMSSpawnPoint&)>& transform_function);

    bool CanExecute(const ClientState& client_state, const State& game_state) final;
    void Execute(ClientState& client_state, State& game_state) final;
    void Undo(ClientState& client_state, State& game_state) final;

private:
    std::vector<std::pair<unsigned int, PMSSpawnPoint>> old_spawn_points_;
    std::function<PMSSpawnPoint(const PMSSpawnPoint&)> transform_function_;
};
} // namespace Soldank

#endif
