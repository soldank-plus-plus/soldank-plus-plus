#ifndef __WAYPOINT_TOOL_HPP__
#define __WAYPOINT_TOOL_HPP__

#include "map_editor/tools/Tool.hpp"

namespace Soldank
{
class WaypointTool final : public Tool
{
public:
    WaypointTool() = default;
    ~WaypointTool() final = default;

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
};
} // namespace Soldank

#endif
