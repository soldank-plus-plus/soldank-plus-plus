module;
#include "imgui.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <string>

export module Chat.Panel;
import Chat.Model;
import Chat.Transcript;
import Chat.Formatting;
import Chat.Palette;

export namespace Soldank
{
class ChatPanel
{
public:
    void Render(ChatModel& model)
    {
        HandleCloseShortcuts(model);
        const auto& io = ImGui::GetIO();
        ImGui::PushFont(ChatFonts::Get({}));
        ImGui::SetNextWindowPos({ 24, std::max(0.0F, io.DisplaySize.y - 340.0F) },
                                ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize({ 520, 300 }, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(
          { std::min(300.0F, io.DisplaySize.x), std::min(180.0F, io.DisplaySize.y) },
          io.DisplaySize);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0F);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0.045F, 0.055F, 0.075F, 0.96F });
        auto flags = static_cast<unsigned int>(ImGuiWindowFlags_NoCollapse);
        if (!model.open) {
            flags |= static_cast<unsigned int>(ImGuiWindowFlags_NoInputs) |
                     static_cast<unsigned int>(ImGuiWindowFlags_NoBackground) |
                     static_cast<unsigned int>(ImGuiWindowFlags_NoFocusOnAppearing);
            ImGui::PushStyleColor(ImGuiCol_TitleBg, { 0, 0, 0, 0 });
            ImGui::PushStyleColor(ImGuiCol_TitleBgActive, { 0, 0, 0, 0 });
            ImGui::PushStyleColor(ImGuiCol_Text, { 0, 0, 0, 0 });
            ImGui::PushStyleColor(ImGuiCol_Border, { 0, 0, 0, 0 });
            ImGui::PushStyleColor(ImGuiCol_ChildBg, { 0, 0, 0, 0 });
        }
        const bool expanded = model.open;
        const bool visible = ImGui::Begin("Chat###GameChat",
                                          expanded ? &model.open : nullptr,
                                          static_cast<ImGuiWindowFlags>(flags));
        const auto position = ImGui::GetWindowPos();
        const auto size = ImGui::GetWindowSize();
        const ImVec2 clamped{
            std::clamp(position.x, 0.0F, std::max(0.0F, io.DisplaySize.x - size.x)),
            std::clamp(position.y, 0.0F, std::max(0.0F, io.DisplaySize.y - size.y))
        };
        if (!ImGui::IsMouseDragging(ImGuiMouseButton_Left) &&
            (clamped.x != position.x || clamped.y != position.y)) {
            ImGui::SetWindowPos(clamped);
        }
        if (visible) {
            const bool has_status = !model.error.empty() || transcript_.HasUnread();
            const float footer = ImGui::GetFrameHeightWithSpacing() * (has_status ? 2.0F : 1.0F);
            auto message_flags = static_cast<unsigned int>(ImGuiWindowFlags_HorizontalScrollbar);
            if (!expanded) {
                message_flags |= static_cast<unsigned int>(ImGuiWindowFlags_NoInputs) |
                                 static_cast<unsigned int>(ImGuiWindowFlags_NoBackground) |
                                 static_cast<unsigned int>(ImGuiWindowFlags_NoScrollbar);
            }
            ImGui::BeginChild(expanded ? "##ChatMessages" : "##ChatPreviewMessages",
                              { 0, -footer },
                              ImGuiChildFlags_None,
                              static_cast<ImGuiWindowFlags>(message_flags));
            if (expanded) {
                transcript_.Render(model);
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsWindowHovered()) {
                    transcript_.ReleaseKeyboard();
                }
            } else {
                const auto recent = RecentMessages(model);
                ImGui::BeginDisabled();
                preview_.Render(recent);
                ImGui::EndDisabled();
            }
            ImGui::EndChild();
            if (expanded) {
                ImGui::Separator();
                RenderInput(model);
            }
        }
        ImGui::End();
        if (!expanded) {
            ImGui::PopStyleColor(5);
        }
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        ImGui::PopFont();
    }

private:
    static void HandleCloseShortcuts(ChatModel& model)
    {
        if (model.open && !model.focus_input && !ImGui::GetIO().WantTextInput &&
            model.draft.empty() &&
            (ImGui::IsKeyPressed(ImGuiKey_Enter, false) ||
             ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false))) {
            model.Submit();
        }
        if (model.open && ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            model.open = false;
        }
    }

    void RenderInput(ChatModel& model)
    {
        auto& io = ImGui::GetIO();
        const char* channel = model.channel == ChatChannel::Team ? "Team" : "All";
        ImGui::SetNextItemWidth(85);
        if (ImGui::BeginCombo("##ChatChannel", channel)) {
            if (ImGui::Selectable("All", model.channel == ChatChannel::All)) {
                model.channel = ChatChannel::All;
            }
            ImGui::BeginDisabled(!model.team_available);
            if (ImGui::Selectable("Team", model.channel == ChatChannel::Team)) {
                model.channel = ChatChannel::Team;
            }
            ImGui::EndDisabled();
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(std::max(30.0F, ImGui::GetContentRegionAvail().x - 60.0F));
        const bool just_focused = model.focus_input;
        if (model.focus_input) {
            ImGui::SetKeyboardFocusHere();
            // Discard characters queued before the input received focus.
            io.ClearInputCharacters();
            model.focus_input = false;
        } else if (!ImGui::IsAnyItemActive() && !ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
                   !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup) &&
                   !transcript_.HasKeyboardFocus()) {
            ImGui::SetKeyboardFocusHere();
        }
        std::array<char, 4096> buffer{};
        const auto count = std::min(model.draft.size(), buffer.size() - 1);
        std::copy_n(model.draft.begin(), count, buffer.begin());
        const bool submit = ImGui::InputText("##ChatInput",
                                             buffer.data(),
                                             buffer.size(),
                                             ImGuiInputTextFlags_EnterReturnsTrue |
                                               ImGuiInputTextFlags_CallbackHistory,
                                             HistoryCallback,
                                             &model);
        if (ImGui::IsItemActive()) {
            transcript_.ReleaseKeyboard();
        }
        model.draft = buffer.data();
        ImGui::SameLine();
        const bool send_clicked = ImGui::Button("Send");
        if ((send_clicked || (submit && !just_focused)) && !model.Submit()) {
            model.focus_input = true;
        }
        if (!model.error.empty()) {
            ImGui::PushTextWrapPos();
            ImGui::TextUnformatted(model.error.c_str());
            ImGui::PopTextWrapPos();
        } else if (transcript_.HasUnread()) {
            ImGui::TextUnformatted("New messages below");
        }
    }
    static int HistoryCallback(ImGuiInputTextCallbackData* data)
    {
        auto& model = *static_cast<ChatModel*>(data->UserData);
        model.draft.assign(data->Buf, static_cast<std::size_t>(data->BufTextLen));
        model.NavigateHistory(data->EventKey == ImGuiKey_UpArrow);
        data->DeleteChars(0, data->BufTextLen);
        data->InsertChars(0, model.draft.c_str());
        data->CursorPos = data->BufTextLen;
        data->SelectionStart = data->SelectionEnd = data->CursorPos;
        return 0;
    }
    static ChatModel RecentMessages(const ChatModel& model)
    {
        ChatModel recent;
        std::size_t expired = 0;
        for (const auto& entry : model.entries) {
            if (std::chrono::steady_clock::now() - entry.received <= std::chrono::seconds(10)) {
                recent.entries.push_back(entry);
            } else {
                ++expired;
            }
        }
        recent.revision = (model.revision * 501) + recent.entries.size();
        recent.evictions = model.evictions + expired;
        return recent;
    }
    SelectableChatTranscript transcript_;
    SelectableChatTranscript preview_;
};
} // namespace Soldank
