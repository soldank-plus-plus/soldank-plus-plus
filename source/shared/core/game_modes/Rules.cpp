module;

#include <cstddef>
#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>
#include <optional>

export module Shared.Core.GameModes.Rules;

export import Shared.Core.GameModes.GameModeDefinition;
import Shared.Core.Types.TeamType;

export namespace Soldank
{
struct KillScoringSettings
{
    int player_points{ 1 };
    int team_points{};
    bool enemy_only{};
    bool flag_multiplier{};
    bool multikill{};
    bool bow_only{};
};
class KillScoringRule final : public IGameRule
{
public:
    explicit KillScoringRule(KillScoringSettings settings)
        : settings_(settings)
    {
    }
    void ResetPlayer(std::uint8_t player) override
    {
        chains_.at(player) = 0;
        last_kill_.at(player).reset();
    }
    void OnKill(const KillFact& fact,
                const ModeWorldView& view,
                std::vector<ModeAction>& actions) override
    {
        if (!fact.killer || *fact.killer == fact.victim || *fact.killer >= MODE_PLAYERS) {
            return;
        }
        const auto killer = *fact.killer;
        if (!view.players.at(killer).active ||
            view.players.at(killer).generation != fact.killer_generation ||
            (settings_.enemy_only && fact.killer_team == fact.victim_team)) {
            return;
        }
        int points = settings_.player_points;
        if (settings_.bow_only && !IsBow(fact.weapon) && !fact.victim_bow) {
            const bool holder = std::ranges::any_of(
              view.players, [](const auto& player) { return player.active && player.bow; });
            actions.push_back({
              .kind = ModeActionKind::PlayerPoints,
              .target = killer,
              .amount = holder ? -1 : 0,
            });
            return;
        }
        if (settings_.flag_multiplier && fact.killer_flag) {
            points *= 2;
        }
        if (settings_.multikill) {
            auto& chain = chains_.at(killer);
            auto& last = last_kill_.at(killer);
            if (!last || view.tick - *last >= 182U) {
                chain = 0;
            }
            if (last && view.tick - *last < 180U && chain >= 2) {
                points *= static_cast<int>(1U << static_cast<unsigned int>(std::min(chain - 1, 5)));
            }
            chain = std::min(chain + 1, 7);
            last = view.tick;
        }
        actions.push_back(
          { .kind = ModeActionKind::PlayerPoints, .target = killer, .amount = points });
        if (settings_.team_points != 0) {
            actions.push_back({
              .kind = ModeActionKind::TeamPoints,
              .target = TeamIndex(fact.killer_team),
              .amount = settings_.team_points,
            });
        }
    }

private:
    KillScoringSettings settings_;
    std::array<int, MODE_PLAYERS> chains_{};
    std::array<std::optional<std::uint32_t>, MODE_PLAYERS> last_kill_{};
};
RuleSpec KillScoring(KillScoringSettings settings = {})
{
    if (settings.player_points < 0 || settings.player_points > 10000 || settings.team_points < 0 ||
        settings.team_points > 10000) {
        throw std::invalid_argument("Invalid kill score");
    }
    const auto hash = static_cast<std::uint64_t>(settings.player_points) |
                      (static_cast<std::uint64_t>(settings.team_points) << 16U) |
                      (static_cast<std::uint64_t>(settings.enemy_only) << 32U) |
                      (static_cast<std::uint64_t>(settings.flag_multiplier) << 33U) |
                      (static_cast<std::uint64_t>(settings.multikill) << 34U) |
                      (static_cast<std::uint64_t>(settings.bow_only) << 35U);
    return {
        .id = "kill_scoring",
        .create = [settings] { return std::make_unique<KillScoringRule>(settings); },
        .settings_hash = hash,
    };
}
struct CaptureScoringSettings
{
    int points{ 1 };
    bool population_penalty{};
};
class CaptureScoringRule final : public IGameRule
{
public:
    explicit CaptureScoringRule(CaptureScoringSettings settings)
        : settings_(settings)
    {
    }
    void OnCapture(const CaptureFact& fact,
                   const ModeWorldView& view,
                   std::vector<ModeAction>& actions) override
    {
        const auto team = view.players.at(fact.player).team;
        int points = settings_.points;
        if (settings_.population_penalty) {
            points -= 5 * std::max(0,
                                   TeamPopulation(view, TeamType::Alpha) -
                                     TeamPopulation(view, TeamType::Bravo));
        }
        actions.push_back(
          { .kind = ModeActionKind::TeamPoints, .target = TeamIndex(team), .amount = points });
        actions.push_back({
          .kind = ModeActionKind::Capture,
          .target = fact.player,
          .amount = 1,
        });
        actions.push_back(
          { .kind = ModeActionKind::ResetObjective, .target = fact.objective, .amount = 0 });
    }

private:
    CaptureScoringSettings settings_;
};
RuleSpec CaptureScoring(CaptureScoringSettings settings = {})
{
    if (settings.points < 0 || settings.points > 10000) {
        throw std::invalid_argument("Invalid capture score");
    }
    const auto hash = static_cast<std::uint64_t>(settings.points) |
                      (static_cast<std::uint64_t>(settings.population_penalty) << 16U);
    return {
        .id = "capture_scoring",
        .create = [settings] { return std::make_unique<CaptureScoringRule>(settings); },
        .settings_hash = hash,
    };
}
enum class PeriodicScoreKind : std::uint8_t
{
    HomeDefense,
    FlagHolder,
};
struct PeriodicScoringSettings
{
    PeriodicScoreKind kind{};
    std::uint32_t interval{ 300 };
    std::size_t objective{};
};
class PeriodicScoringRule final : public IGameRule
{
public:
    explicit PeriodicScoringRule(PeriodicScoringSettings settings)
        : settings_(settings)
        , interval_(settings.interval)
    {
    }
    void OnTick(const ModeWorldView& view, std::vector<ModeAction>& actions) override
    {
        const int alpha = TeamPopulation(view, TeamType::Alpha);
        const int bravo = TeamPopulation(view, TeamType::Bravo);
        if (alpha == 0 || bravo == 0) {
            return;
        }
        if (settings_.kind == PeriodicScoreKind::HomeDefense) {
            interval_ =
              settings_.interval + (120U * static_cast<unsigned int>(std::max(0, bravo - alpha)));
        } else if (alpha == bravo) {
            interval_ = settings_.interval;
        }
        if (view.tick % interval_ != 0) {
            return;
        }
        for (const auto& objective : view.objectives) {
            if (objective.definition_index != settings_.objective) {
                continue;
            }
            if (settings_.kind == PeriodicScoreKind::HomeDefense) {
                if (objective.home && !objective.carrier) {
                    actions.push_back({
                      .kind = ModeActionKind::TeamPoints,
                      .target = TeamIndex(TeamType::Bravo),
                      .amount = 1,
                    });
                }
            } else if (objective.carrier) {
                const auto team = view.players.at(*objective.carrier).team;
                const int excess = team == TeamType::Alpha ? alpha - bravo : bravo - alpha;
                actions.push_back({
                  .kind = ModeActionKind::TeamPoints,
                  .target = TeamIndex(team),
                  .amount = 1,
                });
                interval_ = 300U + (120U * static_cast<unsigned int>(std::max(0, excess)));
            }
        }
    }

private:
    PeriodicScoringSettings settings_;
    std::uint32_t interval_;
};
RuleSpec PeriodicScoring(PeriodicScoringSettings settings)
{
    if (settings.interval == 0 || settings.interval > 1000000U) {
        throw std::invalid_argument("Invalid periodic score interval");
    }
    const auto hash = static_cast<std::uint64_t>(settings.interval) |
                      (static_cast<std::uint64_t>(settings.kind) << 32U) |
                      (static_cast<std::uint64_t>(settings.objective) << 40U);
    return {
        .id = "periodic_scoring",
        .create = [settings] { return std::make_unique<PeriodicScoringRule>(settings); },
        .settings_hash = hash,
    };
}
} // namespace Soldank
