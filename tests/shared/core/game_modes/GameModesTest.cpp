#include <optional>
#include <cstddef>
#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

import Shared.Core.GameModes.BuiltinGameModes;
import Shared.Core.GameModes.GameModeRuntime;
import Shared.Core.Types.TeamType;
import Shared.Core.Types.WeaponType;

using namespace Soldank;

namespace
{
ModeWorldView Duel(bool teams = false)
{
    ModeWorldView view;
    view.players.at(0) = {
        .active = true,
        .alive = true,
        .team = teams ? TeamType::Alpha : TeamType::None,
        .generation = 1,
    };
    view.players.at(1) = {
        .active = true,
        .alive = true,
        .team = teams ? TeamType::Bravo : TeamType::None,
        .generation = 1,
    };
    return view;
}
KillFact Kill(const ModeWorldView& view)
{
    return {
        .victim = 1,
        .killer = 0,
        .killer_generation = 1,
        .killer_team = view.players.at(0).team,
        .victim_team = view.players.at(1).team,
    };
}
} // namespace

TEST(GameModesTest, RegistersAllModesAndRejectsInvalidDefinitions)
{
    const auto registry = MakeBuiltinGameModeRegistry();
    for (const auto* id : {
           "deathmatch",
           "pointmatch",
           "teammatch",
           "capture_the_flag",
           "rambomatch",
           "infiltration",
           "hold_the_flag",
         }) {
        EXPECT_EQ(registry.Create(id).GetSettings().id, id);
    }
}

TEST(GameModesTest, RejectsUnknownMode)
{
    EXPECT_THROW(MakeBuiltinGameModeRegistry().Create("unknown"), std::invalid_argument);
}

TEST(GameModesTest, ContinuousModesRunWithNoPlayers)
{
    const auto registry = MakeBuiltinGameModeRegistry();
    for (const auto* id : { "deathmatch",
                            "pointmatch",
                            "teammatch",
                            "capture_the_flag",
                            "rambomatch",
                            "infiltration",
                            "hold_the_flag" }) {
        SCOPED_TRACE(id);
        GameModeRuntime mode(registry.Create(id));
        ModeWorldView view;
        mode.ObserveRoster(view);
        mode.EndTick(view);
        EXPECT_EQ(mode.GetState().phase, MatchPhase::Running);
        EXPECT_EQ(mode.GetState().elapsed_ticks, 1U);
    }
}

TEST(GameModesTest, CaptureTheFlagAllowsSoloCaptureWithoutRestartingOnRosterChanges)
{
    GameModeRuntime mode(MakeCaptureTheFlag());
    ModeWorldView view;
    mode.ObserveRoster(view);
    mode.EndTick(view);
    const auto round_number = mode.GetState().round_number;
    view.players.at(0) = { .active = true, .alive = true, .team = TeamType::Alpha };
    mode.ObserveRoster(view);
    mode.OnCapture({ .player = 0, .objective = 1 }, view);
    mode.EndTick(view);
    EXPECT_EQ(mode.GetState().teams.at(1), 1);
    EXPECT_EQ(mode.GetState().players.at(0).captures, 1);
    view.players.at(0) = {};
    mode.ObserveRoster(view);
    mode.EndTick(view);
    EXPECT_EQ(mode.GetState().phase, MatchPhase::Running);
    EXPECT_EQ(mode.GetState().elapsed_ticks, 3U);
    EXPECT_EQ(mode.GetState().round_number, round_number);
    EXPECT_EQ(mode.GetState().teams.at(1), 1);
}

TEST(GameModesTest, EmptyContinuousMatchStillReachesTimeLimit)
{
    GameModeRuntime mode(GameModeBuilder(MakeCaptureTheFlag()).TimeLimit(2).Build());
    ModeWorldView view;
    mode.ObserveRoster(view);
    mode.EndTick(view);
    mode.EndTick(view);
    EXPECT_EQ(mode.GetState().phase, MatchPhase::Finished);
    EXPECT_EQ(mode.GetState().elapsed_ticks, 2U);
}

TEST(GameModesTest, SurvivalStillRequiresOpponentsBeforeAwardingWins)
{
    GameModeRuntime mode(WithSurvival(MakeCaptureTheFlag()));
    ModeWorldView view;
    mode.ObserveRoster(view);
    mode.EndTick(view);
    EXPECT_EQ(mode.GetState().phase, MatchPhase::Waiting);
    view.players.at(0) = { .active = true, .alive = true, .team = TeamType::Alpha };
    mode.ObserveRoster(view);
    mode.EndTick(view);
    EXPECT_EQ(mode.GetState().phase, MatchPhase::Waiting);
    EXPECT_EQ(mode.GetState().teams.at(1), 0);
    view.players.at(1) = { .active = true, .alive = true, .team = TeamType::Bravo };
    mode.ObserveRoster(view);
    mode.EndTick(view);
    EXPECT_EQ(mode.GetState().phase, MatchPhase::Running);
}

TEST(GameModesTest, RejectsInvalidTeamCount)
{
    EXPECT_THROW(GameModeBuilder("bad").Teams(3).Build(), std::invalid_argument);
}

TEST(GameModesTest, RejectsZeroScoreLimit)
{
    EXPECT_THROW(GameModeBuilder("bad").ScoreLimit(0).Build(), std::invalid_argument);
}

TEST(GameModesTest, RejectsZeroPeriodicInterval)
{
    EXPECT_THROW(PeriodicScoring({ .interval = 0 }), std::invalid_argument);
}

TEST(GameModesTest, RejectsDuplicateRule)
{
    EXPECT_THROW(GameModeBuilder("bad").AddRule(KillScoring()).AddRule(KillScoring()).Build(),
                 std::invalid_argument);
}

TEST(GameModesTest, RejectsUnsupportedSurvival)
{
    EXPECT_THROW(WithSurvival(MakePointmatch()), std::invalid_argument);
}

TEST(GameModesTest, DeathmatchCreditsKillsOnceThroughCommittedFacts)
{
    GameModeRuntime mode(MakeDeathmatch());
    auto const view = Duel();
    mode.ObserveRoster(view);
    mode.OnKill(Kill(view), view);
    mode.OnKill({ .victim = 0, .killer = 0 }, view);
    mode.EndTick(view);
    EXPECT_EQ(mode.GetState().players.at(0).points, 1);
    EXPECT_EQ(mode.GetState().players.at(0).kills, 1);
    EXPECT_EQ(mode.GetState().players.at(0).deaths, 1);
    EXPECT_EQ(mode.GetState().players.at(1).deaths, 1);
}

TEST(GameModesTest, TeammatchSeparatesPersonalAndTeamScores)
{
    GameModeRuntime mode(MakeTeammatch());
    auto const view = Duel(true);
    mode.ObserveRoster(view);
    mode.OnKill(Kill(view), view);
    mode.EndTick(view);
    EXPECT_EQ(mode.GetState().teams.at(1), 1);
    EXPECT_EQ(mode.GetState().players.at(0).points, 1);
    auto friendly = Kill(view);
    friendly.victim_team = TeamType::Alpha;
    mode.OnKill(friendly, view);
    mode.EndTick(view);
    EXPECT_EQ(mode.GetState().teams.at(1), 1);
}

TEST(GameModesTest, PointmatchUsesPreviousChainCountAndFlagMultiplier)
{
    GameModeRuntime mode(MakePointmatch());
    auto view = Duel();
    mode.ObserveRoster(view);
    const std::array<int, 8> awards{ 1, 1, 2, 4, 8, 16, 32, 32 };
    int total = 0;
    // Remove the match limit so a long chain can be measured.
    GameModeRuntime unlimited(GameModeBuilder(MakePointmatch()).ScoreLimit(std::nullopt).Build());
    unlimited.ObserveRoster(view);
    for (const auto award : awards) {
        ++view.tick;
        unlimited.OnKill(Kill(view), view);
        unlimited.EndTick(view);
        total += award;
        EXPECT_EQ(unlimited.GetState().players.at(0).points, total);
    }
    view.tick += 180;
    auto fact = Kill(view);
    fact.killer_flag = true;
    unlimited.OnKill(fact, view);
    unlimited.EndTick(view);
    EXPECT_EQ(unlimited.GetState().players.at(0).points, total + 2);
}

TEST(GameModesTest, RuntimeInstancesNeverShareChainState)
{
    const auto definition = MakePointmatch();
    GameModeRuntime first(definition);
    GameModeRuntime second(definition);
    auto const view = Duel();
    first.ObserveRoster(view);
    second.ObserveRoster(view);
    for (int i = 0; i < 3; ++i) {
        first.OnKill(Kill(view), view);
        first.EndTick(view);
    }
    second.OnKill(Kill(view), view);
    second.EndTick(view);
    EXPECT_EQ(first.GetState().players.at(0).points, 4);
    EXPECT_EQ(second.GetState().players.at(0).points, 1);
}

TEST(GameModesTest, RamboUsesProjectileWeaponAndFloorsPenalty)
{
    GameModeRuntime mode(MakeRambomatch());
    auto view = Duel();
    mode.ObserveRoster(view);
    mode.OnKill(Kill(view), view);
    mode.EndTick(view);
    EXPECT_EQ(mode.GetState().players.at(0).points, 0);
    auto bow = Kill(view);
    bow.weapon = WeaponType::Bow;
    mode.OnKill(bow, view);
    mode.EndTick(view);
    EXPECT_EQ(mode.GetState().players.at(0).points, 1);
    view.players.at(1).bow = true;
    mode.OnKill(Kill(view), view);
    mode.OnKill(Kill(view), view);
    mode.EndTick(view);
    EXPECT_EQ(mode.GetState().players.at(0).points, 0);
}

TEST(GameModesTest, InfiltrationCapturePenaltyClampsResultingTotal)
{
    GameModeRuntime mode(MakeInfiltration());
    auto view = Duel(true);
    mode.ObserveRoster(view);
    mode.OnCapture({ .player = 0, .objective = 1 }, view);
    mode.EndTick(view);
    EXPECT_EQ(mode.GetState().teams.at(1), 30);
    for (std::size_t i = 2; i < 16; ++i) {
        view.players.at(i) = { .active = true, .alive = true, .team = TeamType::Alpha };
    }
    mode.OnCapture({ .player = 0, .objective = 1 }, view);
    mode.EndTick(view);
    EXPECT_EQ(mode.GetState().teams.at(1), 0);
    EXPECT_EQ(mode.GetState().players.at(0).captures, 2);
}

TEST(GameModesTest, InfiltrationDefenseRequiresHomeAndBothTeams)
{
    GameModeRuntime mode(MakeInfiltration());
    auto view = Duel(true);
    view.tick = 300;
    view.objectives.push_back({ .definition_index = 1, .home = true });
    mode.ObserveRoster(view);
    mode.EndTick(view);
    EXPECT_EQ(mode.GetState().teams.at(2), 1);
    view.tick = 600;
    view.objectives.at(0).home = false;
    mode.EndTick(view);
    EXPECT_EQ(mode.GetState().teams.at(2), 1);
}

TEST(GameModesTest, HoldTheFlagDoesNotRestartClockOnCarrierChange)
{
    GameModeRuntime mode(MakeHoldTheFlag());
    auto view = Duel(true);
    view.tick = 299;
    view.objectives.push_back({ .definition_index = 0, .carrier = 0 });
    mode.ObserveRoster(view);
    mode.EndTick(view);
    EXPECT_EQ(mode.GetState().teams.at(1), 0);
    view.tick = 300;
    view.objectives.at(0).carrier = 1;
    mode.EndTick(view);
    EXPECT_EQ(mode.GetState().teams.at(2), 1);
}

TEST(GameModesTest, SurvivalCaptureSuppressesEliminationAwardAndRestartsAcrossWrap)
{
    GameModeRuntime mode(WithSurvival(MakeCaptureTheFlag()));
    auto view = Duel(true);
    view.tick = std::numeric_limits<std::uint32_t>::max() - 10;
    mode.ObserveRoster(view);
    mode.OnCapture({ .player = 0, .objective = 1 }, view);
    view.players.at(1).alive = false;
    mode.EndTick(view);
    EXPECT_EQ(mode.GetState().teams.at(1), 1);
    EXPECT_EQ(mode.GetState().phase, MatchPhase::RoundEnding);
    EXPECT_FALSE(mode.RestartDue(view.tick + 299));
    EXPECT_TRUE(mode.RestartDue(view.tick + 300));
    mode.RestartRound(view);
    EXPECT_EQ(mode.GetState().round_number, 2U);
}

TEST(GameModesTest, SurvivalDoesNotAllowLateJoinToReviveEliminatedTeam)
{
    GameModeRuntime mode(WithSurvival(MakeTeammatch()));
    auto view = Duel(true);
    mode.ObserveRoster(view);
    view.players.at(
      2) = { .active = true, .alive = true, .team = TeamType::Bravo, .generation = 1 };
    mode.ObserveRoster(view);
    EXPECT_FALSE(mode.IsRoundParticipant(2));
    view.players.at(1).alive = false;
    mode.EndTick(view);
    EXPECT_EQ(mode.GetState().phase, MatchPhase::RoundEnding);
}

TEST(GameModesTest, MatchLimitCommitsFullTickAndAllowsTiedWinners)
{
    GameModeRuntime mode(GameModeBuilder(MakeDeathmatch()).ScoreLimit(1).Build());
    auto const view = Duel();
    mode.ObserveRoster(view);
    mode.OnKill(Kill(view), view);
    auto reverse = Kill(view);
    reverse.victim = 0;
    reverse.killer = 1;
    mode.OnKill(reverse, view);
    mode.EndTick(view);
    EXPECT_EQ(mode.GetState().phase, MatchPhase::Finished);
    EXPECT_TRUE(mode.GetState().winners.at(0));
    EXPECT_TRUE(mode.GetState().winners.at(1));
    mode.OnKill(Kill(view), view);
    mode.EndTick(view);
    EXPECT_EQ(mode.GetState().players.at(0).points, 1);
}

TEST(GameModesTest, SlotGenerationPreventsOldProjectileCredit)
{
    GameModeRuntime mode(MakeDeathmatch());
    auto view = Duel();
    mode.ObserveRoster(view);
    const auto old = Kill(view);
    view.players.at(0).generation = 2;
    mode.ObserveRoster(view);
    mode.OnKill(old, view);
    mode.EndTick(view);
    EXPECT_EQ(mode.GetState().players.at(0).points, 0);
}

namespace
{
class CustomRule final : public IGameRule
{
public:
    explicit CustomRule(int award = 7)
        : award_(award)
    {
    }
    void OnTick(const ModeWorldView& /*view*/, std::vector<ModeAction>& actions) override
    {
        actions.push_back({ .kind = ModeActionKind::PlayerPoints, .target = 0, .amount = award_ });
    }

private:
    int award_;
};
} // namespace
TEST(GameModesTest, CustomRulePlugsInWithoutEditingRuntime)
{
    const auto definition =
      GameModeBuilder("custom")
        .AddRule({ .id = "custom_points", .create = [] { return std::make_unique<CustomRule>(); } })
        .Build();
    GameModeRuntime mode(definition);
    auto const view = Duel();
    mode.ObserveRoster(view);
    mode.EndTick(view);
    EXPECT_EQ(mode.GetState().players.at(0).points, 7);
}

TEST(GameModesTest, FingerprintIncludesResolvedRuleAndRespawnSettings)
{
    const auto base = MakeDeathmatch();
    const auto changed = GameModeBuilder(base).Respawn({ .delay = 240 }).Build();
    EXPECT_EQ(base.GetFingerprint(), MakeDeathmatch().GetFingerprint());
    EXPECT_NE(base.GetFingerprint(), changed.GetFingerprint());
    const auto changed_rule = GameModeBuilder("deathmatch")
                                .AddRule(KillScoring({ .player_points = 2 }))
                                .ScoreLimit(30)
                                .Build();
    EXPECT_NE(base.GetFingerprint(), changed_rule.GetFingerprint());
}

TEST(GameModesTest, MultikillZeroTimerDoesNotMultiplyButRetainsChainUntilExpiration)
{
    GameModeRuntime mode(GameModeBuilder(MakePointmatch()).ScoreLimit(std::nullopt).Build());
    auto view = Duel();
    mode.ObserveRoster(view);
    mode.OnKill(Kill(view), view);
    mode.EndTick(view);
    view.tick = 1;
    mode.OnKill(Kill(view), view);
    mode.EndTick(view);
    view.tick = 181;
    mode.OnKill(Kill(view), view);
    mode.EndTick(view);
    EXPECT_EQ(mode.GetState().players.at(0).points, 3);
    ++view.tick;
    mode.OnKill(Kill(view), view);
    mode.EndTick(view);
    EXPECT_EQ(mode.GetState().players.at(0).points, 7);
}

TEST(GameModesTest, MatchClockIncludesSurvivalRoundEndingDelay)
{
    GameModeRuntime mode(GameModeBuilder(WithSurvival(MakeDeathmatch())).TimeLimit(2).Build());
    auto view = Duel();
    mode.ObserveRoster(view);
    view.players.at(1).alive = false;
    mode.EndTick(view);
    EXPECT_EQ(mode.GetState().phase, MatchPhase::RoundEnding);
    ++view.tick;
    mode.EndTick(view);
    EXPECT_EQ(mode.GetState().phase, MatchPhase::Finished);
}

TEST(GameModesTest, TypedRuleFactoryOwnsItsConfiguration)
{
    const auto definition =
      GameModeBuilder("typed_custom").AddRule<CustomRule>("custom_points", 11).Build();
    GameModeRuntime mode(definition);
    const auto view = Duel();
    mode.ObserveRoster(view);
    mode.EndTick(view);
    EXPECT_EQ(mode.GetState().players.at(0).points, 11);
}
