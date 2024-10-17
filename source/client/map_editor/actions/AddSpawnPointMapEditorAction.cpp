#include "map_editor/actions/AddSpawnPointMapEditorAction.hpp"

namespace Soldank
{
AddSpawnPointMapEditorAction::AddSpawnPointMapEditorAction(const PMSSpawnPoint& new_spawn_point)
    : added_spawn_point_(new_spawn_point)
    , added_spawn_point_id_(0)
{
}

void AddSpawnPointMapEditorAction::Execute(ClientState& /*client_state*/, Map& map)
{
    added_spawn_point_id_ = map.AddNewSpawnPoint(added_spawn_point_);
}

void AddSpawnPointMapEditorAction::Undo(ClientState& /*client_state*/, Map& map)
{
    added_spawn_point_ = map.RemoveSpawnPointById(added_spawn_point_id_);
    // TODO: remove from selection when implemented
}
} // namespace Soldank
