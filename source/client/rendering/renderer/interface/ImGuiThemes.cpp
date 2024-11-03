#include "rendering/renderer/interface/ImGuiThemes.hpp"

#include "imgui.h"
// #include "backends/imgui_impl_glfw.h"
// #include "backends/imgui_impl_opengl3.h"

namespace Soldank
{
void SetupImGuiTheme()
{
    // Future Dark style from ImThemes
    ImGuiStyle& style = ImGui::GetStyle();

    ImGuiIO& io = ImGui::GetIO();
    ImFont* font_title = io.Fonts->AddFontFromFileTTF(
      "play-regular.ttf", 14.0F, nullptr, io.Fonts->GetGlyphRangesDefault());
    IM_ASSERT(font_title != nullptr);
    // ImGui::PushFont(font_title);

    ImVec4* colors = style.Colors;

    // Base
    colors[ImGuiCol_WindowBg] = ImVec4(0.1F, 0.1F, 0.1F, 1.0F);
    colors[ImGuiCol_ChildBg] = ImVec4(0.1F, 0.1F, 0.1F, 1.0F);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08F, 0.08F, 0.08F, 1.0F);
    colors[ImGuiCol_Border] = ImVec4(0.8F, 0.8F, 0.8F, 0.2F);

    colors[ImGuiCol_TitleBg] = ImVec4(0.0F, 0.0F, 0.0F, 1.0F);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.0F, 0.0F, 0.0F, 1.0F);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0F, 0.0F, 0.0F, 1.0F);

    // Text and frames
    colors[ImGuiCol_Text] = ImVec4(1.0F, 1.0F, 1.0F, 1.0F);
    colors[ImGuiCol_FrameBg] = ImVec4(0.2F, 0.2F, 0.2F, 1.0F);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.3F, 0.3F, 0.3F, 0.7F);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.3F, 0.3F, 0.3F, 0.9F);

    // Header
    colors[ImGuiCol_Header] = ImVec4(0.3F, 0.3F, 0.3F, 0.7F);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.4F, 0.4F, 0.4F, 0.8F);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.4F, 0.4F, 0.4F, 1.0F);

    // Buttons
    colors[ImGuiCol_Button] = ImVec4(0.3F, 0.3F, 0.3F, 0.6F);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.4F, 0.4F, 0.4F, 0.8F);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.5F, 0.5F, 0.5F, 1.0F);

    // Tabs
    colors[ImGuiCol_Tab] = ImVec4(0.2F, 0.2F, 0.2F, 1.0F);
    colors[ImGuiCol_TabHovered] = ImVec4(0.15F, 0.15F, 0.15F, 1.0F);
    colors[ImGuiCol_TabActive] = ImVec4(0.1F, 0.1F, 0.1F, 1.0F);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.2F, 0.2F, 0.2F, 1.0F);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.2F, 0.2F, 0.2F, 1.0F);

    // Checkboxes and radio buttons
    colors[ImGuiCol_CheckMark] = ImVec4(0.9F, 0.9F, 0.9F, 1.0F);

    style.WindowRounding = 5.0F;
    style.FrameRounding = 2.0F;
    style.WindowPadding = ImVec2(10, 10);
    style.FramePadding = ImVec2(5, 5);
    style.WindowTitleAlign = ImVec2(0.5, 0.5);
}
} // namespace Soldank
