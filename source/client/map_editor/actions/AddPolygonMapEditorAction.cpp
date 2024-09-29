#include "map_editor/actions/AddPolygonMapEditorAction.hpp"

namespace Soldank
{
AddPolygonMapEditorAction::AddPolygonMapEditorAction(const PMSPolygon& new_polygon)
    : added_polygon_(new_polygon)
{
}

void AddPolygonMapEditorAction::Execute(Map& map)
{
    added_polygon_ = map.AddNewPolygon(added_polygon_);
}

void AddPolygonMapEditorAction::Undo(Map& map)
{
    map.RemovePolygonById(added_polygon_.id);
}
} // namespace Soldank
