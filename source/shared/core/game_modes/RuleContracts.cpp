module;

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

export module Shared.Core.GameModes.RuleContracts;

import Shared.Core.Types.TeamType;
import Shared.Core.Types.ItemType;
import Shared.Core.Types.WeaponType;

export namespace Soldank
{
constexpr std::size_t MODE_PLAYERS = 32;
constexpr std::size_t MODE_TEAMS = 5;

enum class MatchPhase : std::uint8_t
{
    Waiting,
    Running,
    RoundEnding,
    Finished,
};
enum class ObjectiveInteraction : std::uint8_t
{
    Ignore,
    Carry,
    Return,
};
enum class ModeActionKind : std::uint8_t
{
    PlayerPoints,
    TeamPoints,
    Capture,
    ResetObjective,
};
struct ModeParticipant
{
    bool active{};
    bool alive{};
    TeamType team{};
    std::uint32_t generation{};
    bool bow{};
    bool carrying_flag{};
};
struct ModeObjective
{
    std::string id;
    ItemType item_type{ ItemType::AlphaFlag };
    TeamType team{};
    bool fixed{};
    std::uint32_t return_ticks{ 1500 };
    float home_radius{ 75.0F };
    float capture_radius{ 28.0F };
};
struct ObjectiveView
{
    std::size_t definition_index{};
    std::uint8_t item_id{};
    std::optional<std::uint8_t> carrier;
    bool home{};
};
struct ModeWorldView
{
    std::uint32_t tick{};
    std::array<ModeParticipant, MODE_PLAYERS> players{};
    std::vector<ObjectiveView> objectives;
};
struct KillFact
{
    std::uint8_t victim{};
    std::optional<std::uint8_t> killer;
    std::uint32_t killer_generation{};
    TeamType killer_team{};
    TeamType victim_team{};
    WeaponType weapon{};
    bool victim_bow{};
    bool killer_flag{};
};
struct CaptureFact
{
    std::uint8_t player{};
    std::size_t objective{};
};
struct ModeAction
{
    ModeActionKind kind{};
    std::size_t target{};
    int amount{};
};
struct PlayerScore
{
    int points{};
    int kills{};
    int deaths{};
    int captures{};
};
struct MatchState
{
    std::uint32_t epoch{};
    std::uint32_t revision{};
    std::uint32_t elapsed_ticks{};
    std::uint32_t round_deadline{};
    std::uint32_t round_number{};
    MatchPhase phase{ MatchPhase::Waiting };
    std::array<PlayerScore, MODE_PLAYERS> players{};
    std::array<int, MODE_TEAMS> teams{};
    std::array<bool, MODE_PLAYERS> winners{};
    std::array<bool, MODE_TEAMS> winning_teams{};
};

std::size_t TeamIndex(TeamType team)
{
    const auto index = static_cast<std::size_t>(team);
    return index < MODE_TEAMS ? index : 0;
}
bool IsBow(WeaponType weapon)
{
    return weapon == WeaponType::Bow || weapon == WeaponType::FlameBow;
}
int TeamPopulation(const ModeWorldView& view, TeamType team)
{
    int count = 0;
    for (const auto& player : view.players) {
        if (player.active && player.team == team) {
            ++count;
        }
    }
    return count;
}
} // namespace Soldank
