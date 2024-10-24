#include "map_editor/tools/Tool.hpp"

namespace Soldank
{
glm::vec2 Tool::SnapMousePositionToGrid(const glm::vec2& current_mouse_position,
                                        int grid_interval_division)
{
    int offset_x = grid_interval_division / 2;
    int offset_y = grid_interval_division / 2;
    if (current_mouse_position.x < 0.0F) {
        offset_x = -offset_x;
    }
    if (current_mouse_position.y < 0.0F) {
        offset_y = -offset_y;
    }

    int snapped_x = (int)current_mouse_position.x + offset_x;
    int snapped_y = (int)current_mouse_position.y + offset_y;

    snapped_x =
      snapped_x - (snapped_x - grid_interval_division * (snapped_x / grid_interval_division));
    snapped_y =
      snapped_y - (snapped_y - grid_interval_division * (snapped_y / grid_interval_division));

    return { snapped_x, snapped_y };
}
} // namespace Soldank
