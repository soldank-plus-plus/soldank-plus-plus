#ifndef __TOOL_HPP__
#define __TOOL_HPP__

#include "rendering/ClientState.hpp"

#include "core/math/Glm.hpp"

namespace Soldank
{
class Tool
{
public:
    virtual ~Tool() = default;

    virtual void OnSelect() = 0;
    virtual void OnUnselect() = 0;

    virtual void OnSceneLeftMouseButtonClick(ClientState& client_state) = 0;
    virtual void OnSceneLeftMouseButtonRelease() = 0;
    virtual void OnSceneRightMouseButtonClick() = 0;
    virtual void OnSceneRightMouseButtonRelease() = 0;
    virtual void OnMouseScreenPositionChange(ClientState& client_state,
                                             glm::vec2 last_mouse_position,
                                             glm::vec2 new_mouse_position) = 0;
    virtual void OnMouseMapPositionChange(ClientState& client_state,
                                          glm::vec2 last_mouse_position,
                                          glm::vec2 new_mouse_position) = 0;
};
} // namespace Soldank

#endif
