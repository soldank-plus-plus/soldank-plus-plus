module;

#include <cstddef>
#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

export module Shared.Core.GameModes.GameModeRuntime;

export import Shared.Core.GameModes.GameModeDefinition;
import Shared.Core.Types.TeamType;

export namespace Soldank
{
class GameModeRuntime
{
public:
    explicit GameModeRuntime(GameModeDefinition definition, std::uint32_t epoch = 1)
        : definition_(std::move(definition))
    {
        state_.epoch = epoch;
        for (const auto& spec : Settings().rules) {
            rules_.push_back(spec.create());
        }
    }
    const GameModeDefinition& GetDefinition() const { return definition_; }
    const GameModeSettings& Settings() const { return definition_.GetSettings(); }
    const MatchState& GetState() const { return state_; }
    void ApplySnapshot(const MatchState& snapshot) { state_ = snapshot; }
    void ObserveRoster(const ModeWorldView& view)
    {
        for (std::size_t i = 0; i < MODE_PLAYERS; ++i) {
            const auto& current = view.players.at(i);
            const auto& previous = roster_.at(i);
            if (current.active != previous.active || current.generation != previous.generation ||
                current.team != previous.team) {
                for (auto& rule : rules_) {
                    rule->ResetPlayer(static_cast<std::uint8_t>(i));
                }
                if (current.generation != previous.generation || !current.active) {
                    state_.players.at(i) = {};
                }
                if (Settings().rounds.enabled && state_.phase == MatchPhase::Running) {
                    round_roster_.at(i) = false;
                }
                ++state_.revision;
            }
        }
        roster_ = view.players;
        if (state_.phase == MatchPhase::Waiting && Ready(view)) {
            state_.phase = MatchPhase::Running;
            StartRound(view);
            ++state_.revision;
        }
    }
    bool IsRoundParticipant(std::uint8_t player) const
    {
        return !Settings().rounds.enabled || round_roster_.at(player);
    }
    void PenalizeAnimatedSuicide(std::uint8_t player)
    {
        auto& score = state_.players.at(player);
        if (score.kills > 0) {
            --score.kills;
            ++state_.revision;
        }
    }
    void OnKill(const KillFact& fact, const ModeWorldView& view)
    {
        if (state_.phase != MatchPhase::Running || fact.victim >= MODE_PLAYERS) {
            return;
        }
        ++state_.players.at(fact.victim).deaths;
        if (fact.killer && *fact.killer != fact.victim && *fact.killer < MODE_PLAYERS &&
            view.players.at(*fact.killer).active &&
            view.players.at(*fact.killer).generation == fact.killer_generation &&
            (Settings().team_count == 0 || fact.killer_team != fact.victim_team)) {
            ++state_.players.at(*fact.killer).kills;
        }
        for (auto& rule : rules_) {
            rule->OnKill(fact, view, actions_);
            rule->ResetPlayer(fact.victim);
        }
        ++state_.revision;
    }
    void OnCapture(const CaptureFact& fact, const ModeWorldView& view)
    {
        if (state_.phase != MatchPhase::Running || fact.player >= MODE_PLAYERS ||
            fact.objective >= Settings().objectives.size()) {
            return;
        }
        for (auto& rule : rules_) {
            rule->OnCapture(fact, view, actions_);
        }
        captured_ = captured_ || Settings().rounds.end_on_capture;
    }
    std::vector<ModeAction> EndTick(const ModeWorldView& view)
    {
        std::vector<ModeAction> effects;
        if (state_.phase == MatchPhase::RoundEnding) {
            ++state_.elapsed_ticks;
            EvaluateLimits(view);
            return effects;
        }
        if (state_.phase != MatchPhase::Running) {
            return effects;
        }
        ++state_.elapsed_ticks;
        for (auto& rule : rules_) {
            rule->OnTick(view, actions_);
        }
        ApplyActions(effects);
        if (Settings().rounds.enabled) {
            EvaluateRound(view);
            ApplyActions(effects);
        }
        EvaluateLimits(view);
        return effects;
    }
    bool RestartDue(std::uint32_t tick) const
    {
        return state_.phase == MatchPhase::RoundEnding &&
               static_cast<std::int32_t>(tick - state_.round_deadline) >= 0;
    }
    void RestartRound(const ModeWorldView& view)
    {
        if (state_.phase != MatchPhase::RoundEnding) {
            return;
        }
        state_.phase = Ready(view) ? MatchPhase::Running : MatchPhase::Waiting;
        StartRound(view);
        ++state_.revision;
    }
    std::uint32_t RespawnDelay(const ModeWorldView& view) const
    {
        const auto& policy = Settings().respawn;
        if (policy.kind != RespawnKind::Waves) {
            return policy.delay;
        }
        unsigned int population = 0;
        for (const auto& player : view.players) {
            population += player.active ? 1U : 0U;
        }
        const auto candidate = population * 60U;
        const auto total = candidate > policy.minimum ? policy.maximum : candidate;
        const auto period = total > policy.minimum ? total - policy.minimum : 1U;
        return policy.minimum + period - (view.tick % period);
    }

private:
    bool Ready(const ModeWorldView& view) const
    {
        // Continuous matches run even on an empty server. Only Survival needs opponents
        // before starting a round so it cannot award uncontested elimination wins.
        if (!Settings().rounds.enabled || Settings().sandbox) {
            return true;
        }
        std::array<bool, MODE_TEAMS> teams{};
        int participants = 0;
        for (const auto& player : view.players) {
            if (player.active && player.team != TeamType::Spectator) {
                ++participants;
                teams.at(TeamIndex(player.team)) = true;
            }
        }
        if (Settings().team_count == 0) {
            return participants >= 2;
        }
        teams.at(0) = false;
        return std::ranges::count(teams, true) >= 2;
    }
    void StartRound(const ModeWorldView& view)
    {
        ++state_.round_number;
        captured_ = false;
        for (std::size_t i = 0; i < MODE_PLAYERS; ++i) {
            round_roster_.at(i) =
              view.players.at(i).active && view.players.at(i).team != TeamType::Spectator;
            for (auto& rule : rules_) {
                rule->ResetPlayer(static_cast<std::uint8_t>(i));
            }
        }
    }
    static void AddPoints(int& score, int amount)
    {
        const auto total = static_cast<std::int64_t>(score) + amount;
        score =
          static_cast<int>(std::clamp<std::int64_t>(total, 0, std::numeric_limits<int>::max()));
    }
    void ApplyActions(std::vector<ModeAction>& effects)
    {
        for (const auto& action : actions_) {
            switch (action.kind) {
                case ModeActionKind::PlayerPoints:
                    AddPoints(state_.players.at(action.target).points, action.amount);
                    break;
                case ModeActionKind::TeamPoints:
                    AddPoints(state_.teams.at(action.target), action.amount);
                    break;
                case ModeActionKind::Capture:
                    AddPoints(state_.players.at(action.target).captures, action.amount);
                    break;
                case ModeActionKind::ResetObjective:
                    effects.push_back(action);
                    break;
            }
            ++state_.revision;
        }
        actions_.clear();
    }
    void EvaluateRound(const ModeWorldView& view)
    {
        std::array<bool, MODE_TEAMS> alive_teams{};
        int alive = 0;
        for (std::size_t i = 0; i < MODE_PLAYERS; ++i) {
            const auto& player = view.players.at(i);
            if (round_roster_.at(i) && player.active && player.alive &&
                player.team != TeamType::Spectator) {
                ++alive;
                alive_teams.at(TeamIndex(player.team)) = true;
            }
        }
        const bool teams = Settings().team_count > 0;
        const auto remaining =
          teams ? std::ranges::count(alive_teams, true) - static_cast<int>(alive_teams.at(0))
                : alive;
        if (!captured_ && remaining > 1) {
            return;
        }
        const auto& round = Settings().rounds;
        if (!captured_ && teams) {
            for (std::size_t team = 1; team < MODE_TEAMS; ++team) {
                if (alive_teams.at(team) && (!round.infiltration_penalty || team == 1)) {
                    actions_.push_back({
                      .kind = ModeActionKind::TeamPoints,
                      .target = team,
                      .amount = round.survivor_award,
                    });
                }
            }
            if (round.infiltration_penalty) {
                actions_.push_back({
                  .kind = ModeActionKind::TeamPoints,
                  .target = 1,
                  .amount = -5 * std::max(0,
                                          TeamPopulation(view, TeamType::Alpha) -
                                            TeamPopulation(view, TeamType::Bravo)),
                });
            }
        }
        state_.phase = MatchPhase::RoundEnding;
        state_.round_deadline = view.tick + round.restart_delay;
        ++state_.revision;
    }
    void EvaluateLimits(const ModeWorldView& view)
    {
        const bool timed_out =
          Settings().time_limit && state_.elapsed_ticks >= *Settings().time_limit;
        bool score_reached = false;
        int best = 0;
        if (Settings().team_count > 0) {
            for (std::size_t team = 1; team <= Settings().team_count; ++team) {
                best = std::max(best, state_.teams.at(team));
            }
        } else {
            for (std::size_t i = 0; i < MODE_PLAYERS; ++i) {
                if (view.players.at(i).active && view.players.at(i).team != TeamType::Spectator) {
                    best = std::max(best, state_.players.at(i).points);
                }
            }
        }
        score_reached = Settings().score_limit && best >= *Settings().score_limit;
        if (!timed_out && !score_reached) {
            return;
        }
        state_.phase = MatchPhase::Finished;
        for (std::size_t i = 0; i < MODE_PLAYERS; ++i) {
            state_.winners.at(i) = Settings().team_count == 0 && view.players.at(i).active &&
                                   view.players.at(i).team != TeamType::Spectator &&
                                   state_.players.at(i).points == best;
        }
        for (std::size_t team = 1; team <= Settings().team_count; ++team) {
            state_.winning_teams.at(team) = state_.teams.at(team) == best &&
                                            TeamPopulation(view, static_cast<TeamType>(team)) > 0;
        }
        ++state_.revision;
    }
    GameModeDefinition definition_;
    MatchState state_;
    std::vector<std::unique_ptr<IGameRule>> rules_;
    std::vector<ModeAction> actions_;
    std::array<ModeParticipant, MODE_PLAYERS> roster_{};
    std::array<bool, MODE_PLAYERS> round_roster_{};
    bool captured_{};
};
} // namespace Soldank
