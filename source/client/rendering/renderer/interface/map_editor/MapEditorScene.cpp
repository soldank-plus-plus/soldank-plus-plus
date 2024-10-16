#include "rendering/renderer/interface/map_editor/MapEditorScene.hpp"

#include "rendering/renderer/SceneryOutlinesRenderer.hpp"
#include "rendering/renderer/interface/map_editor/MapEditorUI.hpp"
#include "rendering/renderer/interface/map_editor/PolygonVertexOutlinesRenderer.hpp"

namespace Soldank
{
MapEditorScene::MapEditorScene(ClientState& client_state, State& game_state)
    : polygon_vertex_outlines_renderer_(client_state, game_state.map, { 1.0F, 0.0F, 0.0F, 0.5F })
    , scenery_outlines_renderer_(game_state.map, { 1.0F, 0.0F, 0.0F, 0.5F })
{
}

void MapEditorScene::Render(State& game_state,
                            ClientState& client_state,
                            const PolygonsRenderer& polygons_renderer)
{
    const Camera& camera = client_state.camera_component;

    if (client_state.map_editor_state.polygon_tool_wip_polygon_edge) {
        polygons_renderer.RenderSinglePolygonFirstEdge(
          camera.GetView(), *client_state.map_editor_state.polygon_tool_wip_polygon_edge);
    }

    if (client_state.map_editor_state.polygon_tool_wip_polygon) {
        polygons_renderer.RenderSinglePolygon(
          camera.GetView(), *client_state.map_editor_state.polygon_tool_wip_polygon);
    }

    for (auto selected_scenery_id : client_state.map_editor_state.selected_scenery_ids) {
        scenery_outlines_renderer_.Render(camera.GetView(), selected_scenery_id);
    }

    polygon_vertex_outlines_renderer_.Render(camera.GetView());

    for (const auto& spawn_point : game_state.map.GetSpawnPoints()) {
        spawn_point_renderer_.Render(camera.GetView(), spawn_point, camera.GetZoom());
    }

    if (client_state.map_editor_state.vertex_selection_box) {
        glm::vec2 start_position = client_state.map_editor_state.vertex_selection_box->first;
        glm::vec2 end_position = client_state.map_editor_state.vertex_selection_box->second;
        glm::vec4 color = { 0.8F, 0.2F, 0.2F, 1.0F };
        float thickness = camera.GetZoom();
        line_renderer_.Render(
          camera.GetView(), start_position, { start_position.x, end_position.y }, color, thickness);
        line_renderer_.Render(
          camera.GetView(), start_position, { end_position.x, start_position.y }, color, thickness);
        line_renderer_.Render(
          camera.GetView(), { start_position.x, end_position.y }, end_position, color, thickness);
        line_renderer_.Render(
          camera.GetView(), { end_position.x, start_position.y }, end_position, color, thickness);
    }

    MapEditorUI::Render(game_state, client_state);
}
} // namespace Soldank
