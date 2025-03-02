#ifndef __DEBUG_UI_HPP__
#define __DEBUG_UI_HPP__

#include "rendering/ClientState.hpp"
#include "core/state/StateManager.hpp"

namespace Soldank::DebugUI
{
void Render(const StateManager& game_state_manager,
            ClientState& client_state,
            double frame_percent,
            int fps);
bool GetWantCaptureMouse();
} // namespace Soldank::DebugUI

#endif
