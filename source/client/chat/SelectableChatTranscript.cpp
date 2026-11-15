module;
#include "imgui.h"
#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

export module Chat.Transcript;
import Chat.Model;
import Chat.Formatting;
import Chat.Palette;

export namespace Soldank
{
namespace ChatFonts
{
std::array<ImFont*, 4>& Fonts()
{
    static std::array<ImFont*, 4> fonts{};
    return fonts;
}
void Initialize()
{
    if (ImGui::GetIO().Fonts->Fonts.empty()) {
        ImGui::GetIO().Fonts->AddFontDefault();
    }
    constexpr std::array<const char*, 4> PATHS{
        "fonts/LiberationSans-Regular.ttf",
        "fonts/LiberationSans-Bold.ttf",
        "fonts/LiberationSans-Italic.ttf",
        "fonts/LiberationSans-BoldItalic.ttf",
    };
    for (std::size_t i = 0; i < PATHS.size(); ++i) {
        Fonts().at(i) = nullptr;
        if (!std::filesystem::exists(PATHS.at(i))) {
            continue;
        }
        Fonts().at(i) = ImGui::GetIO().Fonts->AddFontFromFileTTF(
          PATHS.at(i), 17.0F, nullptr, ImGui::GetIO().Fonts->GetGlyphRangesCyrillic());
    }
}
ImFont* Get(ChatStyle style)
{
    auto* font = Fonts().at((style.bold ? 1U : 0U) + (style.italic ? 2U : 0U));
    return font != nullptr ? font : ImGui::GetFont();
}
} // namespace ChatFonts

ImU32 ChatDrawColor(std::uint32_t argb)
{
    const auto color = ChatArgb(argb);
    return ImGui::ColorConvertFloat4ToU32({ color.at(0), color.at(1), color.at(2), color.at(3) });
}

class SelectableChatTranscript
{
public:
    void Render(const ChatModel& model)
    {
        const float width = std::max(1.0F, ImGui::GetContentRegionAvail().x);
        const float scale = ImGui::GetFontSize() / ImGui::GetFont()->FontSize;
        const float size = 17.0F * scale;
        if (revision_ != model.revision || width_ != width || font_size_ != size) {
            if (evictions_ != model.evictions) {
                anchor_ = 0;
                focus_ = 0;
                selecting_ = false;
                const auto removed =
                  std::min(model.evictions - evictions_, entry_positions_.size());
                const auto removed_height =
                  removed < entry_positions_.size() ? entry_positions_.at(removed) : height_;
                ImGui::SetScrollY(std::max(0.0F, ImGui::GetScrollY() - removed_height));
            }
            Build(model, width, size);
            evictions_ = model.evictions;
        }
        const bool at_bottom = ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 2.0F;
        const bool new_message = displayed_revision_ != model.revision;
        const ImVec2 origin = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("##transcript",
                               { width, std::max(height_, ImGui::GetContentRegionAvail().y) });
        HandleSelection(origin);
        DrawGlyphs(origin);
        if (new_message && at_bottom && !selecting_) {
            ImGui::SetScrollHereY(1.0F);
            unread_ = false;
        } else if (new_message) {
            unread_ = true;
        }
        if (at_bottom) {
            unread_ = false;
        }
        displayed_revision_ = model.revision;
    }
    void ReleaseKeyboard() { keyboard_focus_ = false; }
    bool HasKeyboardFocus() const { return keyboard_focus_; }
    bool HasUnread() const { return unread_; }
    const std::string& GetVisibleText() const { return text_; }

private:
    void HandleSelection(ImVec2 origin)
    {
        const bool hovered = ImGui::IsItemHovered();
        if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            keyboard_focus_ = true;
            focus_ = HitTest(
              { ImGui::GetIO().MousePos.x - origin.x, ImGui::GetIO().MousePos.y - origin.y });
            if (!ImGui::GetIO().KeyShift) {
                anchor_ = focus_;
            }
            selecting_ = true;
        }
        if (selecting_ && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            focus_ = HitTest(
              { ImGui::GetIO().MousePos.x - origin.x, ImGui::GetIO().MousePos.y - origin.y });
            const float top = ImGui::GetWindowPos().y;
            const float bottom = top + ImGui::GetWindowSize().y;
            if (ImGui::GetIO().MousePos.y < top + 12.0F) {
                ImGui::SetScrollY(ImGui::GetScrollY() - (250.0F * ImGui::GetIO().DeltaTime));
            } else if (ImGui::GetIO().MousePos.y > bottom - 12.0F) {
                ImGui::SetScrollY(ImGui::GetScrollY() + (250.0F * ImGui::GetIO().DeltaTime));
            }
        }
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            selecting_ = false;
        }
        if (keyboard_focus_ && ImGui::GetIO().KeyCtrl) {
            if (ImGui::IsKeyPressed(ImGuiKey_A, false)) {
                anchor_ = 0;
                focus_ = text_.size();
            }
            if (ImGui::IsKeyPressed(ImGuiKey_C, false) && anchor_ != focus_) {
                const auto begin = std::min(anchor_, focus_);
                const auto selected = text_.substr(begin, std::max(anchor_, focus_) - begin);
                ImGui::SetClipboardText(selected.c_str());
            }
        }
    }
    void DrawGlyphs(ImVec2 origin) const
    {
        auto* draw = ImGui::GetWindowDrawList();
        const auto selection_begin = std::min(anchor_, focus_);
        const auto selection_end = std::max(anchor_, focus_);
        for (const auto& glyph : glyphs_) {
            const ImVec2 position{ origin.x + glyph.position.x, origin.y + glyph.position.y };
            if (!ImGui::IsRectVisible(position,
                                      { position.x + glyph.width, position.y + line_height_ })) {
                continue;
            }
            if (glyph.begin < selection_end && glyph.end > selection_begin) {
                draw->AddRectFilled(position,
                                    { position.x + glyph.width, position.y + line_height_ },
                                    IM_COL32(65, 105, 170, 170));
            }
            draw->AddText(glyph.font, font_size_, position, glyph.color, glyph.text.c_str());
        }
    }
    struct Glyph
    {
        std::size_t begin{};
        std::size_t end{};
        ImVec2 position;
        float width{};
        ImFont* font{};
        ImU32 color{};
        std::string text;
    };
    void Build(const ChatModel& model, float width, float size)
    {
        text_.clear();
        glyphs_.clear();
        entry_positions_.clear();
        width_ = width;
        font_size_ = size;
        line_height_ = std::ceil(size * 1.3F);
        ImVec2 position{};
        const ChatStyle plain;
        for (const auto& entry : model.entries) {
            entry_positions_.push_back(position.y);
            const auto& message = entry.message;
            const auto channel = ChatLabel(message.kind, message.channel);
            const auto name_color =
              message.kind == ChatKind::System ? CHAT_SYSTEM_COLOR : ChatTeamColor(message.team);
            Append(std::string(channel) + " " + message.name + ": ", plain, name_color, position);
            const auto body = FormatChat(message.text);
            const auto color = ChatBodyColor(message.kind, message.channel);
            for (std::size_t offset = 0; offset < body.text.size();) {
                const auto end = NextChatCluster(body.text, offset);
                Append(std::string_view(body.text).substr(offset, end - offset),
                       body.styles.at(offset),
                       color,
                       position);
                offset = end;
            }
            text_.push_back('\n');
            position = { 0.0F, position.y + line_height_ };
        }
        height_ = std::max(line_height_, position.y);
        revision_ = model.revision;
        anchor_ = std::min(anchor_, text_.size());
        focus_ = std::min(focus_, text_.size());
    }
    void Append(std::string_view text, ChatStyle style, std::uint32_t color, ImVec2& position)
    {
        auto* font = ChatFonts::Get(style);
        for (std::size_t offset = 0; offset < text.size();) {
            const auto end = NextChatCluster(text, offset);
            const std::string cluster(text.substr(offset, end - offset));
            const auto size = font->CalcTextSizeA(font_size_, FLT_MAX, 0.0F, cluster.c_str());
            if (position.x > 0.0F && position.x + size.x > width_) {
                position = { 0.0F, position.y + line_height_ };
            }
            const auto begin = text_.size();
            text_.append(cluster);
            glyphs_.push_back({
              .begin = begin,
              .end = text_.size(),
              .position = position,
              .width = size.x,
              .font = font,
              .color = ChatDrawColor(color),
              .text = cluster,
            });
            position.x += size.x;
            offset = end;
        }
    }
    std::size_t HitTest(ImVec2 point) const
    {
        for (const auto& glyph : glyphs_) {
            if (point.y < glyph.position.y) {
                return glyph.begin;
            }
            if (point.y < glyph.position.y + line_height_) {
                if (point.x <= glyph.position.x + (glyph.width * 0.5F)) {
                    return glyph.begin;
                }
                if (point.x <= glyph.position.x + glyph.width) {
                    return glyph.end;
                }
            }
        }
        return text_.size();
    }
    std::string text_;
    std::vector<Glyph> glyphs_;
    std::vector<float> entry_positions_;
    std::size_t revision_{ std::numeric_limits<std::size_t>::max() };
    std::size_t displayed_revision_{};
    std::size_t evictions_{};
    std::size_t anchor_{};
    std::size_t focus_{};
    float width_{};
    float font_size_{};
    float line_height_{};
    float height_{};
    bool selecting_{};
    bool keyboard_focus_{};
    bool unread_{};
};
} // namespace Soldank
