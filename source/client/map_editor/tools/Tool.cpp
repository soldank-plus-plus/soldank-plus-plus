module;

export module Tool;

import Extern.Glm;

import ClientState;

import Shared.Core.State.StateManager;

export namespace Soldank
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
                                             int grid_interval_division)
    {
        int offset_x = grid_interval_division / 2;
        int offset_y = grid_interval_division / 2;
        if (current_mouse_position.x < 0.0F) {
            offset_x = -offset_x;
        }
        if (current_mouse_position.y < 0.0F) {
            offset_y = -offset_y;
        }

        int snapped_x = (int)current_mouse_position.x + offset_x;
        int snapped_y = (int)current_mouse_position.y + offset_y;

        snapped_x =
          snapped_x - (snapped_x - grid_interval_division * (snapped_x / grid_interval_division));
        snapped_y =
          snapped_y - (snapped_y - grid_interval_division * (snapped_y / grid_interval_division));

        return { snapped_x, snapped_y };
    }
};
} // namespace Soldank
