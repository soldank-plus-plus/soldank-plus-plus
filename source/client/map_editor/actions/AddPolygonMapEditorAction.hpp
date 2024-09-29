#ifndef __ADD_POLYGON_MAP_EDITOR_ACTION_HPP__
#define __ADD_POLYGON_MAP_EDITOR_ACTION_HPP__

#include "map_editor/actions/MapEditorAction.hpp"

#include "core/map/PMSStructs.hpp"

namespace Soldank
{
class AddPolygonMapEditorAction final : public MapEditorAction
{
public:
    AddPolygonMapEditorAction(const PMSPolygon& new_polygon);

    void Execute(Map& map) final;
    void Undo(Map& map) final;

private:
    PMSPolygon added_polygon_;
};
} // namespace Soldank

#endif
