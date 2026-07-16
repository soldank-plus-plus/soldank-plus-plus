module;

#include <string>
#include <optional>
#include <span>
#include <bitset>
#include <array>
#include <utility>
#include <vector>

export module MapEditorState;

import Extern.Glm;

import Application.Input.Shortcut;

import Shared.Core.Map.PMSEnums;
import Shared.Core.Map.PMSConstants;
import Shared.Core.Map.PMSStructs;

import Shared.Core.Utility.Observable;

export namespace Soldank
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

enum class SettingsSection
{
    General,
    Shortcuts
};

enum class ShortcutSelection
{
    None,
    PlayMode,
    Tool
};

struct MapEditorState
{
    ToolType selected_tool = ToolType::Selection;

    bool is_tools_window_visible = true;
    bool is_properties_window_visible = true;
    bool is_display_window_visible = true;
    bool is_palette_window_visible = true;
    bool is_tool_details_window_visible = true;

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
    Observable<> event_pressed_copy;
    Observable<> event_pressed_paste;
    Observable<const PMSPolygon&, const std::bitset<3>&> event_polygon_selected;
    Observable<const std::string&> event_scenery_texture_changed;
    Observable<PMSSpawnPointType> event_selected_spawn_points_type_changed;
    Observable<int> event_selected_sceneries_level_changed;
    Observable<float> event_selected_polygons_bounciness_changed;
    Observable<PMSPolygonType> event_selected_polygons_type_changed;
    Observable<> event_pixel_color_under_cursor_requested;
    Observable<> event_palette_saved_colors_changed;
    Observable<> event_shortcuts_changed;
    Observable<> event_ui_scale_changed;

    Observable<const std::string&> event_save_map;
    Observable<const std::string&> event_set_map_name;
    Observable<const std::string&> event_set_map_description;
    Observable<PMSWeatherType> event_set_map_weather_type;
    Observable<PMSStepType> event_set_map_step_type;
    Observable<unsigned char> event_set_map_grenades_count;
    Observable<unsigned char> event_set_map_medikits_count;
    Observable<int> event_set_map_jet_count;
    Observable<const PMSColor&> event_set_map_background_top_color;
    Observable<const PMSColor&> event_set_map_background_bottom_color;
    Observable<const std::string&> event_set_map_texture_name;

    // If specified, we render first edge of the polygon
    // that is being created with the polygon tool
    std::optional<PMSPolygon> polygon_tool_wip_polygon_edge;
    // If specified, we render the polygon,
    // we handle it differently because we don't want to save t he polygon immediately.
    // After the Polygon Tool is done creating the polygon then we save it.
    std::optional<PMSPolygon> polygon_tool_wip_polygon;
    PMSPolygonType polygon_tool_polygon_type = PMSPolygonType::Normal;
    float polygon_tool_wip_polygon_bounciness = 100.0F;
    bool should_open_polygon_type_popup = false;

    std::vector<std::pair<unsigned int, std::bitset<3>>> selected_polygon_vertices;
    std::vector<unsigned int> selected_scenery_ids;
    std::vector<unsigned int> selected_spawn_point_ids;
    std::vector<unsigned int> selected_soldier_ids;

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

    std::array<char, DESCRIPTION_MAX_LENGTH + 1> map_description_input{};

    unsigned int polygon_texture_opengl_id = 0;

    std::vector<std::string> all_textures_in_directory;
    std::vector<std::string> all_sceneries_in_directory;

    std::array<char, MAP_NAME_MAX_LENGTH> save_as_map_name_input{};
    std::array<char, TEXTURE_NAME_MAX_LENGTH> texture_search_filter{};
    std::array<char, SCENERY_NAME_MAX_LENGTH> scenery_search_filter{};

    bool should_open_map_settings_modal = false;
    bool should_open_settings_modal = false;
    bool is_play_mode_shortcut_capture_active = false;
    int tool_shortcut_capture_index = -1;
    int shortcut_capture_binding_index = -1;
    int shortcut_capture_modifiers = 0;
    ShortcutSelection selected_shortcut = ShortcutSelection::None;
    int selected_tool_shortcut_index = -1;
    int selected_shortcut_binding_index = -1;
    bool should_open_save_as_modal = false;
    bool should_open_selection_context_menu = false;

    bool draw_wireframe = false;

    ShortcutBindings shortcut_bindings = GetDefaultShortcutBindings();
    float ui_scale = 1.0F;
    float pending_ui_scale = 1.0F;
    SettingsSection selected_settings_section = SettingsSection::General;

    int& GetPlayModeShortcut()
    {
        return shortcut_bindings.at(GetShortcutIndex(ShortcutId::MapEditorPlayMode));
    }
    const int& GetPlayModeShortcut() const
    {
        return shortcut_bindings.at(GetShortcutIndex(ShortcutId::MapEditorPlayMode));
    }
    int& GetToolShortcut(std::size_t tool_index)
    {
        return shortcut_bindings.at(GetShortcutIndex(GetToolShortcutId(tool_index)));
    }
    const int& GetToolShortcut(std::size_t tool_index) const
    {
        return shortcut_bindings.at(GetShortcutIndex(GetToolShortcutId(tool_index)));
    }
    std::span<int, 11> GetToolShortcuts()
    {
        return std::span<int, 11>(shortcut_bindings.data() + 1, 11);
    }
    std::span<const int, 11> GetToolShortcuts() const
    {
        return std::span<const int, 11>(shortcut_bindings.data() + 1, 11);
    }
    std::span<int> GetMenuShortcuts()
    {
        return { shortcut_bindings.data() + 12, SHORTCUT_COUNT - 12 };
    }
    std::span<const int> GetMenuShortcuts() const
    {
        return { shortcut_bindings.data() + 12, SHORTCUT_COUNT - 12 };
    }

    glm::vec4 last_requested_pixel_color;

    std::string current_tool_action_description;

    bool is_map_changed = false;
};
} // namespace Soldank
