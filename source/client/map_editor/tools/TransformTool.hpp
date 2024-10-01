#ifndef __TRANSFORM_TOOL_HPP__
#define __TRANSFORM_TOOL_HPP__

#include "map_editor/tools/Tool.hpp"

namespace Soldank
{
class TransformTool final : public Tool
{
public:
    TransformTool() = default;
    ~TransformTool() final = default;

    void OnSelect() final;
    void OnUnselect() final;

    void OnSceneLeftMouseButtonClick(ClientState& client_state, const State& game_state) final;
    void OnSceneLeftMouseButtonRelease() final;
    void OnSceneRightMouseButtonClick() final;
    void OnSceneRightMouseButtonRelease() final;
    void OnMouseScreenPositionChange(ClientState& client_state,
                                     glm::vec2 last_mouse_position,
                                     glm::vec2 new_mouse_position) final;
    void OnMouseMapPositionChange(ClientState& client_state,
                                  glm::vec2 last_mouse_position,
                                  glm::vec2 new_mouse_position) final;
};
} // namespace Soldank

#endif
