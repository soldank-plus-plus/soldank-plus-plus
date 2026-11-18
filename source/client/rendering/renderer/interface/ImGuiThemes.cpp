module;

#include "imgui.h"

#include <filesystem>
#include <stdexcept>

export module ImGuiThemes;

namespace Soldank
{
// Call once after creating the ImGui context, before backend initialization.
export void InitializeImGuiFonts()
{
    if (!std::filesystem::is_regular_file("play-regular.ttf")) {
        throw std::runtime_error("Missing ImGui font: play-regular.ttf");
    }
    ImGuiIO& io = ImGui::GetIO();
    io.FontDefault = io.Fonts->AddFontFromFileTTF(
      "play-regular.ttf", 14.0F, nullptr, io.Fonts->GetGlyphRangesDefault());
    if (io.FontDefault == nullptr) {
        throw std::runtime_error("Failed to load ImGui font: play-regular.ttf");
    }
}

export void SetupImGuiTheme()
{
    // Future Dark style from ImThemes
    ImGuiStyle& style = ImGui::GetStyle();

    // Scenes may be recreated between frames or during an active UI frame.
    // Keep theme setup independent of the context-owned font atlas.

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
