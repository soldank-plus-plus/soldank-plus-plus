#include "Scene.hpp"

#include "core/entities/Item.hpp"
#include "core/types/ItemType.hpp"
#include "rendering/renderer/ItemRenderer.hpp"
#include "rendering/renderer/PolygonsRenderer.hpp"
#include "rendering/renderer/interface/debug/DebugUI.hpp"
#include "rendering/renderer/interface/ImGuiThemes.hpp"

#include "application/input/Mouse.hpp"
#include "application/config/Config.hpp"

#include "core/entities/Bullet.hpp"
#include "core/math/Calc.hpp"

#include <string>
#include <algorithm>
#include <vector>
#include <format>

namespace Soldank
{
Scene::Scene(const std::shared_ptr<StateManager>& game_state, ClientState& client_state)
    : background_renderer_(game_state->GetState().map.GetBackgroundTopColor(),
                           game_state->GetState().map.GetBackgroundBottomColor(),
                           game_state->GetState().map.GetBoundaries())
    , polygons_renderer_(std::make_unique<PolygonsRenderer>(
        game_state->GetState().map,
        std::string(game_state->GetState().map.GetTextureName())))
    , polygon_outlines_renderer_(game_state->GetState().map, { 1.0F, 1.0F, 1.0F, 1.0F })
    , sceneries_renderer_(game_state->GetState().map.GetSceneryTypes(),
                          game_state->GetState().map.GetSceneryInstances())
    , soldier_renderer_(sprite_manager_)
    , cursor_renderer_(client_state)
    , text_renderer_("play-regular.ttf", 48)
    , bullet_renderer_(sprite_manager_)
    , item_renderer_(sprite_manager_)
    , map_editor_scene_(client_state, game_state->GetState())
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    SetupImGuiTheme();
}

void Scene::Render(State& game_state, ClientState& client_state, double frame_percent, int fps)
{
    // TODO: handle it better, this is not a good place for this to be
    client_state.current_polygon_texture_dimensions = polygons_renderer_->GetTextureDimensions();

    glm::vec2 new_camera_position =
      Calc::Lerp(client_state.camera_prev, client_state.camera, (float)frame_percent);
    Camera& camera = client_state.camera_component;
    camera.Move(new_camera_position.x, new_camera_position.y);

    glClearColor(168.0F / 255.0F, 163.0F / 255.0F, 148.0F / 255.0F, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);

    if (client_state.draw_background) {
        background_renderer_.Render(camera.GetView());
    }

    if (client_state.draw_sceneries) {
        sceneries_renderer_.Render(camera.GetView(), 0, game_state.map.GetSceneryInstances());
    }

    if (client_state.draw_server_pov_client_pos) {
        rectangle_renderer_.Render(camera.GetView(), client_state.soldier_position_server_pov);
    }
    for (const Bullet& bullet : game_state.bullets) {
        bullet_renderer_.Render(camera.GetView(), bullet, frame_percent);
    }
    RenderSoldiers(game_state, client_state, frame_percent);
    for (const Item& item : game_state.items) {
        item_renderer_.Render(camera.GetView(), item, frame_percent, game_state.game_tick);
    }
    if (client_state.draw_sceneries) {
        sceneries_renderer_.Render(camera.GetView(), 1, game_state.map.GetSceneryInstances());
    }
    if (client_state.draw_polygons) {
        polygons_renderer_->Render(camera.GetView());
    }
    if (client_state.draw_colliding_polygons) {
        for (unsigned int polygon_id : client_state.colliding_polygon_ids) {
            polygon_outlines_renderer_.Render(camera.GetView(), polygon_id);
        }
    }
    if (client_state.draw_sceneries) {
        sceneries_renderer_.Render(camera.GetView(), 2, game_state.map.GetSceneryInstances());
    }

    if (client_state.draw_soldier_hitboxes) {
        const auto bullet_colliding_body_parts = std::array{ 12, 11, 10, 6, 5, 4, 3 };
        for (const auto& soldier : game_state.soldiers) {
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
        }
    }

    if (client_state.draw_item_hitboxes) {
        for (const Item& item : game_state.items) {
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
        }
    }

    if (client_state.draw_bullet_hitboxes) {
        for (const auto& bullet : game_state.bullets) {
            auto start_point = bullet.particle.position;
            auto end_point = bullet.particle.position + bullet.particle.GetVelocity();

            line_renderer_.Render(
              camera.GetView(), start_point, end_point, { 1.0F, 0.0F, 0.0F, 1.0F }, 0.5F);
        }
    }

    if (client_state.draw_game_debug_interface) {
        DebugUI::Render(game_state, client_state, frame_percent, fps);
        if (!DebugUI::GetWantCaptureMouse()) {
            cursor_renderer_.Render({ client_state.mouse.x, client_state.mouse.y },
                                    { client_state.window_width, client_state.window_height });
        }
    }

    if (client_state.draw_map_editor_interface) {
        map_editor_scene_.Render(game_state, client_state, *polygons_renderer_);
    }

    if (client_state.draw_game_interface) {
        for (const auto& soldier : game_state.soldiers) {
            if (client_state.client_soldier_id.has_value() &&
                *client_state.client_soldier_id == soldier.id) {
                text_renderer_.Render("Health: " + std::to_string((int)soldier.health),
                                      50.0,
                                      100.0,
                                      1.0,
                                      { 1.0, 1.0, 1.0 },
                                      { client_state.window_width, client_state.window_height });
                text_renderer_.Render("Jets: " + std::to_string((int)soldier.jets_count),
                                      50.0,
                                      50.0,
                                      1.0,
                                      { 1.0, 1.0, 1.0 },
                                      { client_state.window_width, client_state.window_height });
            }
        }

        if (client_state.client_soldier_id.has_value()) {
            for (const auto& soldier : game_state.soldiers) {
                if (*client_state.client_soldier_id == soldier.id) {
                    if (soldier.dead_meat) {
                        text_renderer_.Render(
                          "Respawn timer: " +
                            std::format("{:.2f}", (float)soldier.ticks_to_respawn / 60.0F),
                          400.0,
                          100.0,
                          1.0,
                          { 1.0, 1.0, 1.0 },
                          { client_state.window_width, client_state.window_height });
                    }
                }
            }
        }

        if (game_state.paused) {
            text_renderer_.Render("Game paused",
                                  400.0,
                                  700.0,
                                  1.0,
                                  { 0.6, 0.7, 0.4 },
                                  { client_state.window_width, client_state.window_height });
        }
    }

    if (Config::DEBUG_DRAW) {
        for (const auto& soldier : game_state.soldiers) {
            rectangle_renderer_.Render(
              camera.GetView(),
              glm::vec2(soldier.control.mouse_aim_x, soldier.control.mouse_aim_y));
        }
    }
}

void Scene::RenderSoldiers(const State& game_state,
                           const ClientState& client_state,
                           double frame_percent)
{
    const Camera& camera = client_state.camera_component;

    for (const auto& soldier : game_state.soldiers) {
        if (client_state.client_soldier_id.has_value() &&
            *client_state.client_soldier_id == soldier.id) {

            // skip rendering player's soldier sprites now because we will render it later
            continue;
        }
        if (soldier.active) {
            soldier_renderer_.Render(camera.GetView(), sprite_manager_, soldier, frame_percent);
        }
    }

    // Render player's soldier last because it's the most important for the player to see their
    // soldier on top of others
    if (client_state.client_soldier_id.has_value()) {
        unsigned int client_soldier_id = *client_state.client_soldier_id;
        for (const auto& soldier : game_state.soldiers) {
            if (soldier.id == client_soldier_id && soldier.active) {
                soldier_renderer_.Render(camera.GetView(), sprite_manager_, soldier, frame_percent);
            }
        }
    }
}
} // namespace Soldank
