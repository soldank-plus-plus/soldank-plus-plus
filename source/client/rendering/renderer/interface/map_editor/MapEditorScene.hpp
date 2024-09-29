#ifndef __MAP_EDITOR_SCENE_HPP__
#define __MAP_EDITOR_SCENE_HPP__

#include "rendering/ClientState.hpp"
#include "rendering/renderer/LineRenderer.hpp"
#include "rendering/renderer/PolygonsRenderer.hpp"

#include "core/state/StateManager.hpp"

namespace Soldank
{
class MapEditorScene
{
public:
    void Render(State& game_state,
                ClientState& client_state,
                const PolygonsRenderer& polygons_renderer);

private:
    LineRenderer line_renderer_;
};
} // namespace Soldank

#endif
