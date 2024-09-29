#ifndef __MAP_EDITOR_HPP__
#define __MAP_EDITOR_HPP__

#include "core/state/State.hpp"
#include "map_editor/tools/Tool.hpp"
#include "rendering/ClientState.hpp"

#include "core/math/Glm.hpp"

#include <vector>
#include <memory>

namespace Soldank
{
class MapEditor
{
public:
    MapEditor(ClientState& client_state, State& game_state);

private:
    void OnSelectNewTool(ToolType tool_type);
    void OnSceneLeftMouseButtonClick(ClientState& client_state);
    void OnSceneLeftMouseButtonRelease();
    void OnSceneRightMouseButtonClick();
    void OnSceneRightMouseButtonRelease();
    void OnSceneMiddleMouseButtonClick(ClientState& client_state);
    void OnSceneMiddleMouseButtonRelease();
    static void OnMouseScrollUp(ClientState& client_state);
    static void OnMouseScrollDown(ClientState& client_state);
    void OnMouseScreenPositionChange(ClientState& client_state,
                                     glm::vec2 last_mouse_position,
                                     glm::vec2 new_mouse_position);
    void OnMouseMapPositionChange(ClientState& client_state,
                                  glm::vec2 last_mouse_position,
                                  glm::vec2 new_mouse_position);

    ToolType selected_tool_;
    std::vector<std::unique_ptr<Tool>> tools_;

    glm::vec2 current_mouse_screen_position_;
    glm::vec2 camera_position_on_start_dragging_;
    glm::vec2 mouse_screen_position_on_start_dragging_;
    bool is_dragging_camera_;
};
} // namespace Soldank

#endif
