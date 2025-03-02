#ifndef __MAP_EDITOR_TOOL_DETAILS_WINDOW_HPP__
#define __MAP_EDITOR_TOOL_DETAILS_WINDOW_HPP__

#include "rendering/ClientState.hpp"
#include "core/state/StateManager.hpp"

namespace Soldank::MapEditorToolDetailsWindow
{
void Render(const StateManager& game_state_manager, ClientState& client_state);
}

#endif
