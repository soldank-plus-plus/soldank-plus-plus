#ifndef __MAP_EDITOR_STATE_HPP__
#define __MAP_EDITOR_STATE_HPP__

#include "core/utility/Observable.hpp"
#include "core/math/Glm.hpp"

#include "core/map/PMSStructs.hpp"

#include <optional>

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
    ToolType selected_tool = ToolType::Polygon;

    bool is_mouse_hovering_over_ui = false;

    Observable<ToolType> event_selected_new_tool;
    Observable<> event_pressed_play;
    Observable<> event_pressed_undo;

    // If specified, we render first edge of the polygon
    // that is being created with the polygon tool
    std::optional<PMSPolygon> polygon_tool_wip_polygon_edge;
    // If specified, we render the polygon,
    // we handle it differently because we don't want to save t he polygon immediately.
    // After the Polygon Tool is done creating the polygon then we save it.
    std::optional<PMSPolygon> polygon_tool_wip_polygon;
};
} // namespace Soldank

#endif
