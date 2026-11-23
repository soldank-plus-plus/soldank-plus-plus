module;
#include <algorithm>
#include <cstdint>
#include <array>
#include <optional>
#include <string_view>

export module Shared.Core.Animations.EmoteTypes;
import Shared.Core.Types.WeaponType;
import Extern.Glm;

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
constexpr unsigned int EMOTE_CUE_COUNT = 11;
constexpr std::uint16_t MAX_EMOTE_CUE_AGE = 600;

// Light, first puff, second puff, and discard use positions from the simulation
// frame that emitted the cue, including when a snapshot arrives after the pose changed.
struct CigarVisualOrigins
{
    std::array<glm::vec2, 4> positions{};
    glm::vec2 match_position{};
    std::array<std::int8_t, 4> directions{};
    std::uint8_t valid_mask{};
    bool operator==(const CigarVisualOrigins& other) const
    {
        return std::equal(positions.begin(), positions.end(), other.positions.begin()) &&
               match_position == other.match_position &&
               std::equal(directions.begin(), directions.end(), other.directions.begin()) &&
               valid_mask == other.valid_mask;
    }
};
struct EmoteProgress
{
    std::uint32_t instance{};
    std::uint32_t cues{};
    std::uint8_t phase{};
    WeaponType weapon{ WeaponType::NoWeapon };
    std::array<std::uint16_t, EMOTE_CUE_COUNT> cue_ages{};
    CigarVisualOrigins cigar_origins{};
    bool operator==(const EmoteProgress& other) const
    {
        return instance == other.instance && cues == other.cues && phase == other.phase &&
               weapon == other.weapon &&
               std::equal(cue_ages.begin(), cue_ages.end(), other.cue_ages.begin()) &&
               cigar_origins == other.cigar_origins;
    }
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
