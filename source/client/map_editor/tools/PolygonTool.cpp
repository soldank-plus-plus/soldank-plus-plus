#include "map_editor/tools/PolygonTool.hpp"

#include "core/map/PMSEnums.hpp"
#include "core/map/PMSStructs.hpp"
#include "rendering/renderer/interface/map_editor/MapEditorState.hpp"

#include "spdlog/spdlog.h"

namespace Soldank
{
PolygonTool::PolygonTool(Map& map)
    : map_(map)
    , mouse_map_position_()
{
}

void PolygonTool::OnSelect() {}

void PolygonTool::OnUnselect() {}

void PolygonTool::OnSceneLeftMouseButtonClick(ClientState& client_state)
{
    if (client_state.map_editor_state.polygon_tool_wip_polygon) {
        map_.AddNewPolygon(*client_state.map_editor_state.polygon_tool_wip_polygon);
        client_state.map_editor_state.polygon_tool_wip_polygon = std::nullopt;
    } else if (!client_state.map_editor_state.polygon_tool_wip_polygon_edge) {
        client_state.map_editor_state.polygon_tool_wip_polygon_edge = PMSPolygon{};
        client_state.map_editor_state.polygon_tool_wip_polygon_edge->id = 0;
        client_state.map_editor_state.polygon_tool_wip_polygon_edge->polygon_type =
          PMSPolygonType::Normal;
        client_state.map_editor_state.polygon_tool_wip_polygon_edge->vertices.at(
          0) = { .x = mouse_map_position_.x,
                 .y = mouse_map_position_.y,
                 .z = 1,
                 .rhw = 1.0F,
                 .color = PMSColor(255, 255, 255, 255),
                 .texture_s = mouse_map_position_.x / map_.GetTextureDimensions().x,
                 .texture_t = mouse_map_position_.y / map_.GetTextureDimensions().y };
        client_state.map_editor_state.polygon_tool_wip_polygon_edge->vertices.at(
          1) = { .x = mouse_map_position_.x,
                 .y = mouse_map_position_.y,
                 .z = 1,
                 .rhw = 1.0F,
                 .color = PMSColor(255, 255, 255, 255),
                 .texture_s = mouse_map_position_.x / map_.GetTextureDimensions().x,
                 .texture_t = mouse_map_position_.y / map_.GetTextureDimensions().y };
    } else if (!client_state.map_editor_state.polygon_tool_wip_polygon) {
        client_state.map_editor_state.polygon_tool_wip_polygon =
          *client_state.map_editor_state.polygon_tool_wip_polygon_edge;
        client_state.map_editor_state.polygon_tool_wip_polygon->vertices.at(
          2) = { .x = mouse_map_position_.x,
                 .y = mouse_map_position_.y,
                 .z = 1,
                 .rhw = 1.0F,
                 .color = PMSColor(255, 255, 255, 255),
                 .texture_s = mouse_map_position_.x / map_.GetTextureDimensions().x,
                 .texture_t = mouse_map_position_.y / map_.GetTextureDimensions().y };

        client_state.map_editor_state.polygon_tool_wip_polygon_edge = std::nullopt;
    }
}

void PolygonTool::OnSceneLeftMouseButtonRelease() {}

void PolygonTool::OnSceneRightMouseButtonClick() {}

void PolygonTool::OnSceneRightMouseButtonRelease() {}

void PolygonTool::OnMouseScreenPositionChange(ClientState& client_state,
                                              glm::vec2 last_mouse_position,
                                              glm::vec2 new_mouse_position)
{
}

void PolygonTool::OnMouseMapPositionChange(ClientState& client_state,
                                           glm::vec2 /*last_mouse_position*/,
                                           glm::vec2 new_mouse_position)
{
    mouse_map_position_ = new_mouse_position;

    if (client_state.map_editor_state.polygon_tool_wip_polygon_edge) {
        client_state.map_editor_state.polygon_tool_wip_polygon_edge->vertices.at(1).x =
          new_mouse_position.x;
        client_state.map_editor_state.polygon_tool_wip_polygon_edge->vertices.at(1).y =
          new_mouse_position.y;
        client_state.map_editor_state.polygon_tool_wip_polygon_edge->vertices.at(1).texture_s =
          new_mouse_position.x / map_.GetTextureDimensions().x;
        client_state.map_editor_state.polygon_tool_wip_polygon_edge->vertices.at(1).texture_t =
          new_mouse_position.y / map_.GetTextureDimensions().y;
    }

    if (client_state.map_editor_state.polygon_tool_wip_polygon) {
        client_state.map_editor_state.polygon_tool_wip_polygon->vertices.at(2).x =
          new_mouse_position.x;
        client_state.map_editor_state.polygon_tool_wip_polygon->vertices.at(2).y =
          new_mouse_position.y;
        client_state.map_editor_state.polygon_tool_wip_polygon->vertices.at(2).texture_s =
          new_mouse_position.x / map_.GetTextureDimensions().x;
        client_state.map_editor_state.polygon_tool_wip_polygon->vertices.at(2).texture_t =
          new_mouse_position.y / map_.GetTextureDimensions().y;
    }
}
} // namespace Soldank
