#include "map_editor/actions/ColorObjectsMapEditorAction.hpp"

namespace Soldank
{
ColorObjectsMapEditorAction::ColorObjectsMapEditorAction(
  const PMSColor& new_color,
  const std::vector<std::pair<std::pair<unsigned int, unsigned int>, PMSColor>>&
    polygon_vertices_with_old_color,
  const std::vector<std::pair<unsigned int, PMSColor>>& scenery_ids_with_old_color)
    : new_color_(new_color)
    , polygon_vertices_with_old_color_(polygon_vertices_with_old_color)
    , scenery_ids_with_old_color_(scenery_ids_with_old_color)
{
}

bool ColorObjectsMapEditorAction::CanExecute(const ClientState& /*client_state*/,
                                             const StateManager& /*game_state_manager*/)
{
    return true;
}

void ColorObjectsMapEditorAction::Execute(ClientState& /*client_state*/,
                                          StateManager& game_state_manager)
{
    std::vector<std::pair<std::pair<unsigned int, unsigned int>, PMSColor>>
      polygon_vertices_with_new_color;
    polygon_vertices_with_new_color.reserve(polygon_vertices_with_old_color_.size());
    for (const auto& [polygon_vertex, _] : polygon_vertices_with_old_color_) {
        polygon_vertices_with_new_color.push_back(
          { { polygon_vertex.first, polygon_vertex.second }, new_color_ });
    }
    game_state_manager.GetMap().SetPolygonVerticesColorById(polygon_vertices_with_new_color);

    std::vector<std::pair<unsigned int, PMSColor>> scenery_ids_with_new_color;
    scenery_ids_with_new_color.reserve(scenery_ids_with_old_color_.size());
    for (const auto& [scenery_id, _] : scenery_ids_with_old_color_) {
        scenery_ids_with_new_color.emplace_back(scenery_id, new_color_);
    }
    game_state_manager.GetMap().SetSceneriesColorById(scenery_ids_with_new_color);
}

void ColorObjectsMapEditorAction::Undo(ClientState& /*client_state*/,
                                       StateManager& game_state_manager)
{
    game_state_manager.GetMap().SetPolygonVerticesColorById(polygon_vertices_with_old_color_);
    game_state_manager.GetMap().SetSceneriesColorById(scenery_ids_with_old_color_);
}
} // namespace Soldank
