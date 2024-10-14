#include "map_editor/tools/TextureTool.hpp"

namespace Soldank
{
void TextureTool::OnSelect(ClientState& client_state, const State& game_state) {}

void TextureTool::OnUnselect(ClientState& client_state) {}

void TextureTool::OnSceneLeftMouseButtonClick(ClientState& client_state, const State& game_state) {}

void TextureTool::OnSceneLeftMouseButtonRelease(ClientState& client_state, const State& game_state)
{
}

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

void TextureTool::OnModifierKey1Pressed() {}

void TextureTool::OnModifierKey1Released() {}

void TextureTool::OnModifierKey2Pressed() {}

void TextureTool::OnModifierKey2Released() {}

void TextureTool::OnModifierKey3Pressed() {}

void TextureTool::OnModifierKey3Released() {}
} // namespace Soldank
