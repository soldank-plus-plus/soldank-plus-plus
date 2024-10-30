#ifndef __ROTATE_SELECTION_MAP_EDITOR_ACTION_HPP__
#define __ROTATE_SELECTION_MAP_EDITOR_ACTION_HPP__

#include "map_editor/actions/MapEditorAction.hpp"

#include "core/map/PMSStructs.hpp"

#include <bitset>

namespace Soldank
{
class RotateSelectionMapEditorAction final : public MapEditorAction
{
public:
    RotateSelectionMapEditorAction(
      const std::vector<std::pair<std::pair<unsigned int, std::bitset<3>>, PMSPolygon>>&
        original_polygon_vertices,
      const std::vector<std::pair<unsigned int, PMSScenery>>& original_sceneries,
      const std::vector<std::pair<unsigned int, PMSSpawnPoint>>& original_spawn_points,
      const glm::vec2& origin,
      const glm::vec2& reference_position);

    void Execute(ClientState& client_state, Map& map) final;
    void Undo(ClientState& client_state, Map& map) final;

    void SetCurrentMousePosition(const glm::vec2& new_current_mouse_position)
    {
        current_mouse_position_ = new_current_mouse_position;
    }

private:
    std::vector<std::pair<std::pair<unsigned int, std::bitset<3>>, PMSPolygon>>
      original_polygon_vertices_;
    std::vector<std::pair<unsigned int, PMSScenery>> original_sceneries_;
    std::vector<std::pair<unsigned int, PMSSpawnPoint>> original_spawn_points_;

    std::vector<std::pair<unsigned int, PMSPolygon>> original_polygons_;

    glm::vec2 origin_;
    glm::vec2 reference_position_;
    glm::vec2 current_mouse_position_;
};
} // namespace Soldank

#endif
