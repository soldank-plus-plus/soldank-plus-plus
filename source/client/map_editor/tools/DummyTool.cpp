#include "map_editor/tools/DummyTool.hpp"

namespace Soldank
{
void DummyTool::OnSelect() {}

void DummyTool::OnUnselect() {}

void DummyTool::OnSceneLeftMouseButtonClick(ClientState& client_state) {}

void DummyTool::OnSceneLeftMouseButtonRelease() {}

void DummyTool::OnSceneRightMouseButtonClick() {}

void DummyTool::OnSceneRightMouseButtonRelease() {}

void DummyTool::OnMouseScreenPositionChange(ClientState& client_state,
                                            glm::vec2 last_mouse_position,
                                            glm::vec2 new_mouse_position)
{
}

void DummyTool::OnMouseMapPositionChange(ClientState& client_state,
                                         glm::vec2 last_mouse_position,
                                         glm::vec2 new_mouse_position)
{
}
} // namespace Soldank
