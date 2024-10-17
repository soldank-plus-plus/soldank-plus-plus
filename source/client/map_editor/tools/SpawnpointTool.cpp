#include "map_editor/tools/SpawnpointTool.hpp"

#include "core/map/PMSEnums.hpp"
#include "core/map/PMSStructs.hpp"
#include "map_editor/actions/AddSpawnPointMapEditorAction.hpp"

namespace Soldank
{
SpawnpointTool::SpawnpointTool(
  const std::function<void(std::unique_ptr<MapEditorAction>)>& add_new_map_editor_action)
    : add_new_map_editor_action_(add_new_map_editor_action)
    , mouse_map_position_()
{
}

void SpawnpointTool::OnSelect(ClientState& client_state, const State& game_state) {}

void SpawnpointTool::OnUnselect(ClientState& client_state) {}

void SpawnpointTool::OnSceneLeftMouseButtonClick(ClientState& client_state, const State& game_state)
{
    PMSSpawnPoint new_spawn_point{ .active = 1,
                                   .x = (int)mouse_map_position_.x,
                                   .y = (int)mouse_map_position_.y,
                                   .type =
                                     client_state.map_editor_state.selected_spawn_point_type };
    auto add_spawn_point_action = std::make_unique<AddSpawnPointMapEditorAction>(new_spawn_point);
    add_new_map_editor_action_(std::move(add_spawn_point_action));
}

void SpawnpointTool::OnSceneLeftMouseButtonRelease(ClientState& client_state,
                                                   const State& game_state)
{
}

void SpawnpointTool::OnSceneRightMouseButtonClick(ClientState& client_state)
{
    client_state.map_editor_state.should_open_spawn_point_type_popup = true;
}

void SpawnpointTool::OnSceneRightMouseButtonRelease() {}

void SpawnpointTool::OnMouseScreenPositionChange(ClientState& client_state,
                                                 glm::vec2 last_mouse_position,
                                                 glm::vec2 new_mouse_position)
{
}

void SpawnpointTool::OnMouseMapPositionChange(ClientState& client_state,
                                              glm::vec2 last_mouse_position,
                                              glm::vec2 new_mouse_position)
{
    mouse_map_position_ = new_mouse_position;
}

void SpawnpointTool::OnModifierKey1Pressed() {}

void SpawnpointTool::OnModifierKey1Released() {}

void SpawnpointTool::OnModifierKey2Pressed() {}

void SpawnpointTool::OnModifierKey2Released() {}

void SpawnpointTool::OnModifierKey3Pressed() {}

void SpawnpointTool::OnModifierKey3Released() {}
} // namespace Soldank
