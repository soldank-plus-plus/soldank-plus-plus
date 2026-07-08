module;

#include <string>

export module MapEditor.EditorDocument;

import ClientState;

import Shared.Core.State.StateManager;

export namespace Soldank
{
class EditorDocument
{
public:
    EditorDocument(ClientState& client_state, StateManager& game_state_manager)
        : client_state_(client_state)
        , game_state_manager_(game_state_manager)
    {
    }

    void SaveCurrentMapOrOpenSaveAs()
    {
        if (game_state_manager_.GetConstMap().GetName()) {
            game_state_manager_.GetMap().SaveMap("maps/" +
                                                 *game_state_manager_.GetConstMap().GetName());
            MarkClean();
        } else {
            client_state_.map_editor_state.should_open_save_as_modal = true;
        }
    }

    void OpenMapSettings() { client_state_.map_editor_state.should_open_map_settings_modal = true; }

    void MarkClean() { client_state_.map_editor_state.is_map_changed = false; }

private:
    ClientState& client_state_;
    StateManager& game_state_manager_;
};
} // namespace Soldank
