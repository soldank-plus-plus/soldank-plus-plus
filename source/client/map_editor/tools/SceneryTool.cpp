#include "map_editor/tools/SceneryTool.hpp"

namespace Soldank
{
void SceneryTool::OnSelect(ClientState& client_state, const State& game_state) {}

void SceneryTool::OnUnselect(ClientState& client_state) {}

void SceneryTool::OnSceneLeftMouseButtonClick(ClientState& client_state, const State& game_state) {}

void SceneryTool::OnSceneLeftMouseButtonRelease(ClientState& client_state, const State& game_state)
{
}

void SceneryTool::OnSceneRightMouseButtonClick(ClientState& client_state) {}

void SceneryTool::OnSceneRightMouseButtonRelease() {}

void SceneryTool::OnMouseScreenPositionChange(ClientState& client_state,
                                              glm::vec2 last_mouse_position,
                                              glm::vec2 new_mouse_position)
{
}

void SceneryTool::OnMouseMapPositionChange(ClientState& client_state,
                                           glm::vec2 last_mouse_position,
                                           glm::vec2 new_mouse_position)
{
}

void SceneryTool::OnModifierKey1Pressed() {}

void SceneryTool::OnModifierKey1Released() {}

void SceneryTool::OnModifierKey2Pressed() {}

void SceneryTool::OnModifierKey2Released() {}

void SceneryTool::OnModifierKey3Pressed() {}

void SceneryTool::OnModifierKey3Released() {}
} // namespace Soldank
