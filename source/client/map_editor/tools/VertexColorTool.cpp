#include "map_editor/tools/VertexColorTool.hpp"

namespace Soldank
{
void VertexColorTool::OnSelect(ClientState& client_state, const StateManager& game_state_manager) {}

void VertexColorTool::OnUnselect(ClientState& client_state) {}

void VertexColorTool::OnSceneLeftMouseButtonClick(ClientState& client_state,
                                                  const StateManager& game_state_manager)
{
}

void VertexColorTool::OnSceneLeftMouseButtonRelease(ClientState& client_state,
                                                    const StateManager& game_state_manager)
{
}

void VertexColorTool::OnSceneRightMouseButtonClick(ClientState& client_state) {}

void VertexColorTool::OnSceneRightMouseButtonRelease() {}

void VertexColorTool::OnMouseScreenPositionChange(ClientState& client_state,
                                                  glm::vec2 last_mouse_position,
                                                  glm::vec2 new_mouse_position)
{
}

void VertexColorTool::OnMouseMapPositionChange(ClientState& client_state,
                                               glm::vec2 last_mouse_position,
                                               glm::vec2 new_mouse_position,
                                               const StateManager& game_state_manager)
{
}

void VertexColorTool::OnModifierKey1Pressed(ClientState& client_state) {}

void VertexColorTool::OnModifierKey1Released(ClientState& client_state) {}

void VertexColorTool::OnModifierKey2Pressed(ClientState& client_state) {}

void VertexColorTool::OnModifierKey2Released(ClientState& client_state) {}

void VertexColorTool::OnModifierKey3Pressed(ClientState& client_state) {}

void VertexColorTool::OnModifierKey3Released(ClientState& client_state) {}
} // namespace Soldank
