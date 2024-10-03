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

    style.Alpha = 1.0F;
    style.DisabledAlpha = 1.0F;
    style.WindowPadding = ImVec2(12.0F, 12.0F);
    style.WindowRounding = 0.0F;
    style.WindowBorderSize = 0.0F;
    style.WindowMinSize = ImVec2(20.0F, 20.0F);
    style.WindowTitleAlign = ImVec2(0.5F, 0.5F);
    style.WindowMenuButtonPosition = ImGuiDir_None;
    style.ChildRounding = 0.0F;
    style.ChildBorderSize = 1.0F;
    style.PopupRounding = 0.0F;
    style.PopupBorderSize = 1.0F;
    style.FramePadding = ImVec2(6.0F, 6.0F);
    style.FrameRounding = 0.0F;
    style.FrameBorderSize = 0.0F;
    style.ItemSpacing = ImVec2(12.0F, 6.0F);
    style.ItemInnerSpacing = ImVec2(6.0F, 3.0F);
    style.CellPadding = ImVec2(12.0F, 6.0F);
    style.IndentSpacing = 20.0F;
    style.ColumnsMinSpacing = 6.0F;
    style.ScrollbarSize = 12.0F;
    style.ScrollbarRounding = 0.0F;
    style.GrabMinSize = 12.0F;
    style.GrabRounding = 0.0F;
    style.TabRounding = 0.0F;
    style.TabBorderSize = 0.0F;
    style.TabMinWidthForCloseButton = 0.0F;
    style.ColorButtonPosition = ImGuiDir_Right;
    style.ButtonTextAlign = ImVec2(0.5F, 0.5F);
    style.SelectableTextAlign = ImVec2(0.0F, 0.0F);

    style.Colors[ImGuiCol_Text] = ImVec4(1.0F, 1.0F, 1.0F, 1.0F);
    style.Colors[ImGuiCol_TextDisabled] =
      ImVec4(0.2745098173618317f, 0.3176470696926117f, 0.4509803950786591f, 1.0f);
    style.Colors[ImGuiCol_WindowBg] =
      ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_ChildBg] =
      ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_PopupBg] =
      ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_Border] =
      ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
    style.Colors[ImGuiCol_BorderShadow] =
      ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_FrameBg] =
      ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] =
      ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive] =
      ImVec4(0.2352941185235977f, 0.2156862765550613f, 0.5960784554481506f, 1.0f);
    style.Colors[ImGuiCol_TitleBg] =
      ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] =
      ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed] =
      ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_MenuBarBg] =
      ImVec4(0.09803921729326248f, 0.105882354080677f, 0.1215686276555061f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarBg] =
      ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrab] =
      ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] =
      ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] =
      ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab] =
      ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_SliderGrabActive] =
      ImVec4(0.5372549295425415f, 0.5529412031173706f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_Button] =
      ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] =
      ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] =
      ImVec4(0.2352941185235977f, 0.2156862765550613f, 0.5960784554481506f, 1.0f);
    style.Colors[ImGuiCol_Header] =
      ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered] =
      ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 1.0f);
    style.Colors[ImGuiCol_HeaderActive] =
      ImVec4(0.2352941185235977f, 0.2156862765550613f, 0.5960784554481506f, 1.0f);
    style.Colors[ImGuiCol_Separator] =
      ImVec4(0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f);
    style.Colors[ImGuiCol_SeparatorHovered] =
      ImVec4(0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f);
    style.Colors[ImGuiCol_SeparatorActive] =
      ImVec4(0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f);
    style.Colors[ImGuiCol_ResizeGrip] =
      ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripHovered] =
      ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripActive] =
      ImVec4(0.2352941185235977f, 0.2156862765550613f, 0.5960784554481506f, 1.0f);
    style.Colors[ImGuiCol_Tab] =
      ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
    style.Colors[ImGuiCol_TabHovered] =
      ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_TabActive] =
      ImVec4(0.09803921729326248f, 0.105882354080677f, 0.1215686276555061f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocused] =
      ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocusedActive] =
      ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_PlotLines] =
      ImVec4(0.5215686559677124f, 0.6000000238418579f, 0.7019608020782471f, 1.0f);
    style.Colors[ImGuiCol_PlotLinesHovered] =
      ImVec4(0.03921568766236305f, 0.9803921580314636f, 0.9803921580314636f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogram] =
      ImVec4(1.0F, 0.2901960909366608F, 0.5960784554481506F, 1.0F);
    style.Colors[ImGuiCol_PlotHistogramHovered] =
      ImVec4(0.9960784316062927F, 0.4745098054409027F, 0.6980392336845398F, 1.0F);
    style.Colors[ImGuiCol_TableHeaderBg] =
      ImVec4(0.0470588244497776F, 0.05490196123719215F, 0.07058823853731155F, 1.0F);
    style.Colors[ImGuiCol_TableBorderStrong] =
      ImVec4(0.0470588244497776F, 0.05490196123719215F, 0.07058823853731155F, 1.0F);
    style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.0F, 0.0f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_TableRowBg] =
      ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_TableRowBgAlt] =
      ImVec4(0.09803921729326248f, 0.105882354080677f, 0.1215686276555061F, 1.0F);
    style.Colors[ImGuiCol_TextSelectedBg] =
      ImVec4(0.2352941185235977F, 0.2156862765550613F, 0.5960784554481506F, 1.0F);
    style.Colors[ImGuiCol_DragDropTarget] =
      ImVec4(0.4980392158031464F, 0.5137255191802979F, 1.0F, 1.0F);
    style.Colors[ImGuiCol_NavHighlight] =
      ImVec4(0.4980392158031464F, 0.5137255191802979F, 1.0F, 1.0F);
    style.Colors[ImGuiCol_NavWindowingHighlight] =
      ImVec4(0.4980392158031464F, 0.5137255191802979F, 1.0F, 1.0F);
    style.Colors[ImGuiCol_NavWindowingDimBg] =
      ImVec4(0.196078434586525F, 0.1764705926179886F, 0.5450980663299561F, 0.501960813999176F);
    style.Colors[ImGuiCol_ModalWindowDimBg] =
      ImVec4(0.196078434586525F, 0.1764705926179886F, 0.5450980663299561F, 0.501960813999176F);
}
} // namespace Soldank
