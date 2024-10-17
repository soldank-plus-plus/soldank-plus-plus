#ifndef __MAP_EDITOR_STATE_HPP__
#define __MAP_EDITOR_STATE_HPP__

#include "core/map/PMSEnums.hpp"
#include "core/utility/Observable.hpp"
#include "core/math/Glm.hpp"

#include "core/map/PMSStructs.hpp"

#include <optional>
#include <bitset>
#include <array>

namespace Soldank
{
enum class ToolType
{
    Transform = 0,
    Polygon,
    VertexSelection,
    Selection,
    VertexColor,
    Color,
    Texture,
    Scenery,
    Waypoint,
    Spawnpoint,
    ColorPicker
};

struct MapEditorState
{
    ToolType selected_tool = ToolType::Selection;

    bool is_tools_window_visible = true;
    bool is_properties_window_visible = true;
    bool is_display_window_visible = true;
    bool is_palette_window_visible = true;

    bool is_mouse_hovering_over_ui = false;

    Observable<ToolType> event_selected_new_tool;
    Observable<> event_pressed_play;
    Observable<> event_pressed_undo;
    Observable<> event_pressed_redo;
    Observable<const PMSPolygon&, const std::bitset<3>&> event_polygon_selected;

    // If specified, we render first edge of the polygon
    // that is being created with the polygon tool
    std::optional<PMSPolygon> polygon_tool_wip_polygon_edge;
    // If specified, we render the polygon,
    // we handle it differently because we don't want to save t he polygon immediately.
    // After the Polygon Tool is done creating the polygon then we save it.
    std::optional<PMSPolygon> polygon_tool_wip_polygon;

    std::vector<std::pair<unsigned int, std::bitset<3>>> selected_polygon_vertices;
    std::vector<unsigned int> selected_scenery_ids;

    // If specified, render a selection box for the Vertex Selection Tool
    std::optional<std::pair<glm::vec2, glm::vec2>> vertex_selection_box;

    PMSSpawnPointType selected_spawn_point_type = PMSSpawnPointType::General;
    bool should_open_spawn_point_type_popup = false;

    bool draw_spawn_points = true;

    std::array<float, 4> palette_current_color{ 1.0F, 1.0F, 1.0F, 1.0F };
    std::array<glm::vec4, 72> palette_saved_colors;
};
} // namespace Soldank

#endif
