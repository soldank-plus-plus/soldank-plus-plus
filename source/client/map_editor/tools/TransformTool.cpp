#include "map_editor/tools/TransformTool.hpp"
#include "map_editor/actions/MoveSelectionMapEditorAction.hpp"

namespace Soldank
{
TransformTool::TransformTool(
  const std::function<void(std::unique_ptr<MapEditorAction>)>& add_new_map_editor_action,
  const std::function<void(MapEditorAction*)>& execute_without_adding_map_editor_action)
    : add_new_map_editor_action_(add_new_map_editor_action)
    , execute_without_adding_map_editor_action_(execute_without_adding_map_editor_action)
    , mouse_map_position_on_last_click_()
{
}

void TransformTool::OnSelect(ClientState& client_state, const State& game_state) {}

void TransformTool::OnUnselect(ClientState& client_state) {}

void TransformTool::OnSceneLeftMouseButtonClick(ClientState& client_state, const State& game_state)
{
    std::vector<std::pair<unsigned int, glm::vec2>> scenery_ids_with_position;
    for (const auto& selected_scenery_id : client_state.map_editor_state.selected_scenery_ids) {
        glm::vec2 position = { game_state.map.GetSceneryInstances().at(selected_scenery_id).x,
                               game_state.map.GetSceneryInstances().at(selected_scenery_id).y };
        scenery_ids_with_position.emplace_back(selected_scenery_id, position);
    }

    std::vector<std::pair<unsigned int, glm::ivec2>> spawn_point_ids_with_position;
    for (const auto& selected_spawn_point_id :
         client_state.map_editor_state.selected_spawn_point_ids) {
        glm::ivec2 position = { game_state.map.GetSpawnPoints().at(selected_spawn_point_id).x,
                                game_state.map.GetSpawnPoints().at(selected_spawn_point_id).y };
        spawn_point_ids_with_position.emplace_back(selected_spawn_point_id, position);
    }

    maybe_move_selection_action_ = std::make_unique<MoveSelectionMapEditorAction>(
      scenery_ids_with_position, spawn_point_ids_with_position);
    mouse_map_position_on_last_click_ = client_state.mouse_map_position;
}

void TransformTool::OnSceneLeftMouseButtonRelease(ClientState& /*client_state*/,
                                                  const State& /*game_state*/)
{
    if (maybe_move_selection_action_) {
        add_new_map_editor_action_(std::move(*maybe_move_selection_action_));
        maybe_move_selection_action_ = std::nullopt;
    }
}

void TransformTool::OnSceneRightMouseButtonClick(ClientState& client_state) {}

void TransformTool::OnSceneRightMouseButtonRelease() {}

void TransformTool::OnMouseScreenPositionChange(ClientState& client_state,
                                                glm::vec2 last_mouse_position,
                                                glm::vec2 new_mouse_position)
{
}

void TransformTool::OnMouseMapPositionChange(ClientState& /*client_state*/,
                                             glm::vec2 /*last_mouse_position*/,
                                             glm::vec2 new_mouse_position,
                                             const State& /*game_state*/)
{
    if (maybe_move_selection_action_) {
        glm::vec2 move_offset = new_mouse_position - mouse_map_position_on_last_click_;
        (*maybe_move_selection_action_)->SetMoveOffset(move_offset);
        execute_without_adding_map_editor_action_(maybe_move_selection_action_->get());
    }
}

void TransformTool::OnModifierKey1Pressed() {}

void TransformTool::OnModifierKey1Released() {}

void TransformTool::OnModifierKey2Pressed() {}

void TransformTool::OnModifierKey2Released() {}

void TransformTool::OnModifierKey3Pressed() {}

void TransformTool::OnModifierKey3Released() {}
} // namespace Soldank
