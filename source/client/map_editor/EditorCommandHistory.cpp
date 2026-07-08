module;

#include <memory>
#include <utility>
#include <vector>

export module MapEditor.EditorCommandHistory;

import ClientState;
import MapEditorAction;

import Shared.Core.State.StateManager;

export namespace Soldank
{
class EditorCommandHistory
{
public:
    bool Execute(ClientState& client_state,
                 StateManager& game_state_manager,
                 std::unique_ptr<MapEditorAction> action)
    {
        if (!action->CanExecute(client_state, game_state_manager)) {
            return false;
        }

        undone_actions_.clear();
        action->Execute(client_state, game_state_manager);
        executed_actions_.push_back(std::move(action));
        if (executed_actions_.size() > ACTION_HISTORY_LIMIT) {
            executed_actions_.erase(executed_actions_.begin());
        }

        UpdateButtons(client_state);
        client_state.map_editor_state.is_map_changed = true;
        return true;
    }

    void Undo(ClientState& client_state, StateManager& game_state_manager)
    {
        if (!executed_actions_.empty()) {
            executed_actions_.back()->Undo(client_state, game_state_manager);
            undone_actions_.push_back(std::move(executed_actions_.back()));
            executed_actions_.pop_back();
        }

        UpdateButtons(client_state);
        client_state.map_editor_state.is_map_changed = true;
    }

    void Redo(ClientState& client_state, StateManager& game_state_manager)
    {
        if (!undone_actions_.empty()) {
            executed_actions_.push_back(std::move(undone_actions_.back()));
            undone_actions_.pop_back();
            executed_actions_.back()->Execute(client_state, game_state_manager);
        }

        UpdateButtons(client_state);
        client_state.map_editor_state.is_map_changed = true;
    }

    void UpdateButtons(ClientState& client_state) const
    {
        client_state.map_editor_state.is_undo_enabled = !executed_actions_.empty();
        client_state.map_editor_state.is_redo_enabled = !undone_actions_.empty();
    }

private:
    static const std::size_t ACTION_HISTORY_LIMIT = 50;

    std::vector<std::unique_ptr<MapEditorAction>> executed_actions_;
    std::vector<std::unique_ptr<MapEditorAction>> undone_actions_;
};
} // namespace Soldank
