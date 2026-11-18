#include "imgui.h"

#include <gtest/gtest.h>

#include <string>

import ImGuiThemes;

TEST(ImGuiThemeTest, ReapplyingThemePreservesBuiltFontAtlas)
{
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = { 800.0F, 600.0F };
    io.DeltaTime = 1.0F / 60.0F;
    Soldank::InitializeImGuiFonts();
    auto* interface_font = io.FontDefault;
    ASSERT_NE(interface_font, nullptr);
    EXPECT_NE(std::string(interface_font->GetDebugName()).find("play-regular"), std::string::npos);
    Soldank::SetupImGuiTheme();
    unsigned char* pixels = nullptr;
    int width = 0;
    int height = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    const int font_count = io.Fonts->Fonts.Size;

    for (int iteration = 0; iteration < 10; ++iteration) {
        // Automatic replay recreates the scene before the next NewFrame.
        Soldank::SetupImGuiTheme();
        EXPECT_TRUE(io.Fonts->IsBuilt());
        EXPECT_EQ(io.Fonts->Fonts.Size, font_count);
        EXPECT_EQ(io.FontDefault, interface_font);

        ImGui::NewFrame();
        // Restart, scenario selection, and stepping can recreate it from the UI.
        Soldank::SetupImGuiTheme();
        EXPECT_TRUE(io.Fonts->IsBuilt());
        EXPECT_EQ(io.Fonts->Fonts.Size, font_count);
        EXPECT_EQ(io.FontDefault, interface_font);
        ImGui::EndFrame();
    }
    ImGui::DestroyContext();
}
