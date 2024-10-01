#include "map_editor/tools/TransformTool.hpp"

namespace Soldank
{
void TransformTool::OnSelect() {}

void TransformTool::OnUnselect() {}

void TransformTool::OnSceneLeftMouseButtonClick(ClientState& client_state, const State& game_state)
{
}

void TransformTool::OnSceneLeftMouseButtonRelease() {}

void TransformTool::OnSceneRightMouseButtonClick() {}

void TransformTool::OnSceneRightMouseButtonRelease() {}

void TransformTool::OnMouseScreenPositionChange(ClientState& client_state,
                                                glm::vec2 last_mouse_position,
                                                glm::vec2 new_mouse_position)
{
}

void TransformTool::OnMouseMapPositionChange(ClientState& client_state,
                                             glm::vec2 last_mouse_position,
                                             glm::vec2 new_mouse_position)
{
}
} // namespace Soldank
