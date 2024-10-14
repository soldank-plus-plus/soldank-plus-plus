#include "map_editor/tools/VertexSelectionTool.hpp"

namespace Soldank
{
void VertexSelectionTool::OnSelect() {}

void VertexSelectionTool::OnUnselect() {}

void VertexSelectionTool::OnSceneLeftMouseButtonClick(ClientState& client_state,
                                                      const State& game_state)
{
}

void VertexSelectionTool::OnSceneLeftMouseButtonRelease() {}

void VertexSelectionTool::OnSceneRightMouseButtonClick() {}

void VertexSelectionTool::OnSceneRightMouseButtonRelease() {}

void VertexSelectionTool::OnMouseScreenPositionChange(ClientState& client_state,
                                                      glm::vec2 last_mouse_position,
                                                      glm::vec2 new_mouse_position)
{
}

void VertexSelectionTool::OnMouseMapPositionChange(ClientState& client_state,
                                                   glm::vec2 last_mouse_position,
                                                   glm::vec2 new_mouse_position)
{
}

void VertexSelectionTool::OnModifierKey1Pressed() {}

void VertexSelectionTool::OnModifierKey1Released() {}

void VertexSelectionTool::OnModifierKey2Pressed() {}

void VertexSelectionTool::OnModifierKey2Released() {}

void VertexSelectionTool::OnModifierKey3Pressed() {}

void VertexSelectionTool::OnModifierKey3Released() {}
} // namespace Soldank
