#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <stdexcept>
import MovementScenarios.Catalog;
import MovementScenarios.Runner;
import MovementScenarios.Export;
import Shared.Core.Animations;
using namespace Soldank;
using namespace Soldank::MovementScenarios;

TEST(MovementScenario, PolygonValidationAndConflictingInputs)
{
    ScenarioBuilder builder(ScenarioId::IdleLeft);
    builder.AddPolygon({ 0, 0 }, { 1, 1 }, { 2, 2 });
    EXPECT_THROW(builder.Build(), std::invalid_argument);
    auto definition = GetScenario(ScenarioId::IdleLeft);
    definition.changes = { { 0, ControlActionType::Jump, true },
                           { 0, ControlActionType::Jump, false } };
    EXPECT_THROW(Validate(definition), std::invalid_argument);
}
TEST(MovementScenario, SimultaneousChangesPersistAndDurationIsExclusive)
{
    auto definition = GetScenario(ScenarioId::IdleLeft);
    definition.duration_ticks = 4;
    definition.changes = { { 0, ControlActionType::Jump, true },
                           { 1, ControlActionType::Jump, false },
                           { 1, ControlActionType::UseJets, true } };
    ScenarioRunner runner(definition);
    runner.RunUntil(1);
    EXPECT_TRUE(runner.GetTrace()[0].input.jump);
    EXPECT_FALSE(runner.GetTrace()[1].input.jump);
    EXPECT_TRUE(runner.GetTrace()[1].input.jets);
    runner.RunToCompletion();
    EXPECT_EQ(runner.GetTrace().size(), 4);
    EXPECT_TRUE(runner.GetTrace()[3].input.jets);
    EXPECT_FALSE(runner.Step());
    EXPECT_THROW(runner.RunUntil(0), std::invalid_argument);
}
TEST(MovementScenario, SetupTimeoutIsAnError)
{
    auto definition = GetScenario(ScenarioId::IdleLeft);
    definition.spawn = { 0, -2000 };
    definition.settle_limit = 5;
    EXPECT_THROW(ScenarioRunner runner(definition), std::runtime_error);
}
TEST(MovementScenario, CoordinatesAreTranslatedTogether)
{
    auto definition = GetScenario(ScenarioId::IdleLeft);
    definition.settle_ticks = 0;
    definition.relative_aim = false;
    definition.aim = { 50, -20 };
    definition.duration_ticks = 1;
    ScenarioRunner runner(definition);
    const auto offset = runner.GetOffset();
    EXPECT_FLOAT_EQ(runner.GetOrigin().x, definition.spawn.x + offset.x);
    EXPECT_FLOAT_EQ(runner.GetOrigin().y, definition.spawn.y + offset.y);
    runner.Step();
    EXPECT_FLOAT_EQ(runner.GetTrace()[0].aim.x, definition.aim.x + offset.x);
    EXPECT_FLOAT_EQ(runner.GetTrace()[0].aim.y, definition.aim.y + offset.y);
}
TEST(MovementScenario, FreshResetsHaveIdenticalTraces)
{
    auto definition = GetScenario(ScenarioId::LateBackflipLeft);
    ScenarioRunner first(definition);
    first.RunToCompletion();
    ScenarioRunner second(definition);
    for (int i = 0; i < 10; ++i) {
        second.Reset(definition);
        second.RunToCompletion();
        EXPECT_TRUE(SameTrace(first, second)) << "Repetition " << i;
    }
}
TEST(MovementScenario, ExportReplayHasIdenticalTrace)
{
    ScenarioRunner first(GetScenario(ScenarioId::LateBackflipLeft));
    first.RunToCompletion();
    const auto path = std::filesystem::temp_directory_path() / "soldank-movement-roundtrip.json";
    ExportScenario(first, path.string());
    ScenarioRunner replay(LoadScenario(path.string()));
    replay.RunToCompletion();
    EXPECT_TRUE(SameTrace(first, replay));
    std::filesystem::remove(path);
}
TEST(MovementScenario, InitialIdleCheckpoint)
{
    // A deliberately small independently specified checkpoint: neutral input
    // on the flat floor remains grounded, without horizontal velocity or travel.
    ScenarioRunner runner(GetScenario(ScenarioId::IdleLeft));
    runner.RunUntil(0);
    const auto& sample = runner.GetTrace().front();
    EXPECT_TRUE(sample.grounded) << "idle_left tick 0 grounded";
    EXPECT_NEAR(sample.velocity.x, 0.0F, 0.001F) << "idle_left tick 0 velocity.x";
    EXPECT_NEAR(sample.displacement.x, 0.0F, 0.01F) << "idle_left tick 0 displacement.x";
}

TEST(MovementScenario, IdleScenariosFaceTheirNamedDirection)
{
    for (const auto id : { ScenarioId::IdleLeft, ScenarioId::IdleRight }) {
        SCOPED_TRACE(ScenarioIdToString(id));
        ScenarioRunner runner(GetScenario(id));
        runner.RunToCompletion();
        const int expected_direction = id == ScenarioId::IdleLeft ? -1 : 1;
        for (const auto& sample : runner.GetTrace()) {
            EXPECT_EQ(sample.direction, expected_direction) << "tick " << sample.tick;
        }
    }
}

TEST(MovementScenario, PreparationMomentumAndAimChangesUseNormalPhysics)
{
    auto definition = GetScenario(ScenarioId::IdleLeft);
    definition.aim = { 6400, 0 };
    definition.preparation_ticks = 20;
    definition.preparation_changes = { { 0, ControlActionType::MoveRight, true } };
    definition.duration_ticks = 3;
    definition.aim_changes = { { 1, { -6400, 0 }, true } };
    ScenarioRunner runner(definition);
    runner.RunToCompletion();
    EXPECT_GT(runner.GetTrace()[0].velocity.x, 0);
    EXPECT_TRUE(runner.GetTrace()[0].input.right);
    EXPECT_EQ(runner.GetTrace()[0].direction, 1);
    EXPECT_EQ(runner.GetTrace()[1].direction, -1);
    EXPECT_GT(runner.GetTrace()[0].displacement.x, 0);
    const auto path = std::filesystem::temp_directory_path() / "soldank-movement-preparation.json";
    ExportScenario(runner, path.string());
    ScenarioRunner replay(LoadScenario(path.string()));
    replay.RunToCompletion();
    EXPECT_TRUE(SameTrace(runner, replay));
    std::filesystem::remove(path);
}

TEST(MovementScenario, HoldingJumpPreservesEngineInputHistory)
{
    auto definition = GetScenario(ScenarioId::IdleLeft);
    definition.duration_ticks = 3;
    definition.changes = { { 0, ControlActionType::Jump, true } };
    ScenarioRunner runner(definition);
    runner.GetStateManager()->TransformSoldier(
      definition.soldier_id, [](auto& soldier) { soldier.control.was_jumping = true; });
    runner.RunUntil(1);
    EXPECT_TRUE(runner.GetStateManager()->GetSoldier(definition.soldier_id).control.was_jumping);
    runner.Step();
    EXPECT_TRUE(runner.GetTrace().back().input.jump);
}

TEST(MovementScenario, EveryCatalogScenarioCompletes)
{
    for (const auto& definition : MakeScenarios()) {
        SCOPED_TRACE(ScenarioIdToString(definition.id));
        ScenarioRunner runner(definition);
        runner.RunToCompletion();
        EXPECT_EQ(runner.GetTrace().size(), definition.duration_ticks);
    }
}

TEST(MovementScenario, RejectsUnsupportedRecordingSchema)
{
    const auto path =
      std::filesystem::temp_directory_path() / "soldank-movement-invalid-schema.json";
    ScenarioRunner runner(GetScenario(ScenarioId::IdleLeft));
    runner.RunUntil(0);
    ExportScenario(runner, path.string());
    std::ifstream input(path);

    std::string recording((std::istreambuf_iterator<char>(input)), {});
    const auto version = recording.find("\"schema_version\": 1");
    ASSERT_NE(version, std::string::npos);
    recording.replace(
      version, std::string("\"schema_version\": 1").size(), "\"schema_version\": 2");
    input.close();
    {
        std::ofstream output(path);
        output << recording;
    }
    EXPECT_THROW(LoadScenario(path.string()), std::invalid_argument);
    std::filesystem::remove(path);
}

TEST(MovementScenario, SharedBackflipScenariosEnterRollBack)
{
    for (const auto id : { ScenarioId::LateBackflipLeft, ScenarioId::LateBackflipRight }) {
        SCOPED_TRACE(ScenarioIdToString(id));
        ScenarioRunner runner(GetScenario(id));
        runner.RunUntil(32);
        const auto& flip = runner.GetTrace().at(32);
        EXPECT_EQ(flip.body_type, static_cast<int>(AnimationType::RollBack)) << "tick 32 body";
        EXPECT_EQ(flip.legs_type, static_cast<int>(AnimationType::RollBack)) << "tick 32 legs";
        runner.RunUntil(35);
        const auto& released = runner.GetTrace().at(35).input;
        EXPECT_FALSE(released.left);
        EXPECT_FALSE(released.right);
        EXPECT_FALSE(released.jump);
        EXPECT_FALSE(released.jets);
    }
}

TEST(MovementScenario, InvalidScenarioIdIsRejected)
{
    EXPECT_THROW(GetScenario(ScenarioId::Count), std::invalid_argument);
}

TEST(MovementScenario, ScenarioIdsRoundTripAndCoverTheCatalog)
{
    const auto ids = GetScenarioIds();
    const auto scenarios = MakeScenarios();
    ASSERT_EQ(ids.size(), scenarios.size());
    for (const auto id : ids) {
        const auto name = ScenarioIdToString(id);
        EXPECT_EQ(ScenarioIdFromString(name), id);
        EXPECT_EQ(GetScenario(id).id, id);
    }
    EXPECT_EQ(ScenarioIdToString(ScenarioId::LateBackflipLeft), "late_backflip_left");
    EXPECT_THROW(ScenarioIdFromString("missing_scenario"), std::invalid_argument);
    EXPECT_THROW(ScenarioIdToString(ScenarioId::Count), std::invalid_argument);
}
