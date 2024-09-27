#ifndef __MAP_EDITOR_ACTION_HPP__
#define __MAP_EDITOR_ACTION_HPP__

namespace Soldank
{
class MapEditorAction
{
public:
    virtual ~MapEditorAction() = default;
    virtual void Execute() = 0;
    virtual void Undo() = 0;
};
} // namespace Soldank

#endif
