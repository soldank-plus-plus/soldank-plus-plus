#ifndef __MAP_EDITOR_UI_HPP__
#define __MAP_EDITOR_UI_HPP__

#include "rendering/ClientState.hpp"
#include "core/state/StateManager.hpp"

namespace Soldank::MapEditorUI
{
void Render(State& game_state, ClientState& client_state, double frame_percent, int fps);
}

#endif
