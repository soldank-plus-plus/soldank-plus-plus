#include "map_editor/tools/ColorPickerTool.hpp"

namespace Soldank
{
void ColorPickerTool::OnSelect(ClientState& client_state, const State& game_state) {}

void ColorPickerTool::OnUnselect(ClientState& client_state) {}

void ColorPickerTool::OnSceneLeftMouseButtonClick(ClientState& client_state,
                                                  const State& game_state)
{
}

void ColorPickerTool::OnSceneLeftMouseButtonRelease(ClientState& client_state,
                                                    const State& game_state)
{
}

void ColorPickerTool::OnSceneRightMouseButtonClick(ClientState& client_state) {}

void ColorPickerTool::OnSceneRightMouseButtonRelease() {}

void ColorPickerTool::OnMouseScreenPositionChange(ClientState& client_state,
                                                  glm::vec2 last_mouse_position,
                                                  glm::vec2 new_mouse_position)
{
}

void ColorPickerTool::OnMouseMapPositionChange(ClientState& client_state,
                                               glm::vec2 last_mouse_position,
                                               glm::vec2 new_mouse_position)
{
}

void ColorPickerTool::OnModifierKey1Pressed() {}

void ColorPickerTool::OnModifierKey1Released() {}

void ColorPickerTool::OnModifierKey2Pressed() {}

void ColorPickerTool::OnModifierKey2Released() {}

void ColorPickerTool::OnModifierKey3Pressed() {}

void ColorPickerTool::OnModifierKey3Released() {}
} // namespace Soldank
