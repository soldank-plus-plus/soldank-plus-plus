#include "map_editor/tools/SpawnpointTool.hpp"

namespace Soldank
{
void SpawnpointTool::OnSelect() {}

void SpawnpointTool::OnUnselect() {}

void SpawnpointTool::OnSceneLeftMouseButtonClick(ClientState& client_state, const State& game_state)
{
}

void SpawnpointTool::OnSceneLeftMouseButtonRelease() {}

void SpawnpointTool::OnSceneRightMouseButtonClick() {}

void SpawnpointTool::OnSceneRightMouseButtonRelease() {}

void SpawnpointTool::OnMouseScreenPositionChange(ClientState& client_state,
                                                 glm::vec2 last_mouse_position,
                                                 glm::vec2 new_mouse_position)
{
}

void SpawnpointTool::OnMouseMapPositionChange(ClientState& client_state,
                                              glm::vec2 last_mouse_position,
                                              glm::vec2 new_mouse_position)
{
}
} // namespace Soldank
