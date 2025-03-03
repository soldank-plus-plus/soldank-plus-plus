#ifndef __SCALE_SELECTION_MAP_EDITOR_ACTION_HPP__
#define __SCALE_SELECTION_MAP_EDITOR_ACTION_HPP__

#include "map_editor/actions/MapEditorAction.hpp"

#include "core/map/PMSStructs.hpp"
#include "core/entities/Soldier.hpp"

#include <bitset>

namespace Soldank
{
class ScaleSelectionMapEditorAction final : public MapEditorAction
{
public:
    ScaleSelectionMapEditorAction(
      const std::vector<std::pair<std::pair<unsigned int, std::bitset<3>>, PMSPolygon>>&
        original_polygon_vertices,
      const std::vector<std::pair<unsigned int, PMSScenery>>& original_sceneries,
      const std::vector<std::pair<unsigned int, PMSSpawnPoint>>& original_spawn_points,
      const std::vector<std::pair<unsigned int, glm::vec2>>& original_soldier_positions,
      const glm::vec2& origin,
      const glm::vec2& reference_position,
      bool is_scale_horizontal = true,
      bool is_scale_vertical = true);

    bool CanExecute(const ClientState& client_state, const StateManager& game_state_manager) final;
    void Execute(ClientState& client_state, StateManager& game_state_manager) final;
    void Undo(ClientState& client_state, StateManager& game_state_manager) final;

    void SetCurrentMousePosition(const glm::vec2& new_current_mouse_position)
    {
        current_mouse_position_ = new_current_mouse_position;
    }

private:
    std::vector<std::pair<std::pair<unsigned int, std::bitset<3>>, PMSPolygon>>
      original_polygon_vertices_;
    std::vector<std::pair<unsigned int, PMSScenery>> original_sceneries_;
    std::vector<std::pair<unsigned int, PMSSpawnPoint>> original_spawn_points_;
    std::vector<std::pair<unsigned int, glm::vec2>> original_soldier_positions_;

    std::vector<std::pair<unsigned int, PMSPolygon>> original_polygons_;

    glm::vec2 origin_;
    glm::vec2 reference_position_;
    glm::vec2 current_mouse_position_;

    bool is_scale_horizontal_;
    bool is_scale_vertical_;
};
} // namespace Soldank

#endif
