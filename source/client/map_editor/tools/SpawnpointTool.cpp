#include "map_editor/tools/SpawnpointTool.hpp"

namespace Soldank
{
void SpawnpointTool::OnSelect() {}

void SpawnpointTool::OnUnselect(ClientState& client_state) {}

void SpawnpointTool::OnSceneLeftMouseButtonClick(ClientState& client_state, const State& game_state)
{
}

void SpawnpointTool::OnSceneLeftMouseButtonRelease(ClientState& client_state,
                                                   const State& game_state)
{
}

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

void SpawnpointTool::OnModifierKey1Pressed() {}

void SpawnpointTool::OnModifierKey1Released() {}

void SpawnpointTool::OnModifierKey2Pressed() {}

void SpawnpointTool::OnModifierKey2Released() {}

void SpawnpointTool::OnModifierKey3Pressed() {}

void SpawnpointTool::OnModifierKey3Released() {}
} // namespace Soldank
