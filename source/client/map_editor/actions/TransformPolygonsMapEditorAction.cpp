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

bool TransformPolygonsMapEditorAction::CanExecute(const ClientState& /*client_state*/,
                                                  const StateManager& /*game_state_manager*/)
{
    return true;
}

void TransformPolygonsMapEditorAction::Execute(ClientState& /*client_state*/,
                                               StateManager& game_state_manager)
{
    std::vector<std::pair<unsigned int, PMSPolygon>> new_polygons;
    for (const auto& old_polygon : old_polygons_) {
        PMSPolygon new_polygon = transform_function_(old_polygon.second);
        new_polygons.emplace_back(old_polygon.first, new_polygon);
    }
    game_state_manager.GetMap().SetPolygonsById(new_polygons);
}

void TransformPolygonsMapEditorAction::Undo(ClientState& /*client_state*/,
                                            StateManager& game_state_manager)
{
    game_state_manager.GetMap().SetPolygonsById(old_polygons_);
}
} // namespace Soldank
