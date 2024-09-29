#include "rendering/renderer/interface/map_editor/MapEditorScene.hpp"

#include "rendering/renderer/interface/map_editor/MapEditorUI.hpp"

namespace Soldank
{
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

    MapEditorUI::Render(game_state, client_state);
}
} // namespace Soldank
