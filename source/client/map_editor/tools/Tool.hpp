#ifndef __TOOL_HPP__
#define __TOOL_HPP__

namespace Soldank
{
class Tool
{
public:
    virtual ~Tool() = default;

    virtual void OnSelect() = 0;
    virtual void OnUnselect() = 0;
};
} // namespace Soldank

#endif
