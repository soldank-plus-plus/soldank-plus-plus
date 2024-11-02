#include "map_editor/actions/TransformPolygonsMapEditorAction.hpp"
#include "core/map/PMSStructs.hpp"

namespace Soldank
{
TransformPolygonsMapEditorAction::TransformPolygonsMapEditorAction(
  const std::vector<std::pair<unsigned int, PMSPolygon>>& old_polygons,
  const std::function<PMSPolygon(const PMSPolygon&)>& transform_function)
    : old_polygons_(old_polygons)
    , transform_function_(transform_function)
{
}

void TransformPolygonsMapEditorAction::Execute(ClientState& /*client_state*/, Map& map)
{
    std::vector<std::pair<unsigned int, PMSPolygon>> new_polygons;
    for (const auto& old_polygon : old_polygons_) {
        PMSPolygon new_polygon = transform_function_(old_polygon.second);
        new_polygons.emplace_back(old_polygon.first, new_polygon);
    }
    map.SetPolygonsById(new_polygons);
}

void TransformPolygonsMapEditorAction::Undo(ClientState& /*client_state*/, Map& map)
{
    map.SetPolygonsById(old_polygons_);
}
} // namespace Soldank
