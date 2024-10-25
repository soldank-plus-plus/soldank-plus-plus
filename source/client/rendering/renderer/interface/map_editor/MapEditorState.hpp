#ifndef __MAP_EDITOR_STATE_HPP__
#define __MAP_EDITOR_STATE_HPP__

#include "core/map/PMSEnums.hpp"
#include "core/utility/Observable.hpp"
#include "core/math/Glm.hpp"

#include "core/map/PMSStructs.hpp"

#include <optional>
#include <bitset>
#include <array>
#include <string>

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
    bool is_modal_or_popup_open = false;

    bool is_grid_visible = false;
    bool is_snap_to_grid_enabled = false;
    int grid_interval_division = 4;
    bool is_snap_to_vertices_enabled = false;

    bool is_undo_enabled = false;
    bool is_redo_enabled = false;

    Observable<ToolType> event_selected_new_tool;
    Observable<> event_pressed_play;
    Observable<> event_pressed_undo;
    Observable<> event_pressed_redo;
    Observable<const PMSPolygon&, const std::bitset<3>&> event_polygon_selected;
    Observable<const std::string&> event_scenery_texture_changed;

    // If specified, we render first edge of the polygon
    // that is being created with the polygon tool
    std::optional<PMSPolygon> polygon_tool_wip_polygon_edge;
    // If specified, we render the polygon,
    // we handle it differently because we don't want to save t he polygon immediately.
    // After the Polygon Tool is done creating the polygon then we save it.
    std::optional<PMSPolygon> polygon_tool_wip_polygon;
    PMSPolygonType polygon_tool_polygon_type = PMSPolygonType::Normal;
    bool should_open_polygon_type_popup = false;

    std::vector<std::pair<unsigned int, std::bitset<3>>> selected_polygon_vertices;
    std::vector<unsigned int> selected_scenery_ids;
    std::vector<unsigned int> selected_spawn_point_ids;

    // If specified, render a selection box for the Vertex Selection Tool
    std::optional<std::pair<glm::vec2, glm::vec2>> vertex_selection_box;

    PMSSpawnPointType selected_spawn_point_type = PMSSpawnPointType::General;
    bool should_open_spawn_point_type_popup = false;
    glm::vec2 spawn_point_preview_position;

    std::string selected_scenery_to_place;
    bool should_open_scenery_picker_popup = false;
    PMSScenery scenery_to_place;

    bool draw_spawn_points = true;

    std::array<float, 4> palette_current_color{ 1.0F, 1.0F, 1.0F, 1.0F };
    std::array<glm::vec4, 72> palette_saved_colors;

    std::string map_description_input;

    unsigned int polygon_texture_opengl_id = 0;

    std::vector<std::string> all_textures_in_directory;
    std::vector<std::string> all_sceneries_in_directory;
};
} // namespace Soldank

#endif
