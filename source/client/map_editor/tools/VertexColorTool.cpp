#include "map_editor/tools/VertexColorTool.hpp"

namespace Soldank
{
void VertexColorTool::OnSelect() {}

void VertexColorTool::OnUnselect() {}

void VertexColorTool::OnSceneLeftMouseButtonClick(ClientState& client_state,
                                                  const State& game_state)
{
}

void VertexColorTool::OnSceneLeftMouseButtonRelease() {}

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
} // namespace Soldank
