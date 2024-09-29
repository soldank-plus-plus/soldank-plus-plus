#ifndef __MAP_EDITOR_ACTION_HPP__
#define __MAP_EDITOR_ACTION_HPP__

#include "core/map/Map.hpp"

namespace Soldank
{
class MapEditorAction
{
public:
    virtual ~MapEditorAction() = default;
    virtual void Execute(Map& map) = 0;
    virtual void Undo(Map& map) = 0;
};
} // namespace Soldank

#endif
