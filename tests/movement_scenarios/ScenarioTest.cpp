#include <gtest/gtest.h>
#include <array>
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
        runner.RunUntil(33);
        const auto& after_jet_pulse = runner.GetTrace().at(33).input;
        EXPECT_FALSE(after_jet_pulse.jets);
        EXPECT_TRUE(after_jet_pulse.jump);
        EXPECT_EQ(after_jet_pulse.left, id == ScenarioId::LateBackflipLeft);
        EXPECT_EQ(after_jet_pulse.right, id == ScenarioId::LateBackflipRight);
        runner.RunUntil(40);
        const auto& after_jump = runner.GetTrace().at(40).input;
        EXPECT_FALSE(after_jump.jump);
        EXPECT_EQ(after_jump.left, id == ScenarioId::LateBackflipLeft);
        EXPECT_EQ(after_jump.right, id == ScenarioId::LateBackflipRight);
        runner.RunUntil(120);
        const auto& released = runner.GetTrace().at(120).input;
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

namespace
{
struct CannonballCheckpoint
{
    unsigned int tick;
    glm::vec2 position;
    glm::vec2 velocity;
    AnimationType body_type;
    AnimationType legs_type;
    unsigned int body_frame;
    unsigned int legs_frame;
    int body_speed;
    int legs_speed;
    bool grounded;
};

template<std::size_t SIZE>
void CheckCannonball(ScenarioId id, const std::array<CannonballCheckpoint, SIZE>& checkpoints)
{
    // Fixed baselines captured from the visually approved scenarios. Do not regenerate
    // these from the simulation under test when physics or animation behavior changes.
    constexpr float POSITION_TOLERANCE = 0.001F;
    constexpr float VELOCITY_TOLERANCE = 0.00001F;
    SCOPED_TRACE(ScenarioIdToString(id));
    ScenarioRunner runner(GetScenario(id));
    runner.RunToCompletion();
    ASSERT_EQ(runner.GetTrace().size(), 180U);
    for (const auto& expected : checkpoints) {
        SCOPED_TRACE(::testing::Message() << "tick " << expected.tick);
        const auto& actual = runner.GetTrace().at(expected.tick);
        EXPECT_NEAR(actual.position.x, expected.position.x, POSITION_TOLERANCE);
        EXPECT_NEAR(actual.position.y, expected.position.y, POSITION_TOLERANCE);
        EXPECT_NEAR(actual.velocity.x, expected.velocity.x, VELOCITY_TOLERANCE);
        EXPECT_NEAR(actual.velocity.y, expected.velocity.y, VELOCITY_TOLERANCE);
        EXPECT_EQ(actual.body_type, static_cast<int>(expected.body_type));
        EXPECT_EQ(actual.legs_type, static_cast<int>(expected.legs_type));
        EXPECT_EQ(actual.body_frame, expected.body_frame);
        EXPECT_EQ(actual.legs_frame, expected.legs_frame);
        EXPECT_EQ(actual.body_speed, expected.body_speed);
        EXPECT_EQ(actual.legs_speed, expected.legs_speed);
        EXPECT_EQ(actual.grounded, expected.grounded);
    }
}
} // namespace

TEST(MovementScenario, CannonballLeftCheckpoints)
{
    const std::array checkpoints{
        CannonballCheckpoint{ 0,
                              { -400.000000000F, -182.071273804F },
                              { 0.000000000F, 0.000000000F },
                              AnimationType::Stand,
                              AnimationType::RunBack,
                              17,
                              2,
                              3,
                              1,
                              false },
        CannonballCheckpoint{ 14,
                              { -408.936157227F, -181.466003418F },
                              { -1.151093483F, 0.054058108F },
                              AnimationType::Stand,
                              AnimationType::RunBack,
                              5,
                              16,
                              3,
                              1,
                              true },
        CannonballCheckpoint{ 15,
                              { -410.207702637F, -181.420242310F },
                              { -1.221068978F, 0.043475695F },
                              AnimationType::Stand,
                              AnimationType::JumpSide,
                              5,
                              2,
                              3,
                              1,
                              true },
        CannonballCheckpoint{ 18,
                              { -413.742797852F, -181.252365112F },
                              { -1.122194171F, 0.062634438F },
                              AnimationType::Stand,
                              AnimationType::JumpSide,
                              6,
                              5,
                              3,
                              1,
                              false },
        CannonballCheckpoint{ 24,
                              { -426.505920410F, -184.809967041F },
                              { -2.794563293F, -1.041789412F },
                              AnimationType::Stand,
                              AnimationType::JumpSide,
                              8,
                              11,
                              3,
                              1,
                              false },
        CannonballCheckpoint{ 25,
                              { -429.600494385F, -186.041763306F },
                              { -3.063617706F, -1.219471455F },
                              AnimationType::RollBack,
                              AnimationType::RollBack,
                              2,
                              2,
                              1,
                              1,
                              false },
        CannonballCheckpoint{ 26,
                              { -432.724121094F, -187.301239014F },
                              { -3.092381477F, -1.246876717F },
                              AnimationType::RollBack,
                              AnimationType::RollBack,
                              3,
                              3,
                              1,
                              1,
                              false },
        CannonballCheckpoint{ 30,
                              { -445.503356934F, -191.620391846F },
                              { -3.204588890F, -0.963685274F },
                              AnimationType::RollBack,
                              AnimationType::RollBack,
                              7,
                              7,
                              1,
                              1,
                              false },
        CannonballCheckpoint{ 40,
                              { -479.347900391F, -197.632019043F },
                              { -3.466142654F, -0.303569168F },
                              AnimationType::RollBack,
                              AnimationType::RollBack,
                              17,
                              17,
                              1,
                              1,
                              false },
        CannonballCheckpoint{ 55,
                              { -534.728332520F, -195.005569458F },
                              { -3.812337637F, 0.570166171F },
                              AnimationType::RollBack,
                              AnimationType::RollBack,
                              32,
                              32,
                              1,
                              1,
                              false },
        CannonballCheckpoint{ 56,
                              { -538.600646973F, -194.375396729F },
                              { -3.833614349F, 0.623864532F },
                              AnimationType::Stand,
                              AnimationType::CrouchRunBack,
                              1,
                              1,
                              3,
                              2,
                              false },
        CannonballCheckpoint{ 57,
                              { -542.434265137F, -193.691528320F },
                              { -3.795278311F, 0.677025914F },
                              AnimationType::Aim,
                              AnimationType::RunBack,
                              1,
                              2,
                              2,
                              1,
                              false },
        CannonballCheckpoint{ 58,
                              { -546.259521484F, -192.954498291F },
                              { -3.787025452F, 0.729655683F },
                              AnimationType::Stand,
                              AnimationType::RunBack,
                              1,
                              3,
                              3,
                              1,
                              false },
        CannonballCheckpoint{ 60,
                              { -553.885375977F, -191.323074341F },
                              { -3.770766735F, 0.833341539F },
                              AnimationType::Stand,
                              AnimationType::RunBack,
                              2,
                              5,
                              3,
                              1,
                              false },
        CannonballCheckpoint{ 76,
                              { -613.849609375F, -172.566818237F },
                              { -3.609596729F, 0.197497725F },
                              AnimationType::Stand,
                              AnimationType::RunBack,
                              7,
                              21,
                              3,
                              1,
                              true },
        CannonballCheckpoint{ 90,
                              { -664.459716797F, -168.666091919F },
                              { -3.424826145F, 0.208273202F },
                              AnimationType::Stand,
                              AnimationType::RunBack,
                              12,
                              2,
                              3,
                              1,
                              true },
        CannonballCheckpoint{ 120,
                              { -763.830444336F, -163.726547241F },
                              { -3.051406622F, 0.120759383F },
                              AnimationType::Stand,
                              AnimationType::RunBack,
                              5,
                              32,
                              3,
                              1,
                              true },
        CannonballCheckpoint{ 179,
                              { -946.192871094F, -154.578063965F },
                              { -2.909185410F, 0.178140655F },
                              AnimationType::Stand,
                              AnimationType::RunBack,
                              7,
                              25,
                              3,
                              1,
                              true },
    };
    CheckCannonball(ScenarioId::CannonballLeft, checkpoints);
}

TEST(MovementScenario, CannonballRightCheckpoints)
{
    const std::array checkpoints{
        CannonballCheckpoint{ 0,
                              { -400.000000000F, -142.051254272F },
                              { 0.000000000F, 0.000000000F },
                              AnimationType::Stand,
                              AnimationType::RunBack,
                              13,
                              2,
                              3,
                              1,
                              true },
        CannonballCheckpoint{ 14,
                              { -391.634582520F, -141.738662720F },
                              { 1.011716843F, 0.036288284F },
                              AnimationType::Stand,
                              AnimationType::RunBack,
                              1,
                              16,
                              3,
                              1,
                              true },
        CannonballCheckpoint{ 15,
                              { -390.591308594F, -141.673324585F },
                              { 1.001861453F, 0.062451664F },
                              AnimationType::Stand,
                              AnimationType::JumpSide,
                              1,
                              2,
                              3,
                              1,
                              true },
        CannonballCheckpoint{ 18,
                              { -387.653045654F, -141.573669434F },
                              { 0.926209331F, -0.004657770F },
                              AnimationType::Stand,
                              AnimationType::JumpSide,
                              2,
                              5,
                              3,
                              1,
                              true },
        CannonballCheckpoint{ 24,
                              { -376.036865234F, -145.525070190F },
                              { 2.610047340F, -1.105143666F },
                              AnimationType::Stand,
                              AnimationType::JumpSide,
                              4,
                              11,
                              3,
                              1,
                              false },
        CannonballCheckpoint{ 25,
                              { -373.126831055F, -146.820220947F },
                              { 2.880946875F, -1.282192230F },
                              AnimationType::RollBack,
                              AnimationType::RollBack,
                              2,
                              2,
                              1,
                              1,
                              false },
        CannonballCheckpoint{ 26,
                              { -370.185882568F, -148.142410278F },
                              { 2.911537409F, -1.308970332F },
                              AnimationType::RollBack,
                              AnimationType::RollBack,
                              3,
                              3,
                              1,
                              1,
                              false },
        CannonballCheckpoint{ 30,
                              { -358.119232178F, -152.706268311F },
                              { 3.030870914F, -1.023332357F },
                              AnimationType::RollBack,
                              AnimationType::RollBack,
                              7,
                              7,
                              1,
                              1,
                              false },
        CannonballCheckpoint{ 40,
                              { -325.935668945F, -159.288208008F },
                              { 3.309035540F, -0.357513040F },
                              AnimationType::RollBack,
                              AnimationType::RollBack,
                              17,
                              17,
                              1,
                              1,
                              false },
        CannonballCheckpoint{ 55,
                              { -272.753784180F, -157.416625977F },
                              { 3.677216768F, 0.523771286F },
                              AnimationType::RollBack,
                              AnimationType::RollBack,
                              32,
                              32,
                              1,
                              1,
                              false },
        CannonballCheckpoint{ 56,
                              { -269.016571045F, -156.832855225F },
                              { 3.699844599F, 0.577933609F },
                              AnimationType::Stand,
                              AnimationType::CrouchRunBack,
                              1,
                              1,
                              3,
                              2,
                              false },
        CannonballCheckpoint{ 57,
                              { -265.316711426F, -156.194915771F },
                              { 3.662846088F, 0.631554306F },
                              AnimationType::Aim,
                              AnimationType::RunBack,
                              1,
                              2,
                              2,
                              1,
                              false },
        CannonballCheckpoint{ 58,
                              { -261.623870850F, -155.503356934F },
                              { 3.655917645F, 0.684638798F },
                              AnimationType::Stand,
                              AnimationType::RunBack,
                              1,
                              3,
                              3,
                              1,
                              false },
        CannonballCheckpoint{ 60,
                              { -254.258895874F, -153.961517334F },
                              { 3.642267942F, 0.789220452F },
                              AnimationType::Stand,
                              AnimationType::RunBack,
                              2,
                              5,
                              3,
                              1,
                              false },
        CannonballCheckpoint{ 76,
                              { -196.272857666F, -134.472335815F },
                              { 3.542407990F, 1.554328322F },
                              AnimationType::Stand,
                              AnimationType::RunBack,
                              7,
                              21,
                              3,
                              1,
                              false },
        CannonballCheckpoint{ 90,
                              { -147.973449707F, -129.286087036F },
                              { 3.304593325F, 0.184143886F },
                              AnimationType::Stand,
                              AnimationType::RunBack,
                              12,
                              2,
                              3,
                              1,
                              true },
        CannonballCheckpoint{ 120,
                              { -50.290573120F, -124.436393738F },
                              { 3.106262922F, 0.178930789F },
                              AnimationType::Stand,
                              AnimationType::RunBack,
                              5,
                              32,
                              3,
                              1,
                              true },
        CannonballCheckpoint{ 179,
                              { 132.116195679F, -115.302947998F },
                              { 2.995264053F, 0.129363239F },
                              AnimationType::Stand,
                              AnimationType::RunBack,
                              7,
                              25,
                              3,
                              1,
                              true },
    };
    CheckCannonball(ScenarioId::CannonballRight, checkpoints);
}
