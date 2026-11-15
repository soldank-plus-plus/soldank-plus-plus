#include <cstdint>
#include "core/math/Glm.hpp"
#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

import Shared.Core.World;
import Shared.Core.State.StateManager;
import Shared.Core.GameModes.BuiltinGameModes;
import Shared.Core.GameModes.WorldGameModeAdapter;
import Shared.Core.GameModes.GameModeRuntime;
import Shared.Core.Physics.Polygons.FlagCarry;
import Shared.Core.Map.PMSEnums;
import Shared.Core.Map.PMSStructs;
import Shared.Core.Simulation.WorldTick;
import Shared.Core.Simulation.PlayerInputApplication;
import Shared.Core.Simulation.SimulationCommands;
import Shared.Core.Entities.Soldier;
import Shared.Core.Types.ItemType;
import Shared.Core.Types.TeamType;
import Shared.Core.Types.WeaponType;
import Shared.Networking.NetworkPackets;
import Shared.Networking.GameModeProtocol;
import Shared.Networking.ClientHello;
import Testing.Framework.Shared.MapBuilder;

using namespace Soldank;

namespace
{
std::unique_ptr<World> MakeWorld(const GameModeDefinition& definition)
{
    auto world = std::make_unique<World>();
    const auto map = SoldankTesting::MapBuilder::Empty()->Build();
    world->GetStateManager()->OverrideMap(*map);
    world->GetStateManager()->ConfigureGameMode(definition);
    for (std::uint8_t id = 0; id < 2; ++id) {
        world->CreateSoldier(id);
        world->SpawnSoldier(id, glm::vec2{ static_cast<float>(id) * 200.0F, 0.0F });
    }
    return world;
}
std::optional<std::uint8_t> FindObjective(const StateManager& state, ItemType type)
{
    std::optional<std::uint8_t> id;
    state.ForEachItem([&](const auto& item) {
        if (item.style == type) {
            id = item.id;
        }
    });
    return id;
}

} // namespace

TEST(GameModeWorldTest, WorldCommitsAttributedDeathOnceAndUsesConfiguredRespawn)
{
    auto world = MakeWorld(GameModeBuilder(MakeDeathmatch()).Respawn({ .delay = 4 }).Build());
    const auto state = world->GetStateManager();
    world->Tick({ .tick = 1 });
    state->RecordLethalHit(
      1, { .victim = 1, .killer = 0, .killer_generation = state->GetParticipantGeneration(0) });
    world->KillSoldier(1);
    world->KillSoldier(1);
    world->Tick({ .tick = 2 });
    EXPECT_EQ(state->GetGameMode()->GetState().players.at(0).points, 1);
    EXPECT_EQ(state->GetGameMode()->GetState().players.at(1).deaths, 1);
    EXPECT_TRUE(state->GetSoldier(1).dead_meat);
    for (unsigned int tick = 3; tick <= 5; ++tick) {
        world->Tick({ .tick = tick });
    }
    EXPECT_FALSE(state->GetSoldier(1).dead_meat);
}

TEST(GameModeWorldTest, CompetitiveModeRequiresItsObjectiveSpawns)
{
    auto world = MakeWorld(MakeCaptureTheFlag());
    EXPECT_THROW(world->Tick({ .tick = 1 }), std::invalid_argument);
}

TEST(GameModeWorldTest, TeamShirtsMatchOpenSoldatAndSurviveAppearanceUpdates)
{
    auto world = MakeWorld(MakeTeammatch());
    const auto state = world->GetStateManager();
    EXPECT_EQ(state->GetSoldier(0).appearance.shirt,
              (glm::vec4{ 210.0F / 255.0F, 15.0F / 255.0F, 5.0F / 255.0F, 1.0F }));
    EXPECT_EQ(state->GetSoldier(1).appearance.shirt,
              (glm::vec4{ 21.0F / 255.0F, 31.0F / 255.0F, 217.0F / 255.0F, 1.0F }));
    const std::vector<std::pair<TeamType, glm::vec4>> colors{
        { TeamType::Alpha, { 210.0F / 255.0F, 15.0F / 255.0F, 5.0F / 255.0F, 1.0F } },
        { TeamType::Bravo, { 21.0F / 255.0F, 31.0F / 255.0F, 217.0F / 255.0F, 1.0F } },
        { TeamType::Charlie, { 210.0F / 255.0F, 210.0F / 255.0F, 5.0F / 255.0F, 1.0F } },
        { TeamType::Delta, { 5.0F / 255.0F, 210.0F / 255.0F, 5.0F / 255.0F, 1.0F } },
    };
    SoldierAppearance custom;
    custom.shirt = { 1, 1, 1, 1 };
    custom.pants = { 0, 0, 0, 1 };
    for (const auto& [team, color] : colors) {
        state->TransformSoldier(0, [&](auto& soldier) { soldier.SetTeam(team); });
        EXPECT_EQ(state->GetSoldier(0).appearance.shirt, color);
        PlayerInputCommand command{};
        command.soldier_id = 0;
        command.appearance = custom;
        ApplyPlayerInputCommand(*state, command);
        EXPECT_EQ(state->GetSoldier(0).appearance.shirt, color);
        EXPECT_EQ(state->GetSoldier(0).appearance.pants, custom.pants);
    }
    for (const auto team : { TeamType::None, TeamType::Spectator }) {
        state->TransformSoldier(0, [&](auto& soldier) { soldier.SetTeam(team); });
        PlayerInputCommand command{};
        command.soldier_id = 0;
        command.appearance = custom;
        ApplyPlayerInputCommand(*state, command);
        EXPECT_EQ(state->GetSoldier(0).appearance.shirt, custom.shirt);
    }
}

TEST(GameModeWorldTest, FriendlyFlagReturnWorksWhileCarryingEnemyFlag)
{
    auto world = MakeWorld(MakeCaptureTheFlag());
    const auto state = world->GetStateManager();
    // Exercise the interaction seam independently of physical map contacts.
    ModeWorldView view;
    view.players.at(0) = { .active = true, .alive = true, .team = TeamType::Alpha };
    view.players.at(1) = { .active = true, .alive = true, .team = TeamType::Bravo };
    state->GetGameMode()->ObserveRoster(view);
    auto& soldier = state->GetSoldier(0);
    soldier.SetCeaseFireCounter(-1);
    soldier.is_holding_flags = true;
    auto& flag = state->CreateItem({ 100, 0 }, 255, ItemType::AlphaFlag);
    flag.in_base = false;
    EXPECT_FALSE(FlagCarry::TryAcquire(*state, soldier, flag));
    EXPECT_EQ(flag.time_out, 0);
    EXPECT_EQ(flag.holding_soldier_id, -1);
    flag.in_base = true;
    EXPECT_EQ(state->DecideFlagInteraction(soldier, flag), ObjectiveInteraction::Ignore);
}

TEST(GameModeWorldTest, InfiltrationDestinationCannotBeCarriedByEitherTeam)
{
    auto world = MakeWorld(MakeInfiltration());
    const auto state = world->GetStateManager();
    ModeWorldView view;
    view.players.at(0) = { .active = true, .alive = true, .team = TeamType::Alpha };
    view.players.at(1) = { .active = true, .alive = true, .team = TeamType::Bravo };
    state->GetGameMode()->ObserveRoster(view);
    const auto& marker = state->CreateItem({}, 255, ItemType::AlphaFlag);
    EXPECT_EQ(state->DecideFlagInteraction(state->GetSoldier(0), marker),
              ObjectiveInteraction::Ignore);
    EXPECT_EQ(state->DecideFlagInteraction(state->GetSoldier(1), marker),
              ObjectiveInteraction::Ignore);
}

TEST(GameModeWorldTest, SurvivalRestartsWithoutManufacturingDeathStatistics)
{
    auto world = MakeWorld(WithSurvival(MakeDeathmatch()));
    const auto state = world->GetStateManager();
    world->Tick({ .tick = 1 });
    world->KillSoldier(1);
    world->Tick({ .tick = 2 });
    EXPECT_EQ(state->GetGameMode()->GetState().phase, MatchPhase::RoundEnding);
    world->Tick({ .tick = 301 });
    EXPECT_TRUE(state->GetSoldier(1).dead_meat);
    world->Tick({ .tick = 302 });
    EXPECT_FALSE(state->GetSoldier(1).dead_meat);
    EXPECT_EQ(state->GetGameMode()->GetState().players.at(0).deaths, 0);
    EXPECT_EQ(state->GetGameMode()->GetState().players.at(1).deaths, 1);
}

TEST(GameModeWorldTest, OnlinePredictionCannotAdvanceMatchState)
{
    auto world = MakeWorld(MakeDeathmatch());
    const auto state = world->GetStateManager();
    state->SetAuthoritativeProjectiles(false);
    world->Tick({ .tick = 1 });
    EXPECT_EQ(state->GetGameMode()->GetState().elapsed_ticks, 0U);
    EXPECT_EQ(state->GetGameMode()->GetState().phase, MatchPhase::Waiting);
}

TEST(GameModeWorldTest, PauseFreezesMatchClock)
{
    auto world = MakeWorld(MakeDeathmatch());
    const auto state = world->GetStateManager();
    world->Tick({ .tick = 1 });
    const auto elapsed = state->GetGameMode()->GetState().elapsed_ticks;
    state->PauseGame();
    world->Tick({ .tick = 2 });
    EXPECT_EQ(state->GetGameMode()->GetState().elapsed_ticks, elapsed);
}

TEST(GameModeWorldTest, ProtocolValidatesFullSnapshotsAndHandshakeVersion)
{
    auto world = MakeWorld(MakeDeathmatch());
    world->Tick({ .tick = 1 });
    auto packet = MakeMatchStatePacket(*world->GetStateManager());
    EXPECT_TRUE(IsValidMatchStatePacket(packet));
    ++packet.version;
    EXPECT_FALSE(IsValidMatchStatePacket(packet));
    packet.version = GAME_MODE_PROTOCOL_VERSION;
    packet.objective_count = 255;
    EXPECT_FALSE(IsValidMatchStatePacket(packet));
    EXPECT_EQ(ParseClientHello(MakeClientHello("Player")), "Player");
    EXPECT_FALSE(ParseClientHello("old client"));
    EXPECT_FALSE(ParseClientHello("soldank-mode/1:Player"));
}

TEST(GameModeWorldTest, CaptureRequiresOwnFlagHomeAndScoresOnce)
{
    auto world = MakeWorld(MakeCaptureTheFlag());
    const auto state = world->GetStateManager();
    state->GetMap().ReplaceSpawnPoints({
      { .active = 1, .x = 0, .y = 0, .type = PMSSpawnPointType::AlphaFlag },
      { .active = 1, .x = 500, .y = 0, .type = PMSSpawnPointType::BravoFlag },
    });
    WorldGameModeAdapter adapter;
    adapter.BeginTick(*state);
    const auto alpha = FindObjective(*state, ItemType::AlphaFlag);
    const auto bravo = FindObjective(*state, ItemType::BravoFlag);
    ASSERT_TRUE(alpha);
    ASSERT_TRUE(bravo);
    state->TransformItems([&](auto& item) {
        if (item.id == *bravo) {
            item.holding_soldier_id = 0;
            item.skeleton->SetPos(1, {});
        } else if (item.id == *alpha) {
            item.skeleton->SetPos(1, { 100, 0 });
        }
    });
    adapter.EndTick(*state);
    EXPECT_EQ(state->GetGameMode()->GetState().teams.at(1), 0);
    state->TransformItems([&](auto& item) {
        if (item.id == *alpha) {
            item.skeleton->SetPos(1, {});
        }
    });
    adapter.EndTick(*state);
    EXPECT_EQ(state->GetGameMode()->GetState().teams.at(1), 1);
    adapter.EndTick(*state);
    EXPECT_EQ(state->GetGameMode()->GetState().teams.at(1), 1);
}

TEST(GameModeWorldTest, ObjectiveReturnDurationComesFromDefinition)
{
    const auto definition = GameModeBuilder("fast_return")
                              .Teams(2)
                              .Objective({
                                .id = "alpha",
                                .item_type = ItemType::AlphaFlag,
                                .team = TeamType::Alpha,
                                .return_ticks = 2,
                              })
                              .Build();
    const auto world = MakeWorld(definition);
    const auto state = world->GetStateManager();
    state->GetMap().ReplaceSpawnPoints(
      { { .active = 1, .x = 0, .y = 0, .type = PMSSpawnPointType::AlphaFlag } });
    WorldGameModeAdapter adapter;
    adapter.BeginTick(*state);
    const auto alpha = FindObjective(*state, ItemType::AlphaFlag);
    ASSERT_TRUE(alpha);
    state->TransformItems([&](auto& item) {
        if (item.id == *alpha) {
            item.skeleton->SetPos(1, { 100, 0 });
        }
    });
    state->SetGameTick(1);
    adapter.BeginTick(*state);
    state->SetGameTick(2);
    adapter.BeginTick(*state);
    EXPECT_EQ(state->GetItem(*alpha).skeleton->GetPos(1).x, 100.0F);
    state->SetGameTick(3);
    adapter.BeginTick(*state);
    EXPECT_EQ(state->GetItem(*alpha).skeleton->GetPos(1).x, 0.0F);
}
