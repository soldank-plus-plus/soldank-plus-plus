module;

#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>

export module Shared.Core.GameModes.BuiltinGameModes;

export import Shared.Core.GameModes.GameModeBuilder;
export import Shared.Core.GameModes.Rules;
import Shared.Core.Types.TeamType;
import Shared.Core.Types.ItemType;

export namespace Soldank
{
class GameModeRegistry
{
public:
    void Register(std::string id, std::function<GameModeDefinition()> factory)
    {
        if (!factory || factories_.contains(id)) {
            throw std::invalid_argument("Invalid or duplicate game mode registration");
        }
        factories_.emplace(std::move(id), std::move(factory));
    }
    GameModeDefinition Create(const std::string& id) const
    {
        const auto found = factories_.find(id);
        if (found == factories_.end()) {
            throw std::invalid_argument("Unknown game mode: " + id);
        }
        return found->second();
    }

private:
    std::map<std::string, std::function<GameModeDefinition()>> factories_;
};
GameModeDefinition MakeSandbox()
{
    return GameModeBuilder("sandbox").DisplayName("Sandbox").Sandbox().FriendlyFire(true).Build();
}
GameModeDefinition MakeDeathmatch()
{
    return GameModeBuilder("deathmatch")
      .DisplayName("Deathmatch")
      .AddRule(KillScoring())
      .ScoreLimit(30)
      .Build();
}
GameModeDefinition MakePointmatch()
{
    return GameModeBuilder("pointmatch")
      .DisplayName("Pointmatch")
      .Objective({ .id = "yellow", .item_type = ItemType::PointmatchFlag })
      .AddRule(KillScoring({ .flag_multiplier = true, .multikill = true }))
      .ScoreLimit(30)
      .Build();
}
GameModeDefinition MakeTeammatch()
{
    return GameModeBuilder("teammatch")
      .DisplayName("Teammatch")
      .Teams(4)
      .AddRule(KillScoring({ .team_points = 1, .enemy_only = true }))
      .Respawn({ .kind = RespawnKind::Waves })
      .ScoreLimit(60)
      .Build();
}
GameModeDefinition MakeCaptureTheFlag()
{
    return GameModeBuilder("capture_the_flag")
      .DisplayName("Capture the Flag")
      .Teams(2)
      .Objective({ .id = "alpha", .item_type = ItemType::AlphaFlag, .team = TeamType::Alpha })
      .Objective({ .id = "bravo", .item_type = ItemType::BravoFlag, .team = TeamType::Bravo })
      .AddRule(KillScoring({ .enemy_only = true }))
      .Captures()
      .AddRule(CaptureScoring())
      .Respawn({ .kind = RespawnKind::Waves })
      .ScoreLimit(10)
      .Build();
}
GameModeDefinition MakeRambomatch()
{
    return GameModeBuilder("rambomatch")
      .DisplayName("Rambomatch")
      .Objective({ .id = "bow", .item_type = ItemType::Bow })
      .RegenerateBow()
      .AddRule(KillScoring({ .bow_only = true }))
      .ScoreLimit(30)
      .Build();
}
GameModeDefinition MakeInfiltration()
{
    return GameModeBuilder("infiltration")
      .DisplayName("Infiltration")
      .Teams(2)
      .Objective({
        .id = "destination",
        .item_type = ItemType::AlphaFlag,
        .team = TeamType::Alpha,
        .fixed = true,
      })
      .Objective({ .id = "objective", .item_type = ItemType::BravoFlag, .team = TeamType::Bravo })
      .AddRule(KillScoring({ .enemy_only = true }))
      .Captures(30, true)
      .AddRule(CaptureScoring({ .points = 30, .population_penalty = true }))
      .AddRule(PeriodicScoring({ .kind = PeriodicScoreKind::HomeDefense, .objective = 1 }))
      .Respawn({ .kind = RespawnKind::Waves })
      .ScoreLimit(90)
      .Build();
}
GameModeDefinition MakeHoldTheFlag()
{
    return GameModeBuilder("hold_the_flag")
      .DisplayName("Hold the Flag")
      .Teams(2)
      .Objective({ .id = "yellow", .item_type = ItemType::PointmatchFlag })
      .AddRule(KillScoring({ .enemy_only = true }))
      .AddRule(PeriodicScoring({ .kind = PeriodicScoreKind::FlagHolder }))
      .Respawn({ .kind = RespawnKind::Waves })
      .ScoreLimit(80)
      .Build();
}
GameModeRegistry MakeBuiltinGameModeRegistry()
{
    GameModeRegistry registry;
    registry.Register("sandbox", MakeSandbox);
    registry.Register("deathmatch", MakeDeathmatch);
    registry.Register("pointmatch", MakePointmatch);
    registry.Register("teammatch", MakeTeammatch);
    registry.Register("capture_the_flag", MakeCaptureTheFlag);
    registry.Register("rambomatch", MakeRambomatch);
    registry.Register("infiltration", MakeInfiltration);
    registry.Register("hold_the_flag", MakeHoldTheFlag);
    return registry;
}
GameModeDefinition WithSurvival(const GameModeDefinition& definition, bool clear_weapons = false)
{
    const auto& settings = definition.GetSettings();
    if (settings.id == "pointmatch" || settings.sandbox) {
        throw std::invalid_argument("This mode does not support Survival");
    }
    return GameModeBuilder(definition)
      .Respawn({ .kind = RespawnKind::RoundOnly })
      .Rounds({
        .enabled = true,
        .end_on_capture = settings.captures,
        .survivor_award = settings.survival_award,
        .infiltration_penalty = settings.survival_penalty,
        .clear_weapons = clear_weapons,
      })
      .Build();
}
} // namespace Soldank
