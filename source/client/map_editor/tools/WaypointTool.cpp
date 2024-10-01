#include "map_editor/tools/WaypointTool.hpp"

namespace Soldank
{
void WaypointTool::OnSelect() {}

void WaypointTool::OnUnselect() {}

void WaypointTool::OnSceneLeftMouseButtonClick(ClientState& client_state, const State& game_state)
{
}

void WaypointTool::OnSceneLeftMouseButtonRelease() {}

void WaypointTool::OnSceneRightMouseButtonClick() {}

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
} // namespace Soldank
