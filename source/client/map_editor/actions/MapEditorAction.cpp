export module MapEditorAction;

import ClientState;

import Shared.Core.State.StateManager;

export namespace Soldank
{
class MapEditorAction
{
public:
    virtual ~MapEditorAction() = default;
    virtual bool CanExecute(const ClientState& client_state,
                            const StateManager& game_state_manager) = 0;
    virtual void Execute(ClientState& client_state, StateManager& game_state_manager) = 0;
    virtual void Undo(ClientState& client_state, StateManager& game_state_manager) = 0;
};
} // namespace Soldank
