#include "map_editor/actions/ColorObjectsMapEditorAction.hpp"

namespace Soldank
{
ColorObjectsMapEditorAction::ColorObjectsMapEditorAction(
  const PMSColor& new_color,
  const std::vector<std::pair<unsigned int, PMSColor>>& scenery_ids_with_old_color)
    : new_color_(new_color)
    , scenery_ids_with_old_color_(scenery_ids_with_old_color)
{
}

void ColorObjectsMapEditorAction::Execute(ClientState& /*client_state*/, Map& map)
{
    std::vector<std::pair<unsigned int, PMSColor>> scenery_ids_with_new_color;
    scenery_ids_with_new_color.reserve(scenery_ids_with_old_color_.size());
    for (const auto& [a, _] : scenery_ids_with_old_color_) {
        scenery_ids_with_new_color.emplace_back(a, new_color_);
    }

    map.SetSceneriesColorById(scenery_ids_with_new_color);
}

void ColorObjectsMapEditorAction::Undo(ClientState& /*client_state*/, Map& map)
{
    map.SetSceneriesColorById(scenery_ids_with_old_color_);
}
} // namespace Soldank
