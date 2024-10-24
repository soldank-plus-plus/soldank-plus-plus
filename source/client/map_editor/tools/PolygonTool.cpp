#include "map_editor/tools/PolygonTool.hpp"

#include "core/map/PMSEnums.hpp"
#include "core/map/PMSStructs.hpp"
#include "map_editor/actions/AddPolygonMapEditorAction.hpp"
#include "rendering/renderer/interface/map_editor/MapEditorState.hpp"

namespace Soldank
{
PolygonTool::PolygonTool(
  const std::function<void(std::unique_ptr<MapEditorAction>)>& add_new_map_editor_action)
    : add_new_map_editor_action_(add_new_map_editor_action)
    , mouse_map_position_()
{
}

void PolygonTool::OnSelect(ClientState& /*client_state*/, const State& /*game_state*/) {}

void PolygonTool::OnUnselect(ClientState& client_state)
{
    client_state.map_editor_state.polygon_tool_wip_polygon_edge = std::nullopt;
    client_state.map_editor_state.polygon_tool_wip_polygon = std::nullopt;
}

void PolygonTool::OnSceneLeftMouseButtonClick(ClientState& client_state,
                                              const State& /*game_state*/)
{
    if (client_state.map_editor_state.polygon_tool_wip_polygon) {
        auto add_polygon_action = std::make_unique<AddPolygonMapEditorAction>(
          *client_state.map_editor_state.polygon_tool_wip_polygon);
        add_new_map_editor_action_(std::move(add_polygon_action));
        client_state.map_editor_state.polygon_tool_wip_polygon = std::nullopt;
    } else if (!client_state.map_editor_state.polygon_tool_wip_polygon_edge) {
        client_state.map_editor_state.polygon_tool_wip_polygon_edge = PMSPolygon{};
        client_state.map_editor_state.polygon_tool_wip_polygon_edge->id = 0;
        client_state.map_editor_state.polygon_tool_wip_polygon_edge->polygon_type =
          PMSPolygonType::Normal;
        client_state.map_editor_state.polygon_tool_wip_polygon_edge->vertices.at(0) = {
            .x = mouse_map_position_.x,
            .y = mouse_map_position_.y,
            .z = 1,
            .rhw = 1.0F,
            .color = GetCurrentPaletteColor(client_state),
            .texture_s = mouse_map_position_.x / client_state.current_polygon_texture_dimensions.x,
            .texture_t = mouse_map_position_.y / client_state.current_polygon_texture_dimensions.y
        };
        client_state.map_editor_state.polygon_tool_wip_polygon_edge->vertices.at(1) = {
            .x = mouse_map_position_.x,
            .y = mouse_map_position_.y,
            .z = 1,
            .rhw = 1.0F,
            .color = GetCurrentPaletteColor(client_state),
            .texture_s = mouse_map_position_.x / client_state.current_polygon_texture_dimensions.x,
            .texture_t = mouse_map_position_.y / client_state.current_polygon_texture_dimensions.y
        };
    } else if (!client_state.map_editor_state.polygon_tool_wip_polygon) {
        client_state.map_editor_state.polygon_tool_wip_polygon =
          *client_state.map_editor_state.polygon_tool_wip_polygon_edge;
        client_state.map_editor_state.polygon_tool_wip_polygon->vertices.at(2) = {
            .x = mouse_map_position_.x,
            .y = mouse_map_position_.y,
            .z = 1,
            .rhw = 1.0F,
            .color = GetCurrentPaletteColor(client_state),
            .texture_s = mouse_map_position_.x / client_state.current_polygon_texture_dimensions.x,
            .texture_t = mouse_map_position_.y / client_state.current_polygon_texture_dimensions.y
        };

        client_state.map_editor_state.polygon_tool_wip_polygon_edge = std::nullopt;
    }
}

void PolygonTool::OnSceneLeftMouseButtonRelease(ClientState& /*client_state*/,
                                                const State& /*game_state*/)
{
}

void PolygonTool::OnSceneRightMouseButtonClick(ClientState& /*client_state*/) {}

void PolygonTool::OnSceneRightMouseButtonRelease() {}

void PolygonTool::OnMouseScreenPositionChange(ClientState& /*client_state*/,
                                              glm::vec2 /*last_mouse_position*/,
                                              glm::vec2 /*new_mouse_position*/)
{
}

void PolygonTool::OnMouseMapPositionChange(ClientState& client_state,
                                           glm::vec2 /*last_mouse_position*/,
                                           glm::vec2 new_mouse_position)
{
    if (client_state.map_editor_state.is_snap_to_grid_enabled) {
        mouse_map_position_ = SnapMousePositionToGrid(
          new_mouse_position, client_state.map_editor_state.grid_interval_division);
    } else {
        mouse_map_position_ = new_mouse_position;
    }

    if (client_state.map_editor_state.polygon_tool_wip_polygon_edge) {
        client_state.map_editor_state.polygon_tool_wip_polygon_edge->vertices.at(1).x =
          mouse_map_position_.x;
        client_state.map_editor_state.polygon_tool_wip_polygon_edge->vertices.at(1).y =
          mouse_map_position_.y;
        client_state.map_editor_state.polygon_tool_wip_polygon_edge->vertices.at(1).texture_s =
          mouse_map_position_.x / client_state.current_polygon_texture_dimensions.x;
        client_state.map_editor_state.polygon_tool_wip_polygon_edge->vertices.at(1).texture_t =
          mouse_map_position_.y / client_state.current_polygon_texture_dimensions.y;
        client_state.map_editor_state.polygon_tool_wip_polygon_edge->vertices.at(1).color =
          GetCurrentPaletteColor(client_state);
    }

    if (client_state.map_editor_state.polygon_tool_wip_polygon) {
        client_state.map_editor_state.polygon_tool_wip_polygon->vertices.at(2).x =
          mouse_map_position_.x;
        client_state.map_editor_state.polygon_tool_wip_polygon->vertices.at(2).y =
          mouse_map_position_.y;
        client_state.map_editor_state.polygon_tool_wip_polygon->vertices.at(2).texture_s =
          mouse_map_position_.x / client_state.current_polygon_texture_dimensions.x;
        client_state.map_editor_state.polygon_tool_wip_polygon->vertices.at(2).texture_t =
          mouse_map_position_.y / client_state.current_polygon_texture_dimensions.y;
        client_state.map_editor_state.polygon_tool_wip_polygon->vertices.at(2).color =
          GetCurrentPaletteColor(client_state);
    }
}

void PolygonTool::OnModifierKey1Pressed() {}

void PolygonTool::OnModifierKey1Released() {}

void PolygonTool::OnModifierKey2Pressed() {}

void PolygonTool::OnModifierKey2Released() {}

void PolygonTool::OnModifierKey3Pressed() {}

void PolygonTool::OnModifierKey3Released() {}

PMSColor PolygonTool::GetCurrentPaletteColor(ClientState& client_state)
{
    return { (unsigned char)(client_state.map_editor_state.palette_current_color.at(0) * 255.0F),
             (unsigned char)(client_state.map_editor_state.palette_current_color.at(1) * 255.0F),
             (unsigned char)(client_state.map_editor_state.palette_current_color.at(2) * 255.0F),
             (unsigned char)(client_state.map_editor_state.palette_current_color.at(3) * 255.0F) };
}
} // namespace Soldank
