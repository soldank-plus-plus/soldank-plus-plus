#ifndef __POLYGON_TOOL_HPP__
#define __POLYGON_TOOL_HPP__

#include "map_editor/tools/Tool.hpp"

namespace Soldank
{
class PolygonTool final : public Tool
{
public:
    PolygonTool() = default;
    ~PolygonTool() final = default;

    void OnSelect() final;
    void OnUnselect() final;
    void OnSceneLeftMouseButtonClick() final;
    void OnMouseMove(glm::vec2 new_mouse_position) final;
};
} // namespace Soldank

#endif
