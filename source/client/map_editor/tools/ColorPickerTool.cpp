#include "map_editor/tools/ColorPickerTool.hpp"

#include "core/math/Calc.hpp"

namespace Soldank
{
void ColorPickerTool::OnSelect(ClientState& client_state, const State& /*game_state*/)
{
    SetColorPickerMode(ColorPickerMode::ClosestObject, client_state);
}

void ColorPickerTool::OnUnselect(ClientState& /*client_state*/) {}

void ColorPickerTool::OnSceneLeftMouseButtonClick(ClientState& /*client_state*/,
                                                  const State& /*game_state*/)
{
}

void ColorPickerTool::OnSceneLeftMouseButtonRelease(ClientState& client_state,
                                                    const State& game_state)
{
    float max_square_distance = 40.0F * 40.0F;

    switch (color_picker_mode_) {
        case ColorPickerMode::ClosestObject: {
            for (int i = game_state.map.GetPolygonsCount() - 1; i >= 0; --i) {
                const auto& polygon = game_state.map.GetPolygons().at(i);

                if (Map::PointInPoly(client_state.mouse_map_position, polygon)) {
                    for (const auto& vertex : polygon.vertices) {
                        if (Calc::SquareDistance(client_state.mouse_map_position,
                                                 { vertex.x, vertex.y }) <= max_square_distance) {

                            client_state.map_editor_state.palette_current_color.at(0) =
                              (float)vertex.color.red / 255.0F;
                            client_state.map_editor_state.palette_current_color.at(1) =
                              (float)vertex.color.green / 255.0F;
                            client_state.map_editor_state.palette_current_color.at(2) =
                              (float)vertex.color.blue / 255.0F;
                            client_state.map_editor_state.palette_current_color.at(3) =
                              (float)vertex.color.alpha / 255.0F;
                            return;
                        }
                    }
                }
            }

            for (int i = game_state.map.GetSceneryInstances().size() - 1; i >= 0; --i) {
                const auto& scenery = game_state.map.GetSceneryInstances().at(i);

                if (Map::PointInScenery(client_state.mouse_map_position, scenery)) {
                    client_state.map_editor_state.palette_current_color.at(0) =
                      (float)scenery.color.red / 255.0F;
                    client_state.map_editor_state.palette_current_color.at(1) =
                      (float)scenery.color.green / 255.0F;
                    client_state.map_editor_state.palette_current_color.at(2) =
                      (float)scenery.color.blue / 255.0F;
                    client_state.map_editor_state.palette_current_color.at(3) =
                      (float)scenery.color.alpha / 255.0F;
                    return;
                }
            }
            return;
        }
        case ColorPickerMode::Pixel: {
            client_state.map_editor_state.event_pixel_color_under_cursor_requested.Notify();
            client_state.map_editor_state.palette_current_color.at(0) =
              client_state.map_editor_state.last_requested_pixel_color.x;
            client_state.map_editor_state.palette_current_color.at(1) =
              client_state.map_editor_state.last_requested_pixel_color.y;
            client_state.map_editor_state.palette_current_color.at(2) =
              client_state.map_editor_state.last_requested_pixel_color.z;
            client_state.map_editor_state.palette_current_color.at(3) =
              client_state.map_editor_state.last_requested_pixel_color.w;
            return;
        }
    }
}

void ColorPickerTool::OnSceneRightMouseButtonClick(ClientState& /*client_state*/) {}

void ColorPickerTool::OnSceneRightMouseButtonRelease() {}

void ColorPickerTool::OnMouseScreenPositionChange(ClientState& /*client_state*/,
                                                  glm::vec2 /*last_mouse_position*/,
                                                  glm::vec2 /*new_mouse_position*/)
{
}

void ColorPickerTool::OnMouseMapPositionChange(ClientState& /*client_state*/,
                                               glm::vec2 /*last_mouse_position*/,
                                               glm::vec2 /*new_mouse_position*/,
                                               const State& /*game_state*/)
{
}

void ColorPickerTool::OnModifierKey1Pressed(ClientState& client_state)
{
    SetColorPickerMode(ColorPickerMode::Pixel, client_state);
}

void ColorPickerTool::OnModifierKey1Released(ClientState& client_state)
{
    SetColorPickerMode(ColorPickerMode::ClosestObject, client_state);
}

void ColorPickerTool::OnModifierKey2Pressed(ClientState& client_state) {}

void ColorPickerTool::OnModifierKey2Released(ClientState& client_state) {}

void ColorPickerTool::OnModifierKey3Pressed(ClientState& client_state) {}

void ColorPickerTool::OnModifierKey3Released(ClientState& client_state) {}

void ColorPickerTool::SetColorPickerMode(ColorPickerMode new_color_picker_mode,
                                         ClientState& client_state)
{
    color_picker_mode_ = new_color_picker_mode;

    switch (new_color_picker_mode) {
        case ColorPickerMode::ClosestObject: {
            client_state.map_editor_state.current_tool_action_description =
              "Pick a vertex or scenery color";
            break;
        }
        case ColorPickerMode::Pixel: {
            client_state.map_editor_state.current_tool_action_description = "Pick a pixel color";
            break;
        }
    }
}
} // namespace Soldank
