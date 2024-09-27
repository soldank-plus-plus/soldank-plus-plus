#ifndef __DUMMY_TOOL_HPP__
#define __DUMMY_TOOL_HPP__

#include "map_editor/tools/Tool.hpp"

namespace Soldank
{
class DummyTool final : public Tool
{
public:
    DummyTool() = default;
    ~DummyTool() final = default;

    void OnSelect() final;
    void OnUnselect() final;
    void OnSceneLeftMouseButtonClick() final;
    void OnMouseMove(glm::vec2 new_mouse_position) final;
};
} // namespace Soldank

#endif
