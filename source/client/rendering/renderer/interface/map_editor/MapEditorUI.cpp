#include "rendering/renderer/interface/map_editor/MapEditorUI.hpp"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

namespace Soldank::MapEditorUI
{
void Render(State& game_state, ClientState& client_state, double frame_percent, int fps)
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(client_state.mouse.x, client_state.mouse.y);
    io.MouseDrawCursor = io.WantCaptureMouse;
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit")) {
                if (ImGui::MenuItem("Undo", "CTRL+Z")) {
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Windows")) {
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

    {
        ImGui::Begin("Tools");
        ImGui::Text("Transform tool (A)");
        ImGui::Text("Polygon tool (Q)");
        ImGui::Text("Vertex selection tool (S)");
        ImGui::Text("Selection tool (W)");
        ImGui::Text("Vertex color tool (D)");
        ImGui::Text("Color tool (E)");
        ImGui::Text("Texture tool (F)");
        ImGui::Text("Scenery tool (R)");
        ImGui::Text("Waypoint tool (G)");
        ImGui::Text("Spawnpoint tool (T)");
        ImGui::Text("Color picker tool (H)");
        ImGui::End();
    }

    {
        ImGui::Begin("Debug");
        ImGui::Text("Mouse position: %lf %lf", client_state.mouse.x, client_state.mouse.y);
        ImGui::End();
    }

    {
        ImGui::Begin("Properties");
        ImGui::Text("Polygons: %zu", game_state.map.GetPolygons().size());
        ImGui::Text("Sceneries: %zu/500", game_state.map.GetSceneryInstances().size());
        ImGui::Text("Spawns: %zu/128", game_state.map.GetSpawnPoints().size());
        ImGui::Text("Colliders: %zu/128", game_state.map.GetColliders().size());
        ImGui::Text("Waypoints: %zu/500", game_state.map.GetWayPoints().size());
        ImGui::Text("Connections: 0"); // TODO: Add connections of waypoints
        glm::vec2 map_dimensions;
        map_dimensions.x = game_state.map.GetBoundaries()[Map::MapBoundary::RightBoundary] -
                           game_state.map.GetBoundaries()[Map::MapBoundary::LeftBoundary];
        map_dimensions.y = game_state.map.GetBoundaries()[Map::MapBoundary::BottomBoundary] -
                           game_state.map.GetBoundaries()[Map::MapBoundary::TopBoundary];
        ImGui::Text("Dimensions: %.0fx%.0f", map_dimensions.x, map_dimensions.y);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
} // namespace Soldank::MapEditorUI
