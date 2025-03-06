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
    enum class ColorPickerMode
    {
        ClosestObject = 0,
        Pixel,
    };

    void SetColorPickerMode(ColorPickerMode new_color_picker_mode, ClientState& client_state);

    ColorPickerMode color_picker_mode_;
};
} // namespace Soldank

#endif
