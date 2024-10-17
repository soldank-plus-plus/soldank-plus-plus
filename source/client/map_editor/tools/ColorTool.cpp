#include "map_editor/tools/ColorTool.hpp"

namespace Soldank
{
void ColorTool::OnSelect(ClientState& client_state, const State& game_state) {}

void ColorTool::OnUnselect(ClientState& client_state) {}

void ColorTool::OnSceneLeftMouseButtonClick(ClientState& client_state, const State& game_state) {}

void ColorTool::OnSceneLeftMouseButtonRelease(ClientState& client_state, const State& game_state) {}

void ColorTool::OnSceneRightMouseButtonClick(ClientState& client_state) {}

void ColorTool::OnSceneRightMouseButtonRelease() {}

void ColorTool::OnMouseScreenPositionChange(ClientState& client_state,
                                            glm::vec2 last_mouse_position,
                                            glm::vec2 new_mouse_position)
{
}

void ColorTool::OnMouseMapPositionChange(ClientState& client_state,
                                         glm::vec2 last_mouse_position,
                                         glm::vec2 new_mouse_position)
{
}

void ColorTool::OnModifierKey1Pressed() {}

void ColorTool::OnModifierKey1Released() {}

void ColorTool::OnModifierKey2Pressed() {}

void ColorTool::OnModifierKey2Released() {}

void ColorTool::OnModifierKey3Pressed() {}

void ColorTool::OnModifierKey3Released() {}
} // namespace Soldank
