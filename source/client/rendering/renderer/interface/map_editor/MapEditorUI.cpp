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

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
} // namespace Soldank::MapEditorUI
