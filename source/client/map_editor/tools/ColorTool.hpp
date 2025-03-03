#ifndef __COLOR_TOOL_HPP__
#define __COLOR_TOOL_HPP__

#include "map_editor/tools/Tool.hpp"

#include "map_editor/actions/MapEditorAction.hpp"

#include "core/math/Glm.hpp"

namespace Soldank
{
class ColorTool final : public Tool
{
public:
    ColorTool(
      const std::function<void(std::unique_ptr<MapEditorAction>)>& add_new_map_editor_action);
    ~ColorTool() final = default;

    void OnSelect(ClientState& client_state, const StateManager& game_state_manager) final;
    void OnUnselect(ClientState& client_state) final;

    void OnSceneLeftMouseButtonClick(ClientState& client_state,
                                     const StateManager& game_state_manager) final;
    void OnSceneLeftMouseButtonRelease(ClientState& client_state,
                                       const StateManager& game_state_manager) final;
    void OnSceneRightMouseButtonClick(ClientState& client_state) final;
    void OnSceneRightMouseButtonRelease() final;
    void OnMouseScreenPositionChange(ClientState& client_state,
                                     glm::vec2 last_mouse_position,
                                     glm::vec2 new_mouse_position) final;
    void OnMouseMapPositionChange(ClientState& client_state,
                                  glm::vec2 last_mouse_position,
                                  glm::vec2 new_mouse_position,
                                  const StateManager& game_state_manager) final;
    void OnModifierKey1Pressed(ClientState& client_state) final;
    void OnModifierKey1Released(ClientState& client_state) final;
    void OnModifierKey2Pressed(ClientState& client_state) final;
    void OnModifierKey2Released(ClientState& client_state) final;
    void OnModifierKey3Pressed(ClientState& client_state) final;
    void OnModifierKey3Released(ClientState& client_state) final;

private:
    std::function<void(std::unique_ptr<MapEditorAction>)> add_new_map_editor_action_;
    glm::vec2 mouse_map_position_;
};
} // namespace Soldank

#endif
