#include "map_editor/tools/ColorTool.hpp"

namespace Soldank
{
void ColorTool::OnSelect() {}

void ColorTool::OnUnselect() {}

void ColorTool::OnSceneLeftMouseButtonClick(ClientState& client_state, const State& game_state) {}

void ColorTool::OnSceneLeftMouseButtonRelease() {}

void ColorTool::OnSceneRightMouseButtonClick() {}

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
} // namespace Soldank
