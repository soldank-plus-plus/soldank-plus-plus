#ifndef __MAP_EDITOR_UI_HPP__
#define __MAP_EDITOR_UI_HPP__

#include "rendering/ClientState.hpp"
#include "core/state/StateManager.hpp"
#include "rendering/renderer/interface/map_editor/MapEditorState.hpp"

namespace Soldank::MapEditorUI
{
void Render(const StateManager& game_state_manager, ClientState& client_state);
}

#endif
