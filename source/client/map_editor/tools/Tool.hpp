#ifndef __TOOL_HPP__
#define __TOOL_HPP__

#include "rendering/ClientState.hpp"
#include "core/state/State.hpp"

#include "core/math/Glm.hpp"

namespace Soldank
{
class Tool
{
public:
    virtual ~Tool() = default;

    virtual void OnSelect() = 0;
    virtual void OnUnselect(ClientState& client_state) = 0;

    virtual void OnSceneLeftMouseButtonClick(ClientState& client_state,
                                             const State& game_state) = 0;
    virtual void OnSceneLeftMouseButtonRelease(ClientState& client_state,
                                               const State& game_state) = 0;
    virtual void OnSceneRightMouseButtonClick() = 0;
    virtual void OnSceneRightMouseButtonRelease() = 0;
    virtual void OnMouseScreenPositionChange(ClientState& client_state,
                                             glm::vec2 last_mouse_position,
                                             glm::vec2 new_mouse_position) = 0;
    virtual void OnMouseMapPositionChange(ClientState& client_state,
                                          glm::vec2 last_mouse_position,
                                          glm::vec2 new_mouse_position) = 0;
    virtual void OnModifierKey1Pressed() = 0;
    virtual void OnModifierKey1Released() = 0;
    virtual void OnModifierKey2Pressed() = 0;
    virtual void OnModifierKey2Released() = 0;
    virtual void OnModifierKey3Pressed() = 0;
    virtual void OnModifierKey3Released() = 0;
};
} // namespace Soldank

#endif
