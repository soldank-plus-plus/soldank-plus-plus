#include "imgui.h"
#include "imgui_internal.h"
#include <gtest/gtest.h>
#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include <GLFW/glfw3.h>

import Chat.Model;
import Chat.Formatting;
import Chat.Palette;
import Chat.Panel;
import Chat.Transcript;
import Application.Input.PlatformInput;
import Application.Input.ApplicationInputController;
import Shared.Networking.ChatProtocol;

using namespace Soldank;

TEST(ChatTest, FormatsOnlySupportedMarkupAndKeepsNamesLiteral)
{
    const auto formatted = FormatChat("one **bold __both__** __italic__ end");
    EXPECT_EQ(formatted.text, "one bold both italic end");
    EXPECT_TRUE(formatted.styles.at(4).bold);
    EXPECT_FALSE(formatted.styles.at(4).italic);
    EXPECT_TRUE(formatted.styles.at(9).bold);
    EXPECT_TRUE(formatted.styles.at(9).italic);
    EXPECT_FALSE(formatted.styles.at(14).bold);
    EXPECT_TRUE(formatted.styles.at(14).italic);
    EXPECT_EQ(FormatChat("**unfinished").text, "**unfinished");
    EXPECT_EQ(FormatChat("****").text, "****");
    EXPECT_EQ(FormatChat("**a __b** c__").text, "**a __b** c__");
    EXPECT_EQ(FormatChat("\\**literal\\** <a> `code`").text, "**literal** <a> `code`");
    EXPECT_EQ(formatted.text.size(), formatted.source_offsets.size());
}
TEST(ChatTest, ClusterBoundariesKeepCombiningAndEmojiSequencesTogether)
{
    EXPECT_EQ(NextChatCluster("e\xCC\x81!", 0), 3U);
    EXPECT_EQ(NextChatCluster("👩‍💻!", 0), std::string("👩‍💻").size());
    EXPECT_EQ(NextChatCluster("🇲🇪!", 0), std::string("🇲🇪").size());
    EXPECT_EQ(NextChatCluster("Ж!", 0), std::string("Ж").size());
}
TEST(ChatTest, PaletteUsesMessageColorsRatherThanShirtColors)
{
    EXPECT_EQ(ChatTeamColor(TeamType::Alpha), 0xFFDF3131U);
    EXPECT_EQ(ChatTeamColor(TeamType::Bravo), 0xFF3131DFU);
    EXPECT_EQ(ChatTeamColor(TeamType::Charlie), 0xFFDFDF31U);
    EXPECT_EQ(ChatTeamColor(TeamType::Delta), 0xFF31DF31U);
    const auto rgba = ChatArgb(0x80402010);
    EXPECT_FLOAT_EQ(rgba.at(0), 64.0F / 255.0F);
    EXPECT_FLOAT_EQ(rgba.at(3), 128.0F / 255.0F);
}
TEST(ChatTest, OfflineSubmissionAndHistoryRestoreDraftAndChannel)
{
    ChatModel model;
    model.team_available = true;
    model.Open(ChatChannel::All);
    model.draft = "First\nmessage";
    ASSERT_TRUE(model.Submit());
    EXPECT_EQ(model.entries.back().message.text, "First message");
    model.Open(ChatChannel::Team);
    model.draft = "Second";
    ASSERT_TRUE(model.Submit());
    model.draft = "unsent";
    model.channel = ChatChannel::All;
    model.NavigateHistory(true);
    EXPECT_EQ(model.draft, "Second");
    EXPECT_EQ(model.channel, ChatChannel::Team);
    model.NavigateHistory(true);
    EXPECT_EQ(model.draft, "First message");
    model.NavigateHistory(false);
    model.NavigateHistory(false);
    EXPECT_EQ(model.draft, "unsent");
    EXPECT_EQ(model.channel, ChatChannel::All);
}
TEST(ChatTest, RejectsUnavailableTeam)
{
    ChatModel model;
    model.Open(ChatChannel::Team);
    model.draft = "private";
    EXPECT_FALSE(model.Submit());
    EXPECT_TRUE(model.entries.empty());
}
TEST(ChatTest, ServerFailurePreservesNewDraft)
{
    ChatModel model;
    model.draft = "public";
    model.online = true;
    model.channel = ChatChannel::All;
    std::vector<NetworkMessage> sent;
    model.send = [&](const NetworkMessage& message) { sent.push_back(message); };
    ASSERT_TRUE(model.Submit());
    EXPECT_TRUE(model.entries.empty());
    const auto request = DecodeChatRequest(sent.front());
    ASSERT_TRUE(request);
    model.draft = "new draft";
    model.Result({ .request_id = request->request_id, .status = ChatStatus::RateLimited });
    EXPECT_EQ(model.draft, "new draft");
    EXPECT_TRUE(model.open);
    EXPECT_FALSE(model.error.empty());
}
TEST(ChatTest, DisconnectClearsSessionState)
{
    ChatModel model;
    model.draft = "message";
    EXPECT_TRUE(model.Submit());
    model.Disconnect();
    EXPECT_TRUE(model.pending.empty());
    EXPECT_TRUE(model.history.empty());
    EXPECT_TRUE(model.entries.empty());
    EXPECT_TRUE(model.draft.empty());
    EXPECT_FALSE(model.connected);
}
TEST(ChatTest, TranscriptAndHistoryAreBoundedAndDeliveryIsNotDuplicated)
{
    ChatModel model;
    for (std::uint64_t i = 1; i <= 510; ++i) {
        model.Receive({ .message_id = i, .name = "Test", .text = "message" });
    }
    model.Receive(model.entries.back().message);
    EXPECT_EQ(model.entries.size(), 500U);
    EXPECT_EQ(model.evictions, 10U);
    for (int i = 0; i < 105; ++i) {
        model.draft = std::to_string(i);
        ASSERT_TRUE(model.Submit());
    }
    EXPECT_EQ(model.history.size(), 100U);
}
TEST(ChatTest, HeldInputsRequireReleaseBeforeGameplayResumes)
{
    PlatformInput input;
    input.OnKey(GLFW_KEY_A, GLFW_PRESS, 0);
    input.OnMouseButton(GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS);
    input.SuppressHeldActions();
    EXPECT_FALSE(input.Key(GLFW_KEY_A));
    EXPECT_FALSE(input.Button(GLFW_MOUSE_BUTTON_LEFT));
    input.OnKey(GLFW_KEY_A, GLFW_REPEAT, 0);
    EXPECT_FALSE(input.Key(GLFW_KEY_A));
    input.OnKey(GLFW_KEY_A, GLFW_RELEASE, 0);
    input.OnKey(GLFW_KEY_A, GLFW_PRESS, 0);
    EXPECT_TRUE(input.Key(GLFW_KEY_A));
    input.OnMouseButton(GLFW_MOUSE_BUTTON_LEFT, GLFW_RELEASE);
    input.OnMouseButton(GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS);
    EXPECT_TRUE(input.Button(GLFW_MOUSE_BUTTON_LEFT));
}

namespace
{
class ChatWidgetTest : public testing::Test
{
protected:
    void SetUp() override
    {
        ImGui::CreateContext();
        ImGui::GetIO().IniFilename = nullptr;
        ImGui::GetIO().DisplaySize = { 800, 600 };
        ImGui::GetIO().DeltaTime = 1.0F / 60.0F;
        ChatFonts::Initialize();
        unsigned char* pixels = nullptr;
        int width = 0;
        int height = 0;
        ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        ImGui::GetIO().Fonts->SetTexID(1);
    }
    void TearDown() override { ImGui::DestroyContext(); }
};
} // namespace
TEST_F(ChatWidgetTest, TranscriptWrapsStyledMessagesAndCopiesAcrossThem)
{
    ChatModel model;
    model.Receive(
      { .message_id = 1, .sender_session = 1, .name = "**Name**", .text = "**bold** __italic__" });
    model.Receive({
      .message_id = 2,
      .sender_session = 2,
      .name = "Other",
      .text = "A long message that wraps across several lines in a narrow window.",
    });
    SelectableChatTranscript transcript;
    auto const frame = [&] {
        ImGui::NewFrame();
        ImGui::SetNextWindowPos({ 0, 0 });
        ImGui::SetNextWindowSize({ 220, 300 });
        ImGui::Begin("Transcript");
        transcript.Render(model);
        ImGui::End();
        ImGui::Render();
    };
    frame();
    frame();
    EXPECT_EQ(transcript.GetVisibleText(),
              "[All] **Name**: bold italic\n[All] Other: A long message that wraps across several "
              "lines in a narrow window.\n");
    ImGui::GetIO().AddMousePosEvent(30, 40);
    ImGui::GetIO().AddMouseButtonEvent(0, true);
    frame();
    ImGui::GetIO().AddMouseButtonEvent(0, false);
    frame();
    ImGui::GetIO().AddKeyEvent(ImGuiMod_Ctrl, true);
    ImGui::GetIO().AddKeyEvent(ImGuiKey_A, true);
    frame();
    ImGui::GetIO().AddKeyEvent(ImGuiKey_A, false);
    ImGui::GetIO().AddKeyEvent(ImGuiKey_C, true);
    frame();
    EXPECT_EQ(std::string(ImGui::GetClipboardText()), transcript.GetVisibleText());
}
TEST_F(ChatWidgetTest, PanelUsesOneUiFrameAndRendersNestedStyles)
{
    ChatModel model;
    model.Open(ChatChannel::All);
    model.Receive({
      .message_id = 1,
      .sender_session = 1,
      .name = "Test",
      .text = "**bold __both__** __italic__",
    });
    ChatPanel panel;
    for (int i = 0; i < 3; ++i) {
        ImGui::NewFrame();
        panel.Render(model);
        ImGui::Render();
        EXPECT_GT(ImGui::GetDrawData()->TotalVtxCount, 0);
    }
}

TEST_F(ChatWidgetTest, DraftCaretAndClipboardUseNormalTextEditing)
{
    ChatModel model;
    model.Open(ChatChannel::All);
    ChatPanel panel;
    const auto frame = [&] {
        ImGui::NewFrame();
        panel.Render(model);
        ImGui::Render();
    };
    const auto key = [&](ImGuiKey value) {
        ImGui::GetIO().AddKeyEvent(value, true);
        frame();
        ImGui::GetIO().AddKeyEvent(value, false);
        frame();
    };
    frame();
    frame();
    frame();
    ImGui::GetIO().AddInputCharactersUTF8("abcd");
    frame();
    EXPECT_EQ(model.draft, "abcd");
    key(ImGuiKey_LeftArrow);
    key(ImGuiKey_LeftArrow);
    ImGui::GetIO().AddInputCharactersUTF8("X");
    frame();
    EXPECT_EQ(model.draft, "abXcd");
    ImGui::GetIO().AddKeyEvent(ImGuiMod_Ctrl, true);
    key(ImGuiKey_A);
    key(ImGuiKey_C);
    EXPECT_EQ(std::string(ImGui::GetClipboardText()), "abXcd");
    ImGui::SetClipboardText("pasted");
    key(ImGuiKey_V);
    EXPECT_EQ(model.draft, "pasted");
    ImGui::GetIO().AddKeyEvent(ImGuiMod_Ctrl, false);
    frame();
}

TEST_F(ChatWidgetTest, SubmitAndHistoryUseInputWidget)
{
    ChatModel model;
    model.Open(ChatChannel::All);
    ChatPanel panel;
    const auto frame = [&] {
        ImGui::NewFrame();
        panel.Render(model);
        ImGui::Render();
    };
    const auto key = [&](ImGuiKey value) {
        ImGui::GetIO().AddKeyEvent(value, true);
        frame();
        ImGui::GetIO().AddKeyEvent(value, false);
        frame();
    };
    frame();
    frame();
    frame();
    ImGui::GetIO().AddInputCharactersUTF8("pasted");
    frame();
    key(ImGuiKey_Enter);
    EXPECT_FALSE(model.open);
    ASSERT_EQ(model.entries.size(), 1U);
    EXPECT_EQ(model.entries.back().message.text, "pasted");
    model.Open(ChatChannel::All);
    frame();
    frame();
    ImGui::GetIO().AddInputCharactersUTF8("draft");
    frame();
    key(ImGuiKey_UpArrow);
    EXPECT_EQ(model.draft, "pasted");
    key(ImGuiKey_DownArrow);
    EXPECT_EQ(model.draft, "draft");
}

TEST_F(ChatWidgetTest, LoadsDistinctFontFacesForAllSupportedStyles)
{
    const auto* regular = ChatFonts::Get({});
    EXPECT_NE(regular, ChatFonts::Get({ .bold = true }));
    EXPECT_NE(regular, ChatFonts::Get({ .italic = true }));
    EXPECT_NE(regular, ChatFonts::Get({ .bold = true, .italic = true }));
}

TEST_F(ChatWidgetTest, PanelCanBeDraggedByTitleAndClampsToResizedViewport)
{
    ChatModel model;
    model.Open(ChatChannel::All);
    ChatPanel panel;
    ImVec2 position;
    ImVec2 size;
    const auto frame = [&] {
        ImGui::NewFrame();
        panel.Render(model);
        ImGui::Begin("Chat###GameChat");
        position = ImGui::GetWindowPos();
        size = ImGui::GetWindowSize();
        ImGui::End();
        ImGui::Render();
    };
    frame();
    frame();
    const ImVec2 original = position;
    ImGui::GetIO().AddMousePosEvent(position.x + 50, position.y + 10);
    ImGui::GetIO().AddMouseButtonEvent(0, true);
    frame();
    ImGui::GetIO().AddMousePosEvent(original.x + 150, original.y + 50);
    frame();
    ImGui::GetIO().AddMouseButtonEvent(0, false);
    frame();
    EXPECT_GT(position.x, original.x);
    ImGui::GetIO().DisplaySize = { 320, 240 };
    frame();
    frame();
    EXPECT_GE(position.x, 0);
    EXPECT_GE(position.y, 0);
    EXPECT_LE(position.x + size.x, 320);
    EXPECT_LE(position.y + size.y, 240);
}

TEST_F(ChatWidgetTest, EscapePreservesTheDraftInsteadOfRevertingInputEdits)
{
    ChatModel model;
    model.Open(ChatChannel::All);
    ChatPanel panel;
    const auto frame = [&] {
        ImGui::NewFrame();
        panel.Render(model);
        ImGui::Render();
    };
    frame();
    frame();
    frame();
    ImGui::GetIO().AddInputCharactersUTF8("keep this draft");
    frame();
    ImGui::GetIO().AddKeyEvent(ImGuiKey_Escape, true);
    frame();
    EXPECT_FALSE(model.open);
    EXPECT_EQ(model.draft, "keep this draft");
}

TEST(ChatTest, EnterShortcutsChoosePublicOrTeamAndIgnoreRepeat)
{
    EXPECT_EQ(ApplicationInputController::GetChatShortcut(GLFW_KEY_ENTER, GLFW_PRESS, 0),
              ChatChannel::All);
    EXPECT_EQ(
      ApplicationInputController::GetChatShortcut(GLFW_KEY_ENTER, GLFW_PRESS, GLFW_MOD_SHIFT),
      ChatChannel::Team);
    EXPECT_EQ(
      ApplicationInputController::GetChatShortcut(GLFW_KEY_KP_ENTER, GLFW_PRESS, GLFW_MOD_SHIFT),
      ChatChannel::Team);
    EXPECT_FALSE(ApplicationInputController::GetChatShortcut(GLFW_KEY_ENTER, GLFW_REPEAT, 0));
    EXPECT_FALSE(ApplicationInputController::GetChatShortcut(GLFW_KEY_T, GLFW_PRESS, 0));
    EXPECT_FALSE(ApplicationInputController::GetChatShortcut(GLFW_KEY_Y, GLFW_PRESS, 0));
}

TEST_F(ChatWidgetTest, OpeningEnterDoesNotImmediatelyCloseChat)
{
    ChatModel model;
    model.Open(ChatChannel::All);
    ChatPanel panel;
    ImGui::GetIO().AddKeyEvent(ImGuiKey_Enter, true);
    ImGui::NewFrame();
    panel.Render(model);
    ImGui::Render();
    EXPECT_TRUE(model.open);
    ImGui::GetIO().AddKeyEvent(ImGuiKey_Enter, false);
    ImGui::NewFrame();
    panel.Render(model);
    ImGui::Render();
    EXPECT_TRUE(model.open);
    ImGui::GetIO().AddInputCharactersUTF8("fast message");
    ImGui::GetIO().AddKeyEvent(ImGuiKey_Enter, true);
    ImGui::NewFrame();
    panel.Render(model);
    ImGui::Render();
    ImGui::NewFrame();
    panel.Render(model);
    ImGui::Render();
    ASSERT_EQ(model.entries.size(), 1U);
    EXPECT_EQ(model.entries.front().message.text, "fast message");
}

TEST_F(ChatWidgetTest, EnterAndShiftEnterCloseEmptyInputWithoutSending)
{
    ChatModel model;
    ChatPanel panel;
    const auto frame = [&] {
        ImGui::NewFrame();
        panel.Render(model);
        ImGui::Render();
    };
    for (const bool shift : { false, true }) {
        model.Open(shift ? ChatChannel::Team : ChatChannel::All);
        frame();
        frame();
        ImGui::GetIO().AddKeyEvent(ImGuiMod_Shift, shift);
        ImGui::GetIO().AddKeyEvent(ImGuiKey_Enter, true);
        frame();
        EXPECT_FALSE(model.open);
        EXPECT_TRUE(model.error.empty());
        ImGui::GetIO().AddKeyEvent(ImGuiKey_Enter, false);
        ImGui::GetIO().AddKeyEvent(ImGuiMod_Shift, false);
        frame();
    }
    EXPECT_TRUE(model.entries.empty());
    EXPECT_TRUE(model.history.empty());
}

TEST_F(ChatWidgetTest, PreviewSharesMovedAndResizedMessageAreaWithOpenChat)
{
    ChatModel model;
    model.Open(ChatChannel::All);
    model.Receive({ .message_id = 1, .name = "Player", .text = "Hello" });
    ChatPanel panel;
    const auto frame = [&] {
        ImGui::NewFrame();
        panel.Render(model);
        ImGui::Render();
    };
    frame();
    frame();
    ImGui::SetWindowPos("Chat###GameChat", { 90, 65 });
    ImGui::SetWindowSize("Chat###GameChat", { 610, 370 });
    frame();
    const auto* window = ImGui::FindWindowByName("Chat###GameChat");
    const auto* messages = window->DC.ChildWindows.front();
    const auto position = messages->Pos;
    const auto size = messages->Size;
    model.open = false;
    frame();
    window = ImGui::FindWindowByName("Chat###GameChat");
    messages = window->DC.ChildWindows.front();
    EXPECT_EQ(messages->Pos.x, position.x);
    EXPECT_EQ(messages->Pos.y, position.y);
    EXPECT_EQ(messages->Size.x, size.x);
    EXPECT_EQ(messages->Size.y, size.y);
}

TEST_F(ChatWidgetTest, InputRegainsFocusAfterDraggingTheWindow)
{
    ChatModel model;
    model.Open(ChatChannel::All);
    ChatPanel panel;
    const auto frame = [&] {
        ImGui::NewFrame();
        panel.Render(model);
        ImGui::Render();
    };
    frame();
    frame();
    frame();
    ImGui::GetIO().AddInputCharactersUTF8("before ");
    frame();
    const auto* window = ImGui::FindWindowByName("Chat###GameChat");
    const auto position = window->Pos;
    ImGui::GetIO().AddMousePosEvent(position.x + 50, position.y + 10);
    ImGui::GetIO().AddMouseButtonEvent(0, true);
    frame();
    ImGui::GetIO().AddMousePosEvent(position.x + 90, position.y + 30);
    frame();
    ImGui::GetIO().AddMouseButtonEvent(0, false);
    frame();
    frame();
    ImGui::GetIO().AddInputCharactersUTF8("after dragging");
    frame();
    EXPECT_EQ(model.draft, "before after dragging");
}

TEST_F(ChatWidgetTest, InputRegainsFocusAfterResizingTheWindow)
{
    ChatModel model;
    model.Open(ChatChannel::All);
    ChatPanel panel;
    const auto frame = [&] {
        ImGui::NewFrame();
        panel.Render(model);
        ImGui::Render();
    };
    frame();
    frame();
    frame();
    const auto* window = ImGui::FindWindowByName("Chat###GameChat");
    const auto position = window->Pos;
    const auto size = window->Size;
    ImGui::GetIO().AddMousePosEvent(position.x + size.x - 3, position.y + size.y - 3);
    ImGui::GetIO().AddMouseButtonEvent(0, true);
    frame();
    ImGui::GetIO().AddMousePosEvent(position.x + size.x + 25, position.y + size.y - 25);
    frame();
    ImGui::GetIO().AddMouseButtonEvent(0, false);
    frame();
    frame();
    ImGui::GetIO().AddInputCharactersUTF8("after resizing");
    frame();
    EXPECT_NE(ImGui::FindWindowByName("Chat###GameChat")->Size.x, size.x);
    EXPECT_EQ(model.draft, "after resizing");
}
