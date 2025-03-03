#include "map_editor/actions/AddSpawnPointMapEditorAction.hpp"
#include "core/map/PMSConstants.hpp"

namespace Soldank
{
AddSpawnPointMapEditorAction::AddSpawnPointMapEditorAction(const PMSSpawnPoint& new_spawn_point)
    : added_spawn_point_(new_spawn_point)
    , added_spawn_point_id_(0)
{
}

bool AddSpawnPointMapEditorAction::CanExecute(const ClientState& /*client_state*/,
                                              const StateManager& game_state_manager)
{
    return game_state_manager.GetConstMap().GetSpawnPoints().size() + 1 <= MAX_SPAWN_POINTS_COUNT;

    return true;
}

void AddSpawnPointMapEditorAction::Execute(ClientState& /*client_state*/,
                                           StateManager& game_state_manager)
{
    added_spawn_point_id_ = game_state_manager.GetMap().AddNewSpawnPoint(added_spawn_point_);
}

void AddSpawnPointMapEditorAction::Undo(ClientState& /*client_state*/,
                                        StateManager& game_state_manager)
{
    added_spawn_point_ = game_state_manager.GetMap().RemoveSpawnPointById(added_spawn_point_id_);
    // TODO: remove from selection when implemented
}
} // namespace Soldank
