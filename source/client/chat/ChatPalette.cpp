module;
#include <array>
#include <cstdint>
#include <string_view>

export module Chat.Palette;
import Shared.Networking.ChatProtocol;

export namespace Soldank
{
constexpr std::uint32_t CHAT_PUBLIC_COLOR = 0xEEEFFEEA;
constexpr std::uint32_t CHAT_TEAM_COLOR = 0xEEFEDA7C;
constexpr std::uint32_t CHAT_SYSTEM_COLOR = 0xF9FBDA22;

std::array<float, 4> ChatArgb(std::uint32_t color)
{
    return {
        static_cast<float>((color >> 16U) & 255U) / 255.0F,
        static_cast<float>((color >> 8U) & 255U) / 255.0F,
        static_cast<float>(color & 255U) / 255.0F,
        static_cast<float>((color >> 24U) & 255U) / 255.0F,
    };
}
std::uint32_t ChatTeamColor(TeamType team)
{
    switch (team) {
        case TeamType::Alpha:
            return 0xFFDF3131;
        case TeamType::Bravo:
            return 0xFF3131DF;
        case TeamType::Charlie:
            return 0xFFDFDF31;
        case TeamType::Delta:
            return 0xFF31DF31;
        default:
            return CHAT_PUBLIC_COLOR;
    }
}
std::string_view ChatLabel(ChatKind kind, ChatChannel channel)
{
    if (kind == ChatKind::System) {
        return "[System]";
    }
    return channel == ChatChannel::Team ? "[Team]" : "[All]";
}
std::uint32_t ChatBodyColor(ChatKind kind, ChatChannel channel)
{
    if (kind == ChatKind::System) {
        return CHAT_SYSTEM_COLOR;
    }
    return channel == ChatChannel::Team ? CHAT_TEAM_COLOR : CHAT_PUBLIC_COLOR;
}
} // namespace Soldank
