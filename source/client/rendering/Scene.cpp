module;

#include "application/config/Config.hpp"
#include <glad/glad.h>

#include <array>
#include <string>
#include <algorithm>
#include <vector>
#include <memory>

export module Scene;

import Extern.Glm;

import Camera;
import ItemRenderer;
import PolygonsRenderer;
import DebugUI;
import ImGuiThemes;
import ClientState;
import SpritesManager;
import BackgroundRenderer;
import PolygonOutlinesRenderer;
import SceneriesRenderer;
import SoldierRenderer;
import CursorRenderer;
import TextRenderer;
import GameHudRenderer;
import RectangleRenderer;
import BulletRenderer;
import LineRenderer;
import CircleRenderer;
import MapEditorScene;
import MapEditorUI;

import Shared.Core.State.StateManager;
import Shared.Core.Entities.Item;
import Shared.Core.Types.ItemType;
import Shared.Core.Entities.Soldier;
import Shared.Core.Entities.Bullet;
import Shared.Core.Math.Calc;

export namespace Soldank
{
class Scene
{
public:
    Scene(const std::shared_ptr<StateManager>& game_state, ClientState& client_state);
    ~Scene();

    void Render(const StateManager& game_state_manager,
                ClientState& client_state,
                double frame_percent,
                int fps);
    void RenderGame(const StateManager& game_state_manager,
                    ClientState& client_state,
                    double frame_percent,
                    int fps);
    void RenderEditor(const StateManager& game_state_manager,
                      ClientState& client_state,
                      double frame_percent,
                      int fps);
    void RenderPlayTest(const StateManager& game_state_manager,
                        ClientState& client_state,
                        double frame_percent,
                        int fps);

    void RenderSoldiers(const StateManager& game_state_manager,
                        const ClientState& client_state,
                        double frame_percent);

    glm::vec2 GetTextureDimensions() const { return polygons_renderer_->GetTextureDimensions(); }

    static glm::vec4 GetPixelColor(const glm::vec2& position);

private:
    void RenderWorld(const StateManager& game_state_manager,
                     ClientState& client_state,
                     double frame_percent);
    void RenderDebugOverlay(const StateManager& game_state_manager,
                            ClientState& client_state,
                            double frame_percent,
                            int fps);
    void RenderEditorOverlay(const StateManager& game_state_manager, ClientState& client_state);
    void RenderDebugMouseAim(const StateManager& game_state_manager,
                             const ClientState& client_state);

    Sprites::SpriteManager sprite_manager_;
    BackgroundRenderer background_renderer_;
    std::unique_ptr<PolygonsRenderer> polygons_renderer_;
    PolygonOutlinesRenderer polygon_outlines_renderer_;
    SceneriesRenderer sceneries_renderer_;
    SoldierRenderer soldier_renderer_;
    CursorRenderer cursor_renderer_;
    GameHudRenderer game_hud_renderer_;
    RectangleRenderer rectangle_renderer_;
    BulletRenderer bullet_renderer_;
    LineRenderer line_renderer_;
    CircleRenderer circle_renderer_;
    ItemRenderer item_renderer_;
    MapEditorScene map_editor_scene_;
};
} // namespace Soldank

namespace Soldank
{
Scene::Scene(const std::shared_ptr<StateManager>& game_state, ClientState& client_state)
    : background_renderer_(game_state->GetMap())
    , polygons_renderer_(
        std::make_unique<PolygonsRenderer>(game_state->GetMap(),
                                           std::string(game_state->GetMap().GetTextureName())))
    , polygon_outlines_renderer_(game_state->GetMap(), { 1.0F, 1.0F, 1.0F, 1.0F })
    , sceneries_renderer_(game_state->GetMap())
    , soldier_renderer_(sprite_manager_)
    , cursor_renderer_(client_state)
    , bullet_renderer_(sprite_manager_)
    , item_renderer_(sprite_manager_)
    , map_editor_scene_(client_state, *game_state)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    SetupImGuiTheme();
}

Scene::~Scene() {}

void Scene::Render(const StateManager& game_state_manager,
                   ClientState& client_state,
                   double frame_percent,
                   int fps)
{
    RenderGame(game_state_manager, client_state, frame_percent, fps);
}

void Scene::RenderGame(const StateManager& game_state_manager,
                       ClientState& client_state,
                       double frame_percent,
                       int fps)
{
    RenderWorld(game_state_manager, client_state, frame_percent);
    RenderDebugOverlay(game_state_manager, client_state, frame_percent, fps);
    game_hud_renderer_.Render(game_state_manager, client_state);
    RenderDebugMouseAim(game_state_manager, client_state);
    MapEditorUI::RenderPlayTestEscapeMenu(client_state);
}

void Scene::RenderEditor(const StateManager& game_state_manager,
                         ClientState& client_state,
                         double frame_percent,
                         int /*fps*/)
{
    RenderWorld(game_state_manager, client_state, frame_percent);
    RenderEditorOverlay(game_state_manager, client_state);
    RenderDebugMouseAim(game_state_manager, client_state);
}

void Scene::RenderPlayTest(const StateManager& game_state_manager,
                           ClientState& client_state,
                           double frame_percent,
                           int fps)
{
    RenderGame(game_state_manager, client_state, frame_percent, fps);
}

void Scene::RenderWorld(const StateManager& game_state_manager,
                        ClientState& client_state,
                        double frame_percent)
{
    // TODO: handle it better, this is not a good place for this to be
    client_state.world_render_options.current_polygon_texture_dimensions =
      polygons_renderer_->GetTextureDimensions();

    glm::vec2 new_camera_position = Calc::Lerp(
      client_state.camera.previous_position, client_state.camera.position, (float)frame_percent);
    Camera& camera = client_state.camera.view;
    camera.Move(new_camera_position.x, new_camera_position.y);

    glViewport(0, 0, (int)client_state.input.window_width, (int)client_state.input.window_height);
    glClearColor(168.0F / 255.0F, 163.0F / 255.0F, 148.0F / 255.0F, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);

    if (client_state.world_render_options.draw_background) {
        background_renderer_.Render(camera.GetView());
    }

    if (client_state.world_render_options.draw_sceneries) {
        sceneries_renderer_.Render(
          camera.GetView(), 0, game_state_manager.GetConstMap().GetSceneryInstances());
    }

    if (client_state.network.draw_server_pov_client_pos) {
        rectangle_renderer_.Render(camera.GetView(),
                                   client_state.network.soldier_position_server_pov,
                                   { 1.0F, 0.0F, 0.0F, 1.0F });
    }
    game_state_manager.ForEachBullet([&](const auto& bullet) {
        bullet_renderer_.Render(camera.GetView(), bullet, frame_percent);
    });
    RenderSoldiers(game_state_manager, client_state, frame_percent);
    game_state_manager.ForEachItem([&](const auto& item) {
        item_renderer_.Render(
          camera.GetView(), item, frame_percent, game_state_manager.GetGameTick());
    });
    if (client_state.world_render_options.draw_sceneries) {
        sceneries_renderer_.Render(
          camera.GetView(), 1, game_state_manager.GetConstMap().GetSceneryInstances());
    }
    if (client_state.world_render_options.draw_polygons) {
        polygons_renderer_->Render(camera.GetView());
    }
    if (client_state.debug_render.draw_colliding_polygons) {
        for (unsigned int polygon_id : client_state.debug_render.colliding_polygon_ids) {
            polygon_outlines_renderer_.Render(camera.GetView(), polygon_id);
        }
    }
    if (client_state.world_render_options.draw_sceneries) {
        sceneries_renderer_.Render(
          camera.GetView(), 2, game_state_manager.GetConstMap().GetSceneryInstances());
    }

    if (client_state.debug_render.draw_soldier_hitboxes) {
        const auto bullet_colliding_body_parts = std::array{ 12, 11, 10, 6, 5, 4, 3 };
        game_state_manager.ForEachSoldier([&](const auto& soldier) {
            glm::vec2 soldier_pos = soldier.particle.position;
            for (auto body_part_id : bullet_colliding_body_parts) {
                auto body_part_offset =
                  soldier.skeleton->GetPos(body_part_id) - soldier.particle.position;
                auto body_part_center_position = soldier_pos + body_part_offset;
                float radius = 7.0F;
                circle_renderer_.Render(camera.GetView(),
                                        body_part_center_position,
                                        { 1.0F, 0.0F, 0.0F, 1.0F },
                                        radius,
                                        radius - 0.5F);
            }
        });
    }

    if (client_state.debug_render.draw_item_hitboxes) {
        game_state_manager.ForEachItem([&](const auto& item) {
            if (IsItemTypeWeapon(item.style)) {
                glm::vec2 start_pos = item.skeleton->GetPos(1);
                glm::vec2 end_pos = item.skeleton->GetPos(2);
                float radius = 1.0F;
                circle_renderer_.Render(
                  camera.GetView(), start_pos, { 1.0F, 0.0F, 0.0F, 1.0F }, radius);
                circle_renderer_.Render(
                  camera.GetView(), end_pos, { 1.0F, 0.0F, 0.0F, 1.0F }, radius);
                line_renderer_.Render(
                  camera.GetView(), start_pos, end_pos, { 1.0F, 0.0F, 0.0F, 1.0F }, 0.5F);
            } else {
                for (unsigned int i = 1; i <= 4; ++i) {
                    glm::vec2 start_pos = item.skeleton->GetPos(i);
                    glm::vec2 end_pos = item.skeleton->GetPos((i % 4) + 1);
                    float radius = 1.0F;
                    circle_renderer_.Render(
                      camera.GetView(), start_pos, { 1.0F, 0.0F, 0.0F, 1.0F }, radius);
                    line_renderer_.Render(
                      camera.GetView(), start_pos, end_pos, { 1.0F, 0.0F, 0.0F, 1.0F }, 0.5F);
                }
            }
        });
    }

    if (client_state.debug_render.draw_bullet_hitboxes) {
        game_state_manager.ForEachBullet([&](const auto& bullet) {
            auto start_point = bullet.particle.position;
            auto end_point = bullet.particle.position + bullet.particle.GetVelocity();

            line_renderer_.Render(
              camera.GetView(), start_point, end_point, { 1.0F, 0.0F, 0.0F, 1.0F }, 0.5F);
        });
    }

    if (client_state.debug_render.draw_sectors) {
        int sectors_count = 2 * game_state_manager.GetConstMap().GetSectorsCount() + 1;
        for (int x = 0; x < sectors_count; ++x) {
            for (int y = 0; y < sectors_count; ++y) {
                const auto& boundaries =
                  game_state_manager.GetConstMap().GetSector(x, y).boundaries;

                float top = boundaries[0];
                float bottom = boundaries[1];
                float left = boundaries[2];
                float right = boundaries[3];

                glm::vec4 color = { 0.7F, 0.1F, 0.0F, 1.0F };
                float thickness = camera.GetZoom();

                line_renderer_.Render(
                  camera.GetView(), { left, top }, { right, top }, color, thickness);
                line_renderer_.Render(
                  camera.GetView(), { right, top }, { right, bottom }, color, thickness);
                line_renderer_.Render(
                  camera.GetView(), { left, bottom }, { right, bottom }, color, thickness);
                line_renderer_.Render(
                  camera.GetView(), { left, bottom }, { left, top }, color, thickness);
            }
        }
    }

    if (client_state.debug_render.draw_map_boundaries) {
        auto boundaries = game_state_manager.GetConstMap().GetBoundaries();

        float top = boundaries[0];
        float bottom = boundaries[1];
        float left = boundaries[2];
        float right = boundaries[3];

        glm::vec4 color = { 0.5F, 0.0F, 0.0F, 1.0F };
        float thickness = camera.GetZoom();

        line_renderer_.Render(camera.GetView(), { left, top }, { right, top }, color, thickness);
        line_renderer_.Render(
          camera.GetView(), { right, top }, { right, bottom }, color, thickness);
        line_renderer_.Render(
          camera.GetView(), { left, bottom }, { right, bottom }, color, thickness);
        line_renderer_.Render(camera.GetView(), { left, bottom }, { left, top }, color, thickness);
    }
}

void Scene::RenderDebugOverlay(const StateManager& game_state_manager,
                               ClientState& client_state,
                               double frame_percent,
                               int fps)
{
    if (client_state.debug_render.is_game_debug_interface_enabled) {
        DebugUI::Render(game_state_manager, client_state, frame_percent, fps);
    }
    if (!DebugUI::GetWantCaptureMouse()) {
        cursor_renderer_.Render(
          { client_state.input.mouse_screen_position.x,
            client_state.input.mouse_screen_position.y },
          { client_state.input.window_width, client_state.input.window_height });
    }
}

void Scene::RenderEditorOverlay(const StateManager& game_state_manager, ClientState& client_state)
{
    map_editor_scene_.Render(game_state_manager, client_state, *polygons_renderer_);
}

void Scene::RenderDebugMouseAim(const StateManager& game_state_manager,
                                const ClientState& client_state)
{
    if (Config::DEBUG_DRAW) {
        game_state_manager.ForEachSoldier([&](const auto& soldier) {
            rectangle_renderer_.Render(
              client_state.camera.view.GetView(),
              glm::vec2(soldier.control.mouse_aim_x, soldier.control.mouse_aim_y),
              { 1.0F, 0.0F, 0.0F, 1.0F });
        });
    }
}

glm::vec4 Scene::GetPixelColor(const glm::vec2& position)
{
    std::array<unsigned char, 3> pixel{};
    glReadPixels((int)position.x, (int)position.y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel.data());
    glm::vec4 normalized_color;
    normalized_color.x = (float)pixel.at(0) / 255.0F;
    normalized_color.y = (float)pixel.at(1) / 255.0F;
    normalized_color.z = (float)pixel.at(2) / 255.0F;
    normalized_color.w = 1.0F;
    return normalized_color;
}

void Scene::RenderSoldiers(const StateManager& game_state_manager,
                           const ClientState& client_state,
                           double frame_percent)
{
    const Camera& camera = client_state.camera.view;

    game_state_manager.ForEachSoldier([&](const auto& soldier) {
        // TODO: implement different method to execute the lambda for one soldier
        if (client_state.client_soldier_id.has_value() &&
            *client_state.client_soldier_id == soldier.id) {

            // skip rendering player's soldier sprites now because we will render it later
            return;
        }

        soldier_renderer_.Render(camera.GetView(), sprite_manager_, soldier, frame_percent);
    });

    // Render player's soldier last because it's the most important for the player to see their
    // soldier on top of others
    if (client_state.client_soldier_id.has_value()) {
        unsigned int client_soldier_id = *client_state.client_soldier_id;
        game_state_manager.ForSoldier(client_soldier_id, [&](const auto& soldier) {
            if (soldier.active) {
                soldier_renderer_.Render(camera.GetView(), sprite_manager_, soldier, frame_percent);
            }
        });
    }
}
} // namespace Soldank
