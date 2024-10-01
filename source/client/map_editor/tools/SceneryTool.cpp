#include "map_editor/tools/SceneryTool.hpp"

namespace Soldank
{
void SceneryTool::OnSelect() {}

void SceneryTool::OnUnselect() {}

void SceneryTool::OnSceneLeftMouseButtonClick(ClientState& client_state, const State& game_state) {}

void SceneryTool::OnSceneLeftMouseButtonRelease() {}

void SceneryTool::OnSceneRightMouseButtonClick() {}

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
} // namespace Soldank
