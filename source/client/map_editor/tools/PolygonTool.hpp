#ifndef __POLYGON_TOOL_HPP__
#define __POLYGON_TOOL_HPP__

#include "map_editor/actions/MapEditorAction.hpp"
#include "map_editor/tools/Tool.hpp"

#include "core/utility/Observable.hpp"

#include "core/math/Glm.hpp"

#include <memory>

namespace Soldank
{
class PolygonTool final : public Tool
{
public:
    PolygonTool(
      const std::function<void(std::unique_ptr<MapEditorAction>)>& add_new_map_editor_action);
    ~PolygonTool() final = default;

    void OnSelect() final;
    void OnUnselect() final;

    void OnSceneLeftMouseButtonClick(ClientState& client_state) final;
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
    std::function<void(std::unique_ptr<MapEditorAction>)> add_new_map_editor_action_;
    glm::vec2 mouse_map_position_;
};
} // namespace Soldank

#endif
