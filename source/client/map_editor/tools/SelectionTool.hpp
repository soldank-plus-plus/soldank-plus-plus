#ifndef __SELECTION_TOOL_HPP__
#define __SELECTION_TOOL_HPP__

#include "map_editor/actions/MapEditorAction.hpp"
#include "map_editor/tools/Tool.hpp"

#include "core/utility/Observable.hpp"

#include "core/math/Glm.hpp"

#include <memory>

namespace Soldank
{
class SelectionTool final : public Tool
{
public:
    ~SelectionTool() final = default;

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

private:
    void SelectNextObject(ClientState& client_state,
                          const State& game_state,
                          unsigned int start_index,
                          bool look_for_polygon_initially);

    glm::vec2 mouse_map_position_;
};
} // namespace Soldank

#endif
