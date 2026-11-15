module;

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <cstdint>
#include <bit>
#include <utility>
#include <initializer_list>

export module Shared.Core.GameModes.GameModeDefinition;

export import Shared.Core.GameModes.RuleContracts;

export namespace Soldank
{
class IGameRule
{
public:
    virtual ~IGameRule() = default;
    virtual void OnKill(const KillFact& /*fact*/,
                        const ModeWorldView& /*view*/,
                        std::vector<ModeAction>& /*actions*/)
    {
    }
    virtual void OnCapture(const CaptureFact& /*fact*/,
                           const ModeWorldView& /*view*/,
                           std::vector<ModeAction>& /*actions*/)
    {
    }
    virtual void OnTick(const ModeWorldView& /*view*/, std::vector<ModeAction>& /*actions*/) {}
    virtual void ResetPlayer(std::uint8_t /*player*/) {}
};
struct RuleSpec
{
    std::string id;
    std::function<std::unique_ptr<IGameRule>()> create;
    std::uint64_t settings_hash{};
    std::uint32_t version{ 1 };
};
enum class RespawnKind : std::uint8_t
{
    Delayed,
    Waves,
    RoundOnly,
};
struct RespawnPolicy
{
    RespawnKind kind{ RespawnKind::Delayed };
    std::uint32_t delay{ 180 };
    std::uint32_t minimum{ 120 };
    std::uint32_t maximum{ 240 };
};
struct RoundPolicy
{
    bool enabled{};
    bool end_on_capture{};
    int survivor_award{};
    bool infiltration_penalty{};
    std::uint32_t restart_delay{ 300 };
    bool clear_weapons{};
};
struct GameModeSettings
{
    std::string id;
    std::string display_name;
    unsigned int team_count{};
    bool sandbox{};
    bool friendly_fire{};
    bool regenerate_bow{};
    bool captures{};
    int survival_award{};
    bool survival_penalty{};
    std::optional<int> score_limit;
    std::optional<std::uint32_t> time_limit{ 36000 };
    RespawnPolicy respawn;
    RoundPolicy rounds;
    std::vector<ModeObjective> objectives;
    std::vector<RuleSpec> rules;
};
class GameModeDefinition
{
public:
    explicit GameModeDefinition(GameModeSettings settings)
        : settings_(std::make_shared<const GameModeSettings>(std::move(settings)))
    {
    }
    const GameModeSettings& GetSettings() const { return *settings_; }
    std::uint64_t GetFingerprint() const
    {
        std::uint64_t hash = 14695981039346656037ULL;
        const auto add = [&](std::uint64_t value) { hash = (hash ^ value) * 1099511628211ULL; };
        const auto text = [&](const std::string& value) {
            for (const auto character : value) {
                add(static_cast<unsigned char>(character));
            }
            add(0);
        };
        const auto values = [&](std::initializer_list<std::uint64_t> data) {
            for (const auto value : data) {
                add(value);
            }
        };
        const auto& settings = *settings_;
        text(settings.id);
        values({
          settings.team_count,
          static_cast<const std::uint64_t>(settings.sandbox),
          static_cast<const std::uint64_t>(settings.friendly_fire),
          static_cast<const std::uint64_t>(settings.regenerate_bow),
          static_cast<const std::uint64_t>(settings.captures),
          static_cast<std::uint64_t>(settings.score_limit.value_or(0)),
          settings.time_limit.value_or(0),
          static_cast<std::uint64_t>(settings.respawn.kind),
          settings.respawn.delay,
          settings.respawn.minimum,
          settings.respawn.maximum,
          static_cast<const std::uint64_t>(settings.rounds.enabled),
          static_cast<const std::uint64_t>(settings.rounds.end_on_capture),
          static_cast<std::uint64_t>(settings.rounds.survivor_award),
          static_cast<const std::uint64_t>(settings.rounds.infiltration_penalty),
          settings.rounds.restart_delay,
          static_cast<const std::uint64_t>(settings.rounds.clear_weapons),
        });
        for (const auto& objective : settings.objectives) {
            text(objective.id);
            values({
              static_cast<std::uint64_t>(objective.item_type),
              static_cast<std::uint64_t>(objective.team),
              static_cast<const std::uint64_t>(objective.fixed),
              objective.return_ticks,
              std::bit_cast<std::uint32_t>(objective.home_radius),
              std::bit_cast<std::uint32_t>(objective.capture_radius),
            });
        }
        for (const auto& rule : settings.rules) {
            text(rule.id);
            values({ rule.settings_hash, rule.version });
        }
        return hash;
    }

private:
    std::shared_ptr<const GameModeSettings> settings_;
};
} // namespace Soldank
