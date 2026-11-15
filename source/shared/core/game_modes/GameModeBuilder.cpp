module;

#include <algorithm>
#include <cstdint>
#include <limits>
#include <cmath>
#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

export module Shared.Core.GameModes.GameModeBuilder;

export import Shared.Core.GameModes.GameModeDefinition;

export namespace Soldank
{
class GameModeBuilder
{
public:
    explicit GameModeBuilder(std::string id) { settings_.id = std::move(id); }
    explicit GameModeBuilder(const GameModeDefinition& definition)
        : settings_(definition.GetSettings())
    {
    }
    GameModeBuilder& DisplayName(std::string name)
    {
        settings_.display_name = std::move(name);
        return *this;
    }
    GameModeBuilder& Teams(unsigned int count)
    {
        settings_.team_count = count;
        return *this;
    }
    GameModeBuilder& Sandbox()
    {
        settings_.sandbox = true;
        settings_.time_limit.reset();
        return *this;
    }
    GameModeBuilder& FriendlyFire(bool enabled)
    {
        settings_.friendly_fire = enabled;
        return *this;
    }
    GameModeBuilder& RegenerateBow()
    {
        settings_.regenerate_bow = true;
        return *this;
    }
    GameModeBuilder& Captures(int survival_award = 1, bool survival_penalty = false)
    {
        settings_.captures = true;
        settings_.survival_award = survival_award;
        settings_.survival_penalty = survival_penalty;
        return *this;
    }
    GameModeBuilder& Objective(ModeObjective objective)
    {
        settings_.objectives.push_back(std::move(objective));
        return *this;
    }
    GameModeBuilder& AddRule(RuleSpec spec)
    {
        settings_.rules.push_back(std::move(spec));
        return *this;
    }
    GameModeBuilder& Respawn(RespawnPolicy policy)
    {
        settings_.respawn = policy;
        return *this;
    }
    GameModeBuilder& Rounds(RoundPolicy policy)
    {
        settings_.rounds = policy;
        return *this;
    }
    GameModeBuilder& ScoreLimit(std::optional<int> limit)
    {
        settings_.score_limit = limit;
        return *this;
    }
    GameModeBuilder& TimeLimit(std::optional<std::uint32_t> ticks)
    {
        settings_.time_limit = ticks;
        return *this;
    }
    template<typename Rule, typename Settings>
    GameModeBuilder& AddRule(std::string id, Settings settings)
    {
        return AddRule({ std::move(id), [settings] { return std::make_unique<Rule>(settings); } });
    }
    GameModeDefinition Build() const
    {
        ValidateIdentity();
        ValidateTiming();
        ValidateObjectives();
        ValidateRules();
        return GameModeDefinition(settings_);
    }

private:
    void ValidateIdentity() const
    {
        if (settings_.id.empty() || settings_.id.size() > 63 ||
            (settings_.team_count != 0 && settings_.team_count != 2 && settings_.team_count != 4)) {
            throw std::invalid_argument("Invalid game mode identity or team layout");
        }
        if (settings_.objectives.size() > 4 ||
            !std::ranges::all_of(settings_.id, [](char character) {
                return (character >= 'a' && character <= 'z') ||
                       (character >= '0' && character <= '9') || character == '_';
            })) {
            throw std::invalid_argument("Invalid mode ID or too many objectives");
        }
    }
    void ValidateTiming() const
    {
        if ((settings_.score_limit && *settings_.score_limit <= 0) ||
            (settings_.time_limit &&
             (*settings_.time_limit == 0 || *settings_.time_limit > 1000000000U)) ||
            settings_.respawn.maximum < settings_.respawn.minimum ||
            settings_.respawn.maximum > 1000000U || settings_.respawn.delay > 1000000U ||
            settings_.rounds.restart_delay == 0 || settings_.rounds.restart_delay > 1000000U) {
            throw std::invalid_argument("Invalid game mode limits or respawn timing");
        }
        if (settings_.rounds.enabled != (settings_.respawn.kind == RespawnKind::RoundOnly)) {
            throw std::invalid_argument("Survival requires a round-only respawn policy");
        }
    }
    void ValidateObjectives() const
    {
        for (std::size_t i = 0; i < settings_.objectives.size(); ++i) {
            const auto& objective = settings_.objectives.at(i);
            if (objective.return_ticks == 0 || objective.return_ticks > 1000000U ||
                !std::isfinite(objective.home_radius) || objective.home_radius <= 0 ||
                !std::isfinite(objective.capture_radius) || objective.capture_radius <= 0 ||
                objective.id.empty() || (static_cast<std::size_t>(objective.team) >= MODE_TEAMS) ||
                TeamIndex(objective.team) > settings_.team_count ||
                std::ranges::any_of(
                  settings_.objectives.begin() + static_cast<std::ptrdiff_t>(i + 1),
                  settings_.objectives.end(),
                  [&](const auto& other) {
                      return other.id == objective.id || other.item_type == objective.item_type;
                  })) {
                throw std::invalid_argument("Invalid or duplicate objective");
            }
        }
    }
    void ValidateRules() const
    {
        for (std::size_t i = 0; i < settings_.rules.size(); ++i) {
            const auto& rule = settings_.rules.at(i);
            if (rule.id.empty() || !rule.create ||
                std::ranges::any_of(settings_.rules.begin() + static_cast<std::ptrdiff_t>(i + 1),
                                    settings_.rules.end(),
                                    [&](const auto& other) { return other.id == rule.id; })) {
                throw std::invalid_argument("Invalid or duplicate rule");
            }
        }
    }
    GameModeSettings settings_;
};
} // namespace Soldank
