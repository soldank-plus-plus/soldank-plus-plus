#ifndef __TOOL_HPP__
#define __TOOL_HPP__

#include "core/math/Glm.hpp"

namespace Soldank
{
class Tool
{
public:
    virtual ~Tool() = default;

    virtual void OnSelect() = 0;
    virtual void OnUnselect() = 0;
    virtual void OnSceneLeftMouseButtonClick() = 0;
    virtual void OnMouseMove(glm::vec2 new_mouse_position) = 0;
};
} // namespace Soldank

#endif
