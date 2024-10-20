#ifndef __MAP_EDITOR_SCENE_HPP__
#define __MAP_EDITOR_SCENE_HPP__

#include "rendering/ClientState.hpp"
#include "rendering/renderer/LineRenderer.hpp"
#include "rendering/renderer/PolygonsRenderer.hpp"
#include "rendering/renderer/SceneryOutlinesRenderer.hpp"

#include "core/state/StateManager.hpp"
#include "rendering/renderer/interface/map_editor/PolygonVertexOutlinesRenderer.hpp"
#include "rendering/renderer/interface/map_editor/SingleImageRenderer.hpp"
#include "rendering/renderer/interface/map_editor/SpawnPointRenderer.hpp"

namespace Soldank
{
class MapEditorScene
{
public:
    MapEditorScene(ClientState& client_state, State& game_state);

    void Render(State& game_state,
                ClientState& client_state,
                const PolygonsRenderer& polygons_renderer);

private:
    void RenderRectangle(const glm::mat4& transform,
                         const glm::vec2& left_top_position,
                         const glm::vec2& right_bottom_position,
                         const glm::vec4& color,
                         float thickness);

    LineRenderer line_renderer_;
    PolygonVertexOutlinesRenderer polygon_vertex_outlines_renderer_;
    SceneryOutlinesRenderer scenery_outlines_renderer_;
    SpawnPointRenderer spawn_point_renderer_;
    SingleImageRenderer single_image_renderer_;
};
} // namespace Soldank

#endif
