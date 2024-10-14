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

    MapEditorUI::Render(game_state, client_state);
}
} // namespace Soldank
