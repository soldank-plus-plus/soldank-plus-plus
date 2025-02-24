#include "rendering/renderer/interface/map_editor/MapEditorScene.hpp"

#include "core/map/PMSConstants.hpp"
#include "core/map/PMSEnums.hpp"
#include "core/map/PMSStructs.hpp"
#include "rendering/renderer/SceneryOutlinesRenderer.hpp"
#include "rendering/renderer/interface/map_editor/MapEditorUI.hpp"
#include "rendering/renderer/interface/map_editor/PolygonVertexOutlinesRenderer.hpp"

namespace Soldank
{
MapEditorScene::MapEditorScene(ClientState& client_state, State& game_state)
    : polygon_vertex_outlines_renderer_(client_state, game_state.map, { 1.0F, 0.0F, 0.0F, 0.5F })
    , scenery_outlines_renderer_(game_state.map, { 1.0F, 0.0F, 0.0F, 0.5F })
{
    client_state.map_editor_state.map_description_input.reserve(DESCRIPTION_MAX_LENGTH);
    client_state.map_editor_state.event_scenery_texture_changed.AddObserver(
      [this](const std::string& file_name) { single_image_renderer_.SetTexture(file_name); });
}

void MapEditorScene::Render(const State& game_state,
                            ClientState& client_state,
                            const PolygonsRenderer& polygons_renderer)
{
    const Camera& camera = client_state.camera_component;
    client_state.map_editor_state.polygon_texture_opengl_id =
      polygons_renderer.GetTextureOpenGLID();

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

    if (client_state.map_editor_state.draw_spawn_points) {
        for (const auto& spawn_point : game_state.map.GetSpawnPoints()) {
            spawn_point_renderer_.Render(camera.GetView(), spawn_point, camera.GetZoom());
        }
    }

    for (const auto& selected_spawn_point_id :
         client_state.map_editor_state.selected_spawn_point_ids) {

        const auto& spawn_point = game_state.map.GetSpawnPoints().at(selected_spawn_point_id);
        glm::vec2 start_position = { (float)spawn_point.x - 9.0F * camera.GetZoom(),
                                     (float)spawn_point.y - 9.0F * camera.GetZoom() };
        glm::vec2 end_position = { (float)spawn_point.x + 9.0F * camera.GetZoom(),
                                   (float)spawn_point.y + 9.0F * camera.GetZoom() };
        RenderRectangle(camera.GetView(),
                        start_position,
                        end_position,
                        { 0.8F, 0.2F, 0.2F, 0.7F },
                        camera.GetZoom());
    }

    for (const auto& selected_soldier_id : client_state.map_editor_state.selected_soldier_ids) {
        for (const auto& soldier : game_state.soldiers) {
            if (soldier.id != selected_soldier_id) {
                continue;
            }

            glm::vec2 start_position = { (float)soldier.particle.position.x - 9.0F,
                                         (float)soldier.particle.position.y - 25.0F };
            glm::vec2 end_position = { (float)soldier.particle.position.x + 9.0F,
                                       (float)soldier.particle.position.y + 5.0F };
            RenderRectangle(camera.GetView(),
                            start_position,
                            end_position,
                            { 0.8F, 0.2F, 0.2F, 0.7F },
                            camera.GetZoom());
        }
    }

    if (client_state.map_editor_state.selected_tool == ToolType::Spawnpoint) {
        PMSSpawnPoint spawn_point{
            .x = (int)client_state.map_editor_state.spawn_point_preview_position.x,
            .y = (int)client_state.map_editor_state.spawn_point_preview_position.y,
            .type = client_state.map_editor_state.selected_spawn_point_type,
        };
        spawn_point_renderer_.Render(camera.GetView(), spawn_point, camera.GetZoom());
    }

    if (client_state.map_editor_state.vertex_selection_box) {
        glm::vec2 start_position = client_state.map_editor_state.vertex_selection_box->first;
        glm::vec2 end_position = client_state.map_editor_state.vertex_selection_box->second;
        glm::vec4 color = { 0.8F, 0.2F, 0.2F, 1.0F };
        float thickness = camera.GetZoom();
        RenderRectangle(camera.GetView(), start_position, end_position, color, thickness);

        if (client_state.map_editor_state.selected_tool == ToolType::Transform) {
            glm::vec2 start_position = client_state.map_editor_state.vertex_selection_box->first;
            glm::vec2 end_position = client_state.map_editor_state.vertex_selection_box->second;
            glm::vec2 mid_position = start_position + end_position;
            mid_position /= 2;
            glm::vec4 color = { 0.8F, 0.7F, 0.7F, 1.0F };

            rectangle_renderer_.Render(camera.GetView(), start_position, color);
            rectangle_renderer_.Render(camera.GetView(), end_position, color);
            rectangle_renderer_.Render(
              camera.GetView(), { start_position.x, end_position.y }, color);
            rectangle_renderer_.Render(
              camera.GetView(), { end_position.x, start_position.y }, color);

            rectangle_renderer_.Render(
              camera.GetView(), { mid_position.x, start_position.y }, color);
            rectangle_renderer_.Render(
              camera.GetView(), { start_position.x, mid_position.y }, color);
            rectangle_renderer_.Render(camera.GetView(), { mid_position.x, end_position.y }, color);
            rectangle_renderer_.Render(camera.GetView(), { end_position.x, mid_position.y }, color);
        }
    }

    if (client_state.map_editor_state.selected_tool == ToolType::Scenery) {
        glm::vec2 position = { client_state.map_editor_state.scenery_to_place.x,
                               client_state.map_editor_state.scenery_to_place.y };
        glm::vec4 color = { client_state.map_editor_state.palette_current_color.at(0),
                            client_state.map_editor_state.palette_current_color.at(1),
                            client_state.map_editor_state.palette_current_color.at(2),
                            client_state.map_editor_state.palette_current_color.at(3) };
        glm::vec2 scale = { client_state.map_editor_state.scenery_to_place.scale_x,
                            client_state.map_editor_state.scenery_to_place.scale_y };
        float rotation = client_state.map_editor_state.scenery_to_place.rotation;
        single_image_renderer_.Render(camera.GetView(), position, color, scale, rotation);
        client_state.map_editor_state.scenery_to_place.width =
          single_image_renderer_.GetTextureDimensions().x;
        client_state.map_editor_state.scenery_to_place.height =
          single_image_renderer_.GetTextureDimensions().y;
    }

    if (client_state.map_editor_state.is_grid_visible) {
        grid_renderer_.Render(
          { client_state.camera_component.GetWidth(), client_state.camera_component.GetHeight() },
          { client_state.camera_component.GetX(), client_state.camera_component.GetY() },
          client_state.camera_component.GetZoom());
    }

    MapEditorUI::Render(game_state, client_state);
}

void MapEditorScene::RenderRectangle(const glm::mat4& transform,
                                     const glm::vec2& left_top_position,
                                     const glm::vec2& right_bottom_position,
                                     const glm::vec4& color,
                                     float thickness)
{
    line_renderer_.Render(transform,
                          left_top_position,
                          { left_top_position.x, right_bottom_position.y },
                          color,
                          thickness);
    line_renderer_.Render(transform,
                          left_top_position,
                          { right_bottom_position.x, left_top_position.y },
                          color,
                          thickness);
    line_renderer_.Render(transform,
                          { left_top_position.x, right_bottom_position.y },
                          right_bottom_position,
                          color,
                          thickness);
    line_renderer_.Render(transform,
                          { right_bottom_position.x, left_top_position.y },
                          right_bottom_position,
                          color,
                          thickness);
}
} // namespace Soldank
