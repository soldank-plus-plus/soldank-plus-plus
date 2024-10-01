#include "map_editor/tools/TextureTool.hpp"

namespace Soldank
{
void TextureTool::OnSelect() {}

void TextureTool::OnUnselect() {}

void TextureTool::OnSceneLeftMouseButtonClick(ClientState& client_state, const State& game_state) {}

void TextureTool::OnSceneLeftMouseButtonRelease() {}

void TextureTool::OnSceneRightMouseButtonClick() {}

void TextureTool::OnSceneRightMouseButtonRelease() {}

void TextureTool::OnMouseScreenPositionChange(ClientState& client_state,
                                              glm::vec2 last_mouse_position,
                                              glm::vec2 new_mouse_position)
{
}

void TextureTool::OnMouseMapPositionChange(ClientState& client_state,
                                           glm::vec2 last_mouse_position,
                                           glm::vec2 new_mouse_position)
{
}
} // namespace Soldank
