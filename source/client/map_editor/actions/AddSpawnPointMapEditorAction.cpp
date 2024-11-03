#include "map_editor/actions/AddSpawnPointMapEditorAction.hpp"

namespace Soldank
{
AddSpawnPointMapEditorAction::AddSpawnPointMapEditorAction(const PMSSpawnPoint& new_spawn_point)
    : added_spawn_point_(new_spawn_point)
    , added_spawn_point_id_(0)
{
}

void AddSpawnPointMapEditorAction::Execute(ClientState& /*client_state*/, State& game_state)
{
    added_spawn_point_id_ = game_state.map.AddNewSpawnPoint(added_spawn_point_);
}

void AddSpawnPointMapEditorAction::Undo(ClientState& /*client_state*/, State& game_state)
{
    added_spawn_point_ = game_state.map.RemoveSpawnPointById(added_spawn_point_id_);
    // TODO: remove from selection when implemented
}
} // namespace Soldank
