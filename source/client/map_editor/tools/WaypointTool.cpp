#include "map_editor/tools/WaypointTool.hpp"

namespace Soldank
{
void WaypointTool::OnSelect(ClientState& client_state, const State& game_state) {}

void WaypointTool::OnUnselect(ClientState& client_state) {}

void WaypointTool::OnSceneLeftMouseButtonClick(ClientState& client_state, const State& game_state)
{
}

void WaypointTool::OnSceneLeftMouseButtonRelease(ClientState& client_state, const State& game_state)
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
                                            glm::vec2 new_mouse_position)
{
}

void WaypointTool::OnModifierKey1Pressed() {}

void WaypointTool::OnModifierKey1Released() {}

void WaypointTool::OnModifierKey2Pressed() {}

void WaypointTool::OnModifierKey2Released() {}

void WaypointTool::OnModifierKey3Pressed() {}

void WaypointTool::OnModifierKey3Released() {}
} // namespace Soldank