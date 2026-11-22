module;
#include <cstdint>
#include <optional>
#include <string_view>

export module Shared.Core.Animations.EmoteTypes;
import Shared.Core.Types.WeaponType;

export namespace Soldank
{
enum class Emote : std::int8_t
{
    None = -1,
    Tobacco,
    Smoke,
    Wipe,
    Groin,
    TakeOff,
    Victory,
    Piss,
    Mercy,
    Own,
};
constexpr int DEFAULT_IDLE_TICKS = 60 * 8;
constexpr int LONGER_IDLE_TICKS = 60 * 30;

// Milestones are cumulative within an instance, so snapshots can skip animation frames.
enum class EmoteCue : std::uint32_t
{
    Stuff = 1U << 0,
    Spit = 1U << 1,
    Match = 1U << 2,
    Light = 1U << 3,
    PuffFirst = 1U << 4,
    PuffSecond = 1U << 5,
    Discard = 1U << 6,
    Roar = 1U << 7,
    Piss = 1U << 8,
    Mercy = 1U << 9,
    MercyFire = 1U << 10,
};
struct EmoteProgress
{
    std::uint32_t instance{};
    std::uint32_t cues{};
    std::uint8_t phase{};
    WeaponType weapon{ WeaponType::NoWeapon };
    bool operator==(const EmoteProgress&) const = default;
};
constexpr bool IsValidEmote(Emote emote)
{
    return emote >= Emote::Tobacco && emote <= Emote::Own;
}
std::optional<Emote> ParseEmoteCommand(std::string_view command)
{
    if (command == "/tabac")
        return Emote::Tobacco;
    if (command == "/smoke")
        return Emote::Smoke;
    if (command == "/takeoff")
        return Emote::TakeOff;
    if (command == "/victory")
        return Emote::Victory;
    if (command == "/piss")
        return Emote::Piss;
    if (command == "/mercy")
        return Emote::Mercy;
    if (command == "/pwn")
        return Emote::Own;
    return std::nullopt;
}
} // namespace Soldank
