#include "map_editor/tools/WaypointTool.hpp"

namespace Soldank
{
void WaypointTool::OnSelect(ClientState& client_state, const StateManager& game_state_manager) {}

void WaypointTool::OnUnselect(ClientState& client_state) {}

void WaypointTool::OnSceneLeftMouseButtonClick(ClientState& client_state,
                                               const StateManager& game_state_manager)
{
}

void WaypointTool::OnSceneLeftMouseButtonRelease(ClientState& client_state,
                                                 const StateManager& game_state_manager)
{
}

void WaypointTool::OnSceneRightMouseButtonClick(ClientState& client_state) {}

void WaypointTool::OnSceneRightMouseButtonRelease() {}

void WaypointTool::OnMouseScreenPositionChange(ClientState& client_state,
                                               glm::vec2 last_mouse_position,
                                               glm::vec2 new_mouse_position)
{
}

void WaypointTool::OnMouseMapPositionChange(ClientState& client_state,
                                            glm::vec2 last_mouse_position,
                                            glm::vec2 new_mouse_position,
                                            const StateManager& game_state_manager)
{
}

void WaypointTool::OnModifierKey1Pressed(ClientState& client_state) {}

void WaypointTool::OnModifierKey1Released(ClientState& client_state) {}

void WaypointTool::OnModifierKey2Pressed(ClientState& client_state) {}

void WaypointTool::OnModifierKey2Released(ClientState& client_state) {}

void WaypointTool::OnModifierKey3Pressed(ClientState& client_state) {}

void WaypointTool::OnModifierKey3Released(ClientState& client_state) {}
} // namespace Soldank
