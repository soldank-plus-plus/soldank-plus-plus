#include "map_editor/tools/SceneryTool.hpp"
#include "map_editor/actions/AddSceneryMapEditorAction.hpp"

namespace Soldank
{
SceneryTool::SceneryTool(
  const std::function<void(std::unique_ptr<MapEditorAction>)>& add_new_map_editor_action)
    : add_new_map_editor_action_(add_new_map_editor_action)
{
}

void SceneryTool::OnSelect(ClientState& client_state, const State& game_state)
{
    client_state.map_editor_state.scenery_to_place.scale_x = 1.0F;
    client_state.map_editor_state.scenery_to_place.scale_y = 1.0F;
    client_state.map_editor_state.scenery_to_place.active = true;
}

void SceneryTool::OnUnselect(ClientState& client_state) {}

void SceneryTool::OnSceneLeftMouseButtonClick(ClientState& client_state, const State& game_state)
{
    client_state.map_editor_state.scenery_to_place.color.red =
      (unsigned char)(client_state.map_editor_state.palette_current_color.at(0) * 255.0F);
    client_state.map_editor_state.scenery_to_place.color.green =
      (unsigned char)(client_state.map_editor_state.palette_current_color.at(1) * 255.0F);
    client_state.map_editor_state.scenery_to_place.color.blue =
      (unsigned char)(client_state.map_editor_state.palette_current_color.at(2) * 255.0F);
    client_state.map_editor_state.scenery_to_place.color.alpha =
      (unsigned char)(client_state.map_editor_state.palette_current_color.at(3) * 255.0F);
    client_state.map_editor_state.scenery_to_place.alpha =
      (int)(client_state.map_editor_state.palette_current_color.at(3) * 255.0F);

    auto add_polygon_action = std::make_unique<AddSceneryMapEditorAction>(
      client_state.map_editor_state.scenery_to_place,
      client_state.map_editor_state.selected_scenery_to_place);
    add_new_map_editor_action_(std::move(add_polygon_action));
}

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
    if (client_state.map_editor_state.is_snap_to_grid_enabled) {
        glm::vec2 snapped_mouse_position = SnapMousePositionToGrid(
          new_mouse_position, client_state.map_editor_state.grid_interval_division);

        client_state.map_editor_state.scenery_to_place.x = snapped_mouse_position.x;
        client_state.map_editor_state.scenery_to_place.y = snapped_mouse_position.y;
    } else {
        client_state.map_editor_state.scenery_to_place.x = new_mouse_position.x;
        client_state.map_editor_state.scenery_to_place.y = new_mouse_position.y;
    }
}

void SceneryTool::OnModifierKey1Pressed() {}

void SceneryTool::OnModifierKey1Released() {}

void SceneryTool::OnModifierKey2Pressed() {}

void SceneryTool::OnModifierKey2Released() {}

void SceneryTool::OnModifierKey3Pressed() {}

void SceneryTool::OnModifierKey3Released() {}
} // namespace Soldank
