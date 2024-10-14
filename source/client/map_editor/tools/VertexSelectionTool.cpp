#include "map_editor/tools/VertexSelectionTool.hpp"

namespace Soldank
{
void VertexSelectionTool::OnSelect() {}

void VertexSelectionTool::OnUnselect(ClientState& client_state)
{
    client_state.map_editor_state.vertex_selection_box = std::nullopt;
}

void VertexSelectionTool::OnSceneLeftMouseButtonClick(ClientState& client_state,
                                                      const State& game_state)
{
    client_state.map_editor_state.vertex_selection_box = { { mouse_map_position_ },
                                                           { mouse_map_position_ } };
}

void VertexSelectionTool::OnSceneLeftMouseButtonRelease(ClientState& client_state)
{
    client_state.map_editor_state.vertex_selection_box = std::nullopt;
}

void VertexSelectionTool::OnSceneRightMouseButtonClick() {}

void VertexSelectionTool::OnSceneRightMouseButtonRelease() {}

void VertexSelectionTool::OnMouseScreenPositionChange(ClientState& client_state,
                                                      glm::vec2 last_mouse_position,
                                                      glm::vec2 new_mouse_position)
{
}

void VertexSelectionTool::OnMouseMapPositionChange(ClientState& client_state,
                                                   glm::vec2 last_mouse_position,
                                                   glm::vec2 new_mouse_position)
{
    mouse_map_position_ = new_mouse_position;

    if (client_state.map_editor_state.vertex_selection_box) {
        client_state.map_editor_state.vertex_selection_box->second = new_mouse_position;
    }
}

void VertexSelectionTool::OnModifierKey1Pressed() {}

void VertexSelectionTool::OnModifierKey1Released() {}

void VertexSelectionTool::OnModifierKey2Pressed() {}

void VertexSelectionTool::OnModifierKey2Released() {}

void VertexSelectionTool::OnModifierKey3Pressed() {}

void VertexSelectionTool::OnModifierKey3Released() {}
} // namespace Soldank
