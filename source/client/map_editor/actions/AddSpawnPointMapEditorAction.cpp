export module AddSpawnPointMapEditorAction;

import MapEditorAction;
import ClientState;

import Shared.Core.State.StateManager;
import Shared.Core.Map.PMSConstants;
import Shared.Core.Map.PMSStructs;

export namespace Soldank
{
class AddSpawnPointMapEditorAction final : public MapEditorAction
{
public:
    AddSpawnPointMapEditorAction(const PMSSpawnPoint& new_spawn_point)
        : added_spawn_point_(new_spawn_point)
    {
    }

    bool CanExecute(const ClientState& /*client_state*/,
                    const StateManager& game_state_manager) final
    {
        return game_state_manager.GetConstMap().GetSpawnPoints().size() + 1 <=
               MAX_SPAWN_POINTS_COUNT;

        return true;
    }

    void Execute(ClientState& /*client_state*/, StateManager& game_state_manager) final
    {
        added_spawn_point_id_ = game_state_manager.GetMap().AddNewSpawnPoint(added_spawn_point_);
    }

    void Undo(ClientState& /*client_state*/, StateManager& game_state_manager) final
    {
        added_spawn_point_ =
          game_state_manager.GetMap().RemoveSpawnPointById(added_spawn_point_id_);
        // TODO: remove from selection when implemented
    }

private:
    PMSSpawnPoint added_spawn_point_;
    unsigned int added_spawn_point_id_{};
};
} // namespace Soldank
