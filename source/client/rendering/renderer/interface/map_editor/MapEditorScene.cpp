#include "rendering/renderer/interface/map_editor/MapEditorScene.hpp"

#include "rendering/renderer/SceneryOutlinesRenderer.hpp"
#include "rendering/renderer/interface/map_editor/MapEditorUI.hpp"

namespace Soldank
{
MapEditorScene::MapEditorScene(State& game_state)
    : polygon_outlines_renderer_(game_state.map, { 1.0F, 0.0F, 0.0F, 1.0F })
    , scenery_outlines_renderer_(game_state.map, { 1.0F, 0.0F, 0.0F, 1.0F })
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

    for (auto selected_polygon_id : client_state.map_editor_state.selected_polygon_ids) {
        polygon_outlines_renderer_.Render(camera.GetView(), selected_polygon_id);
    }

    MapEditorUI::Render(game_state, client_state);
}
} // namespace Soldank
