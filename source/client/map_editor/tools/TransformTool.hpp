#ifndef __TRANSFORM_TOOL_HPP__
#define __TRANSFORM_TOOL_HPP__

#include "map_editor/actions/RotateSelectionMapEditorAction.hpp"
#include "map_editor/tools/Tool.hpp"

#include "map_editor/actions/MapEditorAction.hpp"
#include "map_editor/actions/MoveSelectionMapEditorAction.hpp"
#include "map_editor/actions/ScaleSelectionMapEditorAction.hpp"

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
    void OnModifierKey1Pressed(ClientState& client_state) final;
    void OnModifierKey1Released(ClientState& client_state) final;
    void OnModifierKey2Pressed(ClientState& client_state) final;
    void OnModifierKey2Released(ClientState& client_state) final;
    void OnModifierKey3Pressed(ClientState& client_state) final;
    void OnModifierKey3Released(ClientState& client_state) final;

private:
    enum class TransformMode
    {
        Move = 0,
        Scale,
        Rotate
    };

    static void SetupSelectionBox(ClientState& client_state, const State& game_state);

    void SetTransformMode(TransformMode new_transform_mode, ClientState& client_state);

    std::function<void(std::unique_ptr<MapEditorAction>)> add_new_map_editor_action_;
    std::function<void(MapEditorAction*)> execute_without_adding_map_editor_action_;

    std::optional<std::unique_ptr<MoveSelectionMapEditorAction>> maybe_move_selection_action_;
    std::optional<std::unique_ptr<ScaleSelectionMapEditorAction>> maybe_scale_selection_action_;
    std::optional<std::unique_ptr<RotateSelectionMapEditorAction>> maybe_rotate_selection_action_;
    glm::vec2 mouse_map_position_on_last_click_;

    TransformMode transform_mode_;
};
} // namespace Soldank

#endif
