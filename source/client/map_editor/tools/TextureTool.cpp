#include "map_editor/tools/TextureTool.hpp"

namespace Soldank
{
void TextureTool::OnSelect(ClientState& client_state, const StateManager& game_state_manager) {}

void TextureTool::OnUnselect(ClientState& client_state) {}

void TextureTool::OnSceneLeftMouseButtonClick(ClientState& client_state,
                                              const StateManager& game_state_manager)
{
}

void TextureTool::OnSceneLeftMouseButtonRelease(ClientState& client_state,
                                                const StateManager& game_state_manager)
{
}

void TextureTool::OnSceneRightMouseButtonClick(ClientState& client_state) {}

void TextureTool::OnSceneRightMouseButtonRelease() {}

void TextureTool::OnMouseScreenPositionChange(ClientState& client_state,
                                              glm::vec2 last_mouse_position,
                                              glm::vec2 new_mouse_position)
{
}

void TextureTool::OnMouseMapPositionChange(ClientState& client_state,
                                           glm::vec2 last_mouse_position,
                                           glm::vec2 new_mouse_position,
                                           const StateManager& game_state_manager)
{
}

void TextureTool::OnModifierKey1Pressed(ClientState& client_state) {}

void TextureTool::OnModifierKey1Released(ClientState& client_state) {}

void TextureTool::OnModifierKey2Pressed(ClientState& client_state) {}

void TextureTool::OnModifierKey2Released(ClientState& client_state) {}

void TextureTool::OnModifierKey3Pressed(ClientState& client_state) {}

void TextureTool::OnModifierKey3Released(ClientState& client_state) {}
} // namespace Soldank
