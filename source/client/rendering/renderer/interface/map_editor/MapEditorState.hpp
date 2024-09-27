#ifndef __MAP_EDITOR_STATE_HPP__
#define __MAP_EDITOR_STATE_HPP__

#include "core/utility/Observable.hpp"

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
};
} // namespace Soldank

#endif
