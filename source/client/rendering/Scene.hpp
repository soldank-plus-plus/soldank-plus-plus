#ifndef __SCENE_HPP__
#define __SCENE_HPP__

#include "rendering/ClientState.hpp"
#include "rendering/components/Camera.hpp"
#include "rendering/data/sprites/SpritesManager.hpp"
#include "rendering/renderer/BackgroundRenderer.hpp"
#include "rendering/renderer/PolygonsRenderer.hpp"
#include "rendering/renderer/PolygonOutlinesRenderer.hpp"
#include "rendering/renderer/SceneriesRenderer.hpp"
#include "rendering/renderer/SoldierRenderer.hpp"
#include "rendering/renderer/interface/CursorRenderer.hpp"
#include "rendering/renderer/TextRenderer.hpp"
#include "rendering/renderer/RectangleRenderer.hpp"
#include "rendering/renderer/BulletRenderer.hpp"
#include "rendering/renderer/LineRenderer.hpp"
#include "rendering/renderer/CircleRenderer.hpp"
#include "rendering/renderer/ItemRenderer.hpp"
#include "rendering/renderer/interface/map_editor/MapEditorScene.hpp"

#include "core/state/StateManager.hpp"
#include "core/entities/Soldier.hpp"

#include <memory>

namespace Soldank
{
class Scene
{
public:
    Scene(const std::shared_ptr<StateManager>& game_state, ClientState& client_state);

    void Render(const State& game_state, ClientState& client_state, double frame_percent, int fps);

    void RenderSoldiers(const State& game_state,
                        const ClientState& client_state,
                        double frame_percent);

    glm::vec2 GetTextureDimensions() const { return polygons_renderer_->GetTextureDimensions(); }

    static glm::vec4 GetPixelColor(const glm::vec2& position);

private:
    Sprites::SpriteManager sprite_manager_;
    BackgroundRenderer background_renderer_;
    std::unique_ptr<PolygonsRenderer> polygons_renderer_;
    PolygonOutlinesRenderer polygon_outlines_renderer_;
    SceneriesRenderer sceneries_renderer_;
    SoldierRenderer soldier_renderer_;
    CursorRenderer cursor_renderer_;
    TextRenderer text_renderer_;
    RectangleRenderer rectangle_renderer_;
    BulletRenderer bullet_renderer_;
    LineRenderer line_renderer_;
    CircleRenderer circle_renderer_;
    ItemRenderer item_renderer_;
    MapEditorScene map_editor_scene_;
};
} // namespace Soldank

#endif
