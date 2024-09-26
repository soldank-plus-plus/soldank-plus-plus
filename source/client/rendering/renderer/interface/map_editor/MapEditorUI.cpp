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

    {
        static bool test = false;
        ImGui::Begin("Display");
        if (ImGui::Checkbox("Background", &test)) {
        }
        if (ImGui::Checkbox("Polygons", &test)) {
        }
        if (ImGui::Checkbox("Scenery", &test)) {
        }
        if (ImGui::Checkbox("Spawn points", &test)) {
        }
        if (ImGui::Checkbox("Wireframe", &test)) {
        }
        ImGui::End();
    }

    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();

        // Set position to the top of the viewport and move it below the main menu bar
        ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + ImGui::GetFrameHeight()));

        // Extend width to viewport width
        ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, ImGui::GetFrameHeight()));

        // Add menu bar flag and disable everything else
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse |
                                 ImGuiWindowFlags_NoSavedSettings |
                                 ImGuiWindowFlags_NoBringToFrontOnFocus |
                                 ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_MenuBar;

        if (ImGui::Begin("MapTabBar", nullptr, flags)) {
            if (ImGui::BeginMenuBar()) {
                ImGui::Button("ctf_Ash.pms");
                ImGui::Button("+");
                ImGui::EndMenuBar();
            }
            ImGui::End();
        }
    }

    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();

        // Set position to the bottom of the viewport
        ImGui::SetNextWindowPos(
          ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - ImGui::GetFrameHeight()));

        // Extend width to viewport width
        ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, ImGui::GetFrameHeight()));

        // Add menu bar flag and disable everything else
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse |
                                 ImGuiWindowFlags_NoSavedSettings |
                                 ImGuiWindowFlags_NoBringToFrontOnFocus |
                                 ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_MenuBar;

        if (ImGui::Begin("StatusBar", nullptr, flags)) {
            if (ImGui::BeginMenuBar()) {
                ImGui::Text("Untitled.pms");
                ImGui::SameLine(0, viewport->Size.x / 10.0F);
                ImGui::Text("Zoom: 100%%");
                ImGui::SameLine(0, viewport->Size.x / 10.0F);
                ImGui::Text("Current tool/action description");
                ImGui::SameLine(0, viewport->Size.x / 10.0F);
                // TODO: translate mouse position to position on map.
                ImGui::Text(
                  "Mouse position: %.0f, %.0f", client_state.mouse.x, client_state.mouse.y);
                ImGui::EndMenuBar();
            }
            ImGui::End();
        }
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
} // namespace Soldank::MapEditorUI
