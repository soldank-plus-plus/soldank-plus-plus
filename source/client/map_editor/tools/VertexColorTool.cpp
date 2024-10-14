#include "map_editor/tools/VertexColorTool.hpp"

namespace Soldank
{
void VertexColorTool::OnSelect() {}

void VertexColorTool::OnUnselect(ClientState& client_state) {}

void VertexColorTool::OnSceneLeftMouseButtonClick(ClientState& client_state,
                                                  const State& game_state)
{
}

void VertexColorTool::OnSceneLeftMouseButtonRelease(ClientState& client_state,
                                                    const State& game_state)
{
}

void VertexColorTool::OnSceneRightMouseButtonClick() {}

void VertexColorTool::OnSceneRightMouseButtonRelease() {}

void VertexColorTool::OnMouseScreenPositionChange(ClientState& client_state,
                                                  glm::vec2 last_mouse_position,
                                                  glm::vec2 new_mouse_position)
{
}

void VertexColorTool::OnMouseMapPositionChange(ClientState& client_state,
                                               glm::vec2 last_mouse_position,
                                               glm::vec2 new_mouse_position)
{
}

void VertexColorTool::OnModifierKey1Pressed() {}

void VertexColorTool::OnModifierKey1Released() {}

void VertexColorTool::OnModifierKey2Pressed() {}

void VertexColorTool::OnModifierKey2Released() {}

void VertexColorTool::OnModifierKey3Pressed() {}

void VertexColorTool::OnModifierKey3Released() {}
} // namespace Soldank
