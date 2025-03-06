#ifndef __TOOL_HPP__
#define __TOOL_HPP__

#include "rendering/ClientState.hpp"
#include "core/state/StateManager.hpp"

#include "core/math/Glm.hpp"

namespace Soldank
{
class Tool
{
public:
    virtual ~Tool() = default;

    virtual void OnSelect(ClientState& client_state, const StateManager& game_state_manager) = 0;
    virtual void OnUnselect(ClientState& client_state) = 0;

    virtual void OnSceneLeftMouseButtonClick(ClientState& client_state,
                                             const StateManager& game_state_manager) = 0;
    virtual void OnSceneLeftMouseButtonRelease(ClientState& client_state,
                                               const StateManager& game_state_manager) = 0;
    virtual void OnSceneRightMouseButtonClick(ClientState& client_state) = 0;
    virtual void OnSceneRightMouseButtonRelease() = 0;
    virtual void OnMouseScreenPositionChange(ClientState& client_state,
                                             glm::vec2 last_mouse_position,
                                             glm::vec2 new_mouse_position) = 0;
    virtual void OnMouseMapPositionChange(ClientState& client_state,
                                          glm::vec2 last_mouse_position,
                                          glm::vec2 new_mouse_position,
                                          const StateManager& game_state_manager) = 0;
    virtual void OnModifierKey1Pressed(ClientState& client_state) = 0;
    virtual void OnModifierKey1Released(ClientState& client_state) = 0;
    virtual void OnModifierKey2Pressed(ClientState& client_state) = 0;
    virtual void OnModifierKey2Released(ClientState& client_state) = 0;
    virtual void OnModifierKey3Pressed(ClientState& client_state) = 0;
    virtual void OnModifierKey3Released(ClientState& client_state) = 0;

protected:
    static glm::vec2 SnapMousePositionToGrid(const glm::vec2& current_mouse_position,
                                             int grid_interval_division);
};
} // namespace Soldank

#endif
