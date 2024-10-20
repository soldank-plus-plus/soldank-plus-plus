#include "map_editor/tools/SceneryTool.hpp"

namespace Soldank
{
void SceneryTool::OnSelect(ClientState& client_state, const State& game_state)
{
    client_state.map_editor_state.scenery_to_place.scale_x = 1.0F;
    client_state.map_editor_state.scenery_to_place.scale_y = 1.0F;
}

void SceneryTool::OnUnselect(ClientState& client_state) {}

void SceneryTool::OnSceneLeftMouseButtonClick(ClientState& client_state, const State& game_state) {}

void SceneryTool::OnSceneLeftMouseButtonRelease(ClientState& client_state, const State& game_state)
{
}

void SceneryTool::OnSceneRightMouseButtonClick(ClientState& client_state)
{
    client_state.map_editor_state.should_open_scenery_picker_popup = true;
}

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
    client_state.map_editor_state.scenery_to_place.x = new_mouse_position.x;
    client_state.map_editor_state.scenery_to_place.y = new_mouse_position.y;
}

void SceneryTool::OnModifierKey1Pressed() {}

void SceneryTool::OnModifierKey1Released() {}

void SceneryTool::OnModifierKey2Pressed() {}

void SceneryTool::OnModifierKey2Released() {}

void SceneryTool::OnModifierKey3Pressed() {}

void SceneryTool::OnModifierKey3Released() {}
} // namespace Soldank
