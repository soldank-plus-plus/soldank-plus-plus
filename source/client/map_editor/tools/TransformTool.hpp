#ifndef __TRANSFORM_TOOL_HPP__
#define __TRANSFORM_TOOL_HPP__

#include "map_editor/tools/Tool.hpp"

#include "map_editor/actions/MapEditorAction.hpp"
#include "map_editor/actions/MoveSelectionMapEditorAction.hpp"

#include <memory>
#include <optional>

namespace Soldank
{
class TransformTool final : public Tool
{
public:
    TransformTool(
      const std::function<void(std::unique_ptr<MapEditorAction>)>& add_new_map_editor_action,
      const std::function<void(MapEditorAction*)>& execute_without_adding_map_editor_action);
    ~TransformTool() final = default;

    void OnSelect(ClientState& client_state, const State& game_state) final;
    void OnUnselect(ClientState& client_state) final;

    void OnSceneLeftMouseButtonClick(ClientState& client_state, const State& game_state) final;
    void OnSceneLeftMouseButtonRelease(ClientState& client_state, const State& game_state) final;
    void OnSceneRightMouseButtonClick(ClientState& client_state) final;
    void OnSceneRightMouseButtonRelease() final;
    void OnMouseScreenPositionChange(ClientState& client_state,
                                     glm::vec2 last_mouse_position,
                                     glm::vec2 new_mouse_position) final;
    void OnMouseMapPositionChange(ClientState& client_state,
                                  glm::vec2 last_mouse_position,
                                  glm::vec2 new_mouse_position,
                                  const State& game_state) final;
    void OnModifierKey1Pressed() final;
    void OnModifierKey1Released() final;
    void OnModifierKey2Pressed() final;
    void OnModifierKey2Released() final;
    void OnModifierKey3Pressed() final;
    void OnModifierKey3Released() final;

private:
    static void SetupSelectionBox(ClientState& client_state, const State& game_state);

    std::function<void(std::unique_ptr<MapEditorAction>)> add_new_map_editor_action_;
    std::function<void(MapEditorAction*)> execute_without_adding_map_editor_action_;

    std::optional<std::unique_ptr<MoveSelectionMapEditorAction>> maybe_move_selection_action_;
    glm::vec2 mouse_map_position_on_last_click_;
};
} // namespace Soldank

#endif
