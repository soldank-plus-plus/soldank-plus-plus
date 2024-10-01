#ifndef __COLOR_PICKER_TOOL_HPP__
#define __COLOR_PICKER_TOOL_HPP__

#include "map_editor/tools/Tool.hpp"

namespace Soldank
{
class ColorPickerTool final : public Tool
{
public:
    ColorPickerTool() = default;
    ~ColorPickerTool() final = default;

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
