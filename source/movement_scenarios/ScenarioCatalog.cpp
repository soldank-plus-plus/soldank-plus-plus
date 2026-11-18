module;
#include <array>
#include <stdexcept>
#include <vector>
export module MovementScenarios.Catalog;
export import MovementScenarios.Definition;
namespace Soldank::MovementScenarios
{
ScenarioDefinition MakeIdleLeft()
{
    ScenarioBuilder builder(ScenarioId::IdleLeft);
    builder.AddPolygon({ -1600, 0 }, { 1600, 0 }, { 1600, 400 }, PMSPolygonType::Normal);
    builder.AddPolygon({ -1600, 0 }, { 1600, 400 }, { -1600, 400 }, PMSPolygonType::Normal);
    builder.SpawnSoldier({ 400, -34 });
    builder.AimRelativeToSoldier({ -6400, 0 });
    builder.SettleBeforePlayback(5, 5000);
    builder.SetDurationTicks(180);
    auto definition = builder.Build();
    definition.description = "Authored idle_left input schedule. Inspect and calibrate before "
                             "approving regression checkpoints.";
    return definition;
}

ScenarioDefinition MakeIdleRight()
{
    ScenarioBuilder builder(ScenarioId::IdleRight);
    builder.AddPolygon({ -1600, 0 }, { 1600, 0 }, { 1600, 400 }, PMSPolygonType::Normal);
    builder.AddPolygon({ -1600, 0 }, { 1600, 400 }, { -1600, 400 }, PMSPolygonType::Normal);
    builder.SpawnSoldier({ -400, -34 });
    builder.AimRelativeToSoldier({ 6400, 0 });
    builder.SettleBeforePlayback(5, 5000);
    builder.SetDurationTicks(180);
    auto definition = builder.Build();
    definition.description = "Authored idle_right input schedule. Inspect and calibrate before "
                             "approving regression checkpoints.";
    return definition;
}

ScenarioDefinition MakeRunLeft()
{
    ScenarioBuilder builder(ScenarioId::RunLeft);
    builder.AddPolygon({ -1600, 0 }, { 1600, 0 }, { 1600, 400 }, PMSPolygonType::Normal);
    builder.AddPolygon({ -1600, 0 }, { 1600, 400 }, { -1600, 400 }, PMSPolygonType::Normal);
    builder.SpawnSoldier({ 400, -34 });
    builder.AimRelativeToSoldier({ 6400, 0 });
    builder.SettleBeforePlayback(5, 5000);
    builder.SetDurationTicks(180);
    builder.ChangeAt(0, ControlActionType::MoveLeft, true);
    auto definition = builder.Build();
    definition.description = "Authored run_left input schedule. Inspect and calibrate before "
                             "approving regression checkpoints.";
    return definition;
}

ScenarioDefinition MakeRunRight()
{
    ScenarioBuilder builder(ScenarioId::RunRight);
    builder.AddPolygon({ -1600, 0 }, { 1600, 0 }, { 1600, 400 }, PMSPolygonType::Normal);
    builder.AddPolygon({ -1600, 0 }, { 1600, 400 }, { -1600, 400 }, PMSPolygonType::Normal);
    builder.SpawnSoldier({ -400, -34 });
    builder.AimRelativeToSoldier({ -6400, 0 });
    builder.SettleBeforePlayback(5, 5000);
    builder.SetDurationTicks(180);
    builder.ChangeAt(0, ControlActionType::MoveRight, true);
    auto definition = builder.Build();
    definition.description = "Authored run_right input schedule. Inspect and calibrate before "
                             "approving regression checkpoints.";
    return definition;
}

ScenarioDefinition MakeSideJumpLeft()
{
    ScenarioBuilder builder(ScenarioId::SideJumpLeft);
    builder.AddPolygon({ -1600, 0 }, { 1600, 0 }, { 1600, 400 }, PMSPolygonType::Normal);
    builder.AddPolygon({ -1600, 0 }, { 1600, 400 }, { -1600, 400 }, PMSPolygonType::Normal);
    builder.SpawnSoldier({ 400, -34 });
    builder.AimRelativeToSoldier({ 6400, 0 });
    builder.SettleBeforePlayback(5, 5000);
    builder.SetDurationTicks(180);
    builder.ChangeAt(0, ControlActionType::MoveLeft, true);
    builder.ChangeAt(1, ControlActionType::Jump, true);
    builder.ChangeAt(35, ControlActionType::Jump, false);
    builder.ChangeAt(35, ControlActionType::UseJets, false);
    builder.ChangeAt(35, ControlActionType::MoveLeft, false);
    auto definition = builder.Build();
    definition.description = "Authored side_jump_left input schedule. Inspect and calibrate before "
                             "approving regression checkpoints.";
    return definition;
}

ScenarioDefinition MakeSideJumpRight()
{
    ScenarioBuilder builder(ScenarioId::SideJumpRight);
    builder.AddPolygon({ -1600, 0 }, { 1600, 0 }, { 1600, 400 }, PMSPolygonType::Normal);
    builder.AddPolygon({ -1600, 0 }, { 1600, 400 }, { -1600, 400 }, PMSPolygonType::Normal);
    builder.SpawnSoldier({ -400, -34 });
    builder.AimRelativeToSoldier({ -6400, 0 });
    builder.SettleBeforePlayback(5, 5000);
    builder.SetDurationTicks(180);
    builder.ChangeAt(0, ControlActionType::MoveRight, true);
    builder.ChangeAt(1, ControlActionType::Jump, true);
    builder.ChangeAt(35, ControlActionType::Jump, false);
    builder.ChangeAt(35, ControlActionType::UseJets, false);
    builder.ChangeAt(35, ControlActionType::MoveRight, false);
    auto definition = builder.Build();
    definition.description = "Authored side_jump_right input schedule. Inspect and calibrate "
                             "before approving regression checkpoints.";
    return definition;
}

ScenarioDefinition MakeLateBackflipLeft()
{
    ScenarioBuilder builder(ScenarioId::LateBackflipLeft);
    builder.AddPolygon({ -1600, 0 }, { 1600, 0 }, { 1600, 400 }, PMSPolygonType::Normal);
    builder.AddPolygon({ -1600, 0 }, { 1600, 400 }, { -1600, 400 }, PMSPolygonType::Normal);
    builder.SpawnSoldier({ 400, -34 });
    builder.AimRelativeToSoldier({ 6400, 0 });
    builder.SettleBeforePlayback(5, 5000);
    builder.SetDurationTicks(180);
    builder.ChangeAt(0, ControlActionType::MoveLeft, true);
    builder.ChangeAt(1, ControlActionType::Jump, true);
    builder.ChangeAt(32, ControlActionType::UseJets, true);
    builder.ChangeAt(35, ControlActionType::Jump, false);
    builder.ChangeAt(35, ControlActionType::UseJets, false);
    builder.ChangeAt(35, ControlActionType::MoveLeft, false);
    auto definition = builder.Build();
    definition.description = "Authored late_backflip_left input schedule. Inspect and calibrate "
                             "before approving regression checkpoints.";
    return definition;
}

ScenarioDefinition MakeLateBackflipRight()
{
    ScenarioBuilder builder(ScenarioId::LateBackflipRight);
    builder.AddPolygon({ -1600, 0 }, { 1600, 0 }, { 1600, 400 }, PMSPolygonType::Normal);
    builder.AddPolygon({ -1600, 0 }, { 1600, 400 }, { -1600, 400 }, PMSPolygonType::Normal);
    builder.SpawnSoldier({ -400, -34 });
    builder.AimRelativeToSoldier({ -6400, 0 });
    builder.SettleBeforePlayback(5, 5000);
    builder.SetDurationTicks(180);
    builder.ChangeAt(0, ControlActionType::MoveRight, true);
    builder.ChangeAt(1, ControlActionType::Jump, true);
    builder.ChangeAt(32, ControlActionType::UseJets, true);
    builder.ChangeAt(35, ControlActionType::Jump, false);
    builder.ChangeAt(35, ControlActionType::UseJets, false);
    builder.ChangeAt(35, ControlActionType::MoveRight, false);
    auto definition = builder.Build();
    definition.description = "Authored late_backflip_right input schedule. Inspect and calibrate "
                             "before approving regression checkpoints.";
    return definition;
}

ScenarioDefinition MakeStaggeredCannonballLeft()
{
    ScenarioBuilder builder(ScenarioId::StaggeredCannonballLeft);
    builder.AddPolygon({ -1600, 0 }, { 1600, 0 }, { 1600, 400 }, PMSPolygonType::Normal);
    builder.AddPolygon({ -1600, 0 }, { 1600, 400 }, { -1600, 400 }, PMSPolygonType::Normal);
    builder.SpawnSoldier({ 400, -34 });
    builder.AimRelativeToSoldier({ 6400, 0 });
    builder.SettleBeforePlayback(5, 5000);
    builder.SetDurationTicks(180);
    builder.ChangeAt(0, ControlActionType::MoveLeft, true);
    builder.ChangeAt(1, ControlActionType::Jump, true);
    builder.ChangeAt(12, ControlActionType::UseJets, true);
    builder.ChangeAt(14, ControlActionType::Jump, false);
    builder.ChangeAt(19, ControlActionType::UseJets, false);
    builder.ChangeAt(14, ControlActionType::MoveLeft, false);
    auto definition = builder.Build();
    definition.description = "Authored staggered_cannonball_left input schedule. Inspect and "
                             "calibrate before approving regression checkpoints.";
    return definition;
}

ScenarioDefinition MakeStaggeredCannonballRight()
{
    ScenarioBuilder builder(ScenarioId::StaggeredCannonballRight);
    builder.AddPolygon({ -1600, 0 }, { 1600, 0 }, { 1600, 400 }, PMSPolygonType::Normal);
    builder.AddPolygon({ -1600, 0 }, { 1600, 400 }, { -1600, 400 }, PMSPolygonType::Normal);
    builder.SpawnSoldier({ -400, -34 });
    builder.AimRelativeToSoldier({ -6400, 0 });
    builder.SettleBeforePlayback(5, 5000);
    builder.SetDurationTicks(180);
    builder.ChangeAt(0, ControlActionType::MoveRight, true);
    builder.ChangeAt(1, ControlActionType::Jump, true);
    builder.ChangeAt(12, ControlActionType::UseJets, true);
    builder.ChangeAt(14, ControlActionType::Jump, false);
    builder.ChangeAt(19, ControlActionType::UseJets, false);
    builder.ChangeAt(14, ControlActionType::MoveRight, false);
    auto definition = builder.Build();
    definition.description = "Authored staggered_cannonball_right input schedule. Inspect and "
                             "calibrate before approving regression checkpoints.";
    return definition;
}

ScenarioDefinition MakeSimultaneousCannonballLeft()
{
    ScenarioBuilder builder(ScenarioId::SimultaneousCannonballLeft);
    builder.AddPolygon({ -1600, 0 }, { 1600, 0 }, { 1600, 400 }, PMSPolygonType::Normal);
    builder.AddPolygon({ -1600, 0 }, { 1600, 400 }, { -1600, 400 }, PMSPolygonType::Normal);
    builder.SpawnSoldier({ 400, -34 });
    builder.AimRelativeToSoldier({ 6400, 0 });
    builder.SettleBeforePlayback(5, 5000);
    builder.SetDurationTicks(180);
    builder.ChangeAt(0, ControlActionType::MoveLeft, true);
    builder.ChangeAt(1, ControlActionType::Jump, true);
    builder.ChangeAt(12, ControlActionType::UseJets, true);
    builder.ChangeAt(12, ControlActionType::Jump, false);
    builder.ChangeAt(19, ControlActionType::UseJets, false);
    builder.ChangeAt(14, ControlActionType::MoveLeft, false);
    auto definition = builder.Build();
    definition.description = "Authored simultaneous_cannonball_left input schedule. Inspect and "
                             "calibrate before approving regression checkpoints.";
    return definition;
}

ScenarioDefinition MakeSimultaneousCannonballRight()
{
    ScenarioBuilder builder(ScenarioId::SimultaneousCannonballRight);
    builder.AddPolygon({ -1600, 0 }, { 1600, 0 }, { 1600, 400 }, PMSPolygonType::Normal);
    builder.AddPolygon({ -1600, 0 }, { 1600, 400 }, { -1600, 400 }, PMSPolygonType::Normal);
    builder.SpawnSoldier({ -400, -34 });
    builder.AimRelativeToSoldier({ -6400, 0 });
    builder.SettleBeforePlayback(5, 5000);
    builder.SetDurationTicks(180);
    builder.ChangeAt(0, ControlActionType::MoveRight, true);
    builder.ChangeAt(1, ControlActionType::Jump, true);
    builder.ChangeAt(12, ControlActionType::UseJets, true);
    builder.ChangeAt(12, ControlActionType::Jump, false);
    builder.ChangeAt(19, ControlActionType::UseJets, false);
    builder.ChangeAt(14, ControlActionType::MoveRight, false);
    auto definition = builder.Build();
    definition.description = "Authored simultaneous_cannonball_right input schedule. Inspect and "
                             "calibrate before approving regression checkpoints.";
    return definition;
}

ScenarioDefinition MakeEarlyBackflipLeft()
{
    ScenarioBuilder builder(ScenarioId::EarlyBackflipLeft);
    builder.AddPolygon({ -1600, 0 }, { 1600, 0 }, { 1600, 400 }, PMSPolygonType::Normal);
    builder.AddPolygon({ -1600, 0 }, { 1600, 400 }, { -1600, 400 }, PMSPolygonType::Normal);
    builder.SpawnSoldier({ 400, -34 });
    builder.AimRelativeToSoldier({ 6400, 0 });
    builder.SettleBeforePlayback(5, 5000);
    builder.SetDurationTicks(180);
    builder.ChangeAt(0, ControlActionType::MoveLeft, true);
    builder.ChangeAt(1, ControlActionType::Jump, true);
    builder.ChangeAt(4, ControlActionType::UseJets, true);
    builder.ChangeAt(7, ControlActionType::Jump, false);
    builder.ChangeAt(7, ControlActionType::UseJets, false);
    builder.ChangeAt(7, ControlActionType::MoveLeft, false);
    auto definition = builder.Build();
    definition.description = "Authored early_backflip_left input schedule. Inspect and calibrate "
                             "before approving regression checkpoints.";
    return definition;
}

ScenarioDefinition MakeEarlyBackflipRight()
{
    ScenarioBuilder builder(ScenarioId::EarlyBackflipRight);
    builder.AddPolygon({ -1600, 0 }, { 1600, 0 }, { 1600, 400 }, PMSPolygonType::Normal);
    builder.AddPolygon({ -1600, 0 }, { 1600, 400 }, { -1600, 400 }, PMSPolygonType::Normal);
    builder.SpawnSoldier({ -400, -34 });
    builder.AimRelativeToSoldier({ -6400, 0 });
    builder.SettleBeforePlayback(5, 5000);
    builder.SetDurationTicks(180);
    builder.ChangeAt(0, ControlActionType::MoveRight, true);
    builder.ChangeAt(1, ControlActionType::Jump, true);
    builder.ChangeAt(4, ControlActionType::UseJets, true);
    builder.ChangeAt(7, ControlActionType::Jump, false);
    builder.ChangeAt(7, ControlActionType::UseJets, false);
    builder.ChangeAt(7, ControlActionType::MoveRight, false);
    auto definition = builder.Build();
    definition.description = "Authored early_backflip_right input schedule. Inspect and calibrate "
                             "before approving regression checkpoints.";
    return definition;
}

ScenarioDefinition MakeMiniflipLeft()
{
    ScenarioBuilder builder(ScenarioId::MiniflipLeft);
    builder.AddPolygon({ -1600, 0 }, { 1600, 0 }, { 1600, 400 }, PMSPolygonType::Normal);
    builder.AddPolygon({ -1600, 0 }, { 1600, 400 }, { -1600, 400 }, PMSPolygonType::Normal);
    builder.SpawnSoldier({ 400, -34 });
    builder.AimRelativeToSoldier({ 6400, 0 });
    builder.SettleBeforePlayback(5, 5000);
    builder.SetDurationTicks(180);
    builder.ChangeAt(0, ControlActionType::MoveLeft, true);
    builder.ChangeAt(1, ControlActionType::Jump, true);
    builder.ChangeAt(2, ControlActionType::UseJets, true);
    builder.ChangeAt(3, ControlActionType::Jump, false);
    builder.ChangeAt(3, ControlActionType::UseJets, false);
    builder.ChangeAt(3, ControlActionType::MoveLeft, false);
    auto definition = builder.Build();
    definition.description = "Authored miniflip_left input schedule. Inspect and calibrate before "
                             "approving regression checkpoints.";
    return definition;
}

ScenarioDefinition MakeMiniflipRight()
{
    ScenarioBuilder builder(ScenarioId::MiniflipRight);
    builder.AddPolygon({ -1600, 0 }, { 1600, 0 }, { 1600, 400 }, PMSPolygonType::Normal);
    builder.AddPolygon({ -1600, 0 }, { 1600, 400 }, { -1600, 400 }, PMSPolygonType::Normal);
    builder.SpawnSoldier({ -400, -34 });
    builder.AimRelativeToSoldier({ -6400, 0 });
    builder.SettleBeforePlayback(5, 5000);
    builder.SetDurationTicks(180);
    builder.ChangeAt(0, ControlActionType::MoveRight, true);
    builder.ChangeAt(1, ControlActionType::Jump, true);
    builder.ChangeAt(2, ControlActionType::UseJets, true);
    builder.ChangeAt(3, ControlActionType::Jump, false);
    builder.ChangeAt(3, ControlActionType::UseJets, false);
    builder.ChangeAt(3, ControlActionType::MoveRight, false);
    auto definition = builder.Build();
    definition.description = "Authored miniflip_right input schedule. Inspect and calibrate before "
                             "approving regression checkpoints.";
    return definition;
}

ScenarioDefinition MakeSlopeJumpLeft()
{
    ScenarioBuilder builder(ScenarioId::SlopeJumpLeft);
    builder.AddPolygon({ -1600, 0 }, { 1600, 160 }, { 1600, 400 }, PMSPolygonType::Normal);
    builder.AddPolygon({ -1600, 0 }, { 1600, 400 }, { -1600, 400 }, PMSPolygonType::Normal);
    builder.SpawnSoldier({ 400, 66 });
    builder.AimRelativeToSoldier({ 6400, 0 });
    builder.SettleBeforePlayback(5, 5000);
    builder.SetDurationTicks(180);
    builder.ChangeAt(0, ControlActionType::MoveLeft, true);
    builder.ChangeAt(1, ControlActionType::Jump, true);
    builder.ChangeAt(35, ControlActionType::Jump, false);
    builder.ChangeAt(35, ControlActionType::UseJets, false);
    builder.ChangeAt(35, ControlActionType::MoveLeft, false);
    auto definition = builder.Build();
    definition.description = "Authored slope_jump_left input schedule. Inspect and calibrate "
                             "before approving regression checkpoints.";
    return definition;
}

ScenarioDefinition MakeSlopeJumpRight()
{
    ScenarioBuilder builder(ScenarioId::SlopeJumpRight);
    builder.AddPolygon({ -1600, 0 }, { 1600, -160 }, { 1600, 400 }, PMSPolygonType::Normal);
    builder.AddPolygon({ -1600, 0 }, { 1600, 400 }, { -1600, 400 }, PMSPolygonType::Normal);
    builder.SpawnSoldier({ -400, -94 });
    builder.AimRelativeToSoldier({ -6400, 0 });
    builder.SettleBeforePlayback(5, 5000);
    builder.SetDurationTicks(180);
    builder.ChangeAt(0, ControlActionType::MoveRight, true);
    builder.ChangeAt(1, ControlActionType::Jump, true);
    builder.ChangeAt(35, ControlActionType::Jump, false);
    builder.ChangeAt(35, ControlActionType::UseJets, false);
    builder.ChangeAt(35, ControlActionType::MoveRight, false);
    auto definition = builder.Build();
    definition.description = "Authored slope_jump_right input schedule. Inspect and calibrate "
                             "before approving regression checkpoints.";
    return definition;
}

ScenarioDefinition MakeSlopeTapsLeft()
{
    ScenarioBuilder builder(ScenarioId::SlopeTapsLeft);
    builder.AddPolygon({ -1600, 0 }, { 1600, 160 }, { 1600, 400 }, PMSPolygonType::Normal);
    builder.AddPolygon({ -1600, 0 }, { 1600, 400 }, { -1600, 400 }, PMSPolygonType::Normal);
    builder.SpawnSoldier({ 400, 66 });
    builder.AimRelativeToSoldier({ 6400, 0 });
    builder.SettleBeforePlayback(5, 5000);
    builder.SetDurationTicks(180);
    builder.ChangeAt(0, ControlActionType::MoveLeft, true);
    builder.ChangeAt(1, ControlActionType::Jump, true);
    builder.ChangeAt(2, ControlActionType::Jump, false);
    builder.ChangeAt(9, ControlActionType::Jump, true);
    builder.ChangeAt(10, ControlActionType::Jump, false);
    builder.ChangeAt(17, ControlActionType::Jump, true);
    builder.ChangeAt(18, ControlActionType::Jump, false);
    builder.ChangeAt(25, ControlActionType::Jump, true);
    builder.ChangeAt(26, ControlActionType::Jump, false);
    builder.ChangeAt(33, ControlActionType::Jump, true);
    builder.ChangeAt(34, ControlActionType::Jump, false);
    builder.ChangeAt(41, ControlActionType::Jump, true);
    builder.ChangeAt(42, ControlActionType::Jump, false);
    builder.ChangeAt(49, ControlActionType::Jump, true);
    builder.ChangeAt(50, ControlActionType::Jump, false);
    builder.ChangeAt(57, ControlActionType::Jump, true);
    builder.ChangeAt(58, ControlActionType::Jump, false);
    builder.ChangeAt(65, ControlActionType::Jump, true);
    builder.ChangeAt(66, ControlActionType::Jump, false);
    builder.ChangeAt(73, ControlActionType::Jump, true);
    builder.ChangeAt(74, ControlActionType::Jump, false);
    builder.ChangeAt(81, ControlActionType::Jump, true);
    builder.ChangeAt(82, ControlActionType::Jump, false);
    builder.ChangeAt(89, ControlActionType::Jump, true);
    builder.ChangeAt(90, ControlActionType::Jump, false);
    auto definition = builder.Build();
    definition.description =
      "Candidate repeated uphill jump taps; inspect before approving checkpoints.";
    return definition;
}

ScenarioDefinition MakeSlopeTapsRight()
{
    ScenarioBuilder builder(ScenarioId::SlopeTapsRight);
    builder.AddPolygon({ -1600, 0 }, { 1600, -160 }, { 1600, 400 }, PMSPolygonType::Normal);
    builder.AddPolygon({ -1600, 0 }, { 1600, 400 }, { -1600, 400 }, PMSPolygonType::Normal);
    builder.SpawnSoldier({ -400, -94 });
    builder.AimRelativeToSoldier({ -6400, 0 });
    builder.SettleBeforePlayback(5, 5000);
    builder.SetDurationTicks(180);
    builder.ChangeAt(0, ControlActionType::MoveRight, true);
    builder.ChangeAt(1, ControlActionType::Jump, true);
    builder.ChangeAt(2, ControlActionType::Jump, false);
    builder.ChangeAt(9, ControlActionType::Jump, true);
    builder.ChangeAt(10, ControlActionType::Jump, false);
    builder.ChangeAt(17, ControlActionType::Jump, true);
    builder.ChangeAt(18, ControlActionType::Jump, false);
    builder.ChangeAt(25, ControlActionType::Jump, true);
    builder.ChangeAt(26, ControlActionType::Jump, false);
    builder.ChangeAt(33, ControlActionType::Jump, true);
    builder.ChangeAt(34, ControlActionType::Jump, false);
    builder.ChangeAt(41, ControlActionType::Jump, true);
    builder.ChangeAt(42, ControlActionType::Jump, false);
    builder.ChangeAt(49, ControlActionType::Jump, true);
    builder.ChangeAt(50, ControlActionType::Jump, false);
    builder.ChangeAt(57, ControlActionType::Jump, true);
    builder.ChangeAt(58, ControlActionType::Jump, false);
    builder.ChangeAt(65, ControlActionType::Jump, true);
    builder.ChangeAt(66, ControlActionType::Jump, false);
    builder.ChangeAt(73, ControlActionType::Jump, true);
    builder.ChangeAt(74, ControlActionType::Jump, false);
    builder.ChangeAt(81, ControlActionType::Jump, true);
    builder.ChangeAt(82, ControlActionType::Jump, false);
    builder.ChangeAt(89, ControlActionType::Jump, true);
    builder.ChangeAt(90, ControlActionType::Jump, false);
    auto definition = builder.Build();
    definition.description =
      "Candidate repeated uphill jump taps; inspect before approving checkpoints.";
    return definition;
}

ScenarioDefinition MakeCannonballLeft()
{
    ScenarioBuilder builder(ScenarioId::CannonballLeft);
    builder.AddPolygon({ -1600, 0 }, { 1600, -160 }, { 1600, 400 }, PMSPolygonType::Normal);
    builder.AddPolygon({ -1600, 0 }, { 1600, 400 }, { -1600, 400 }, PMSPolygonType::Normal);
    builder.SpawnSoldier({ -400, -94 });
    builder.AimRelativeToSoldier({ 6400, 0 });
    builder.SettleBeforePlayback(5, 5000);
    builder.SetDurationTicks(180);
    builder.ChangeAt(0, ControlActionType::MoveLeft, true);
    builder.ChangeAt(15, ControlActionType::Jump, true);
    builder.ChangeAt(25, ControlActionType::Jump, false);
    builder.ChangeAt(25, ControlActionType::UseJets, true);
    builder.ChangeAt(26, ControlActionType::UseJets, false);
    auto definition = builder.Build();
    definition.description = "Soldier doing cannonball to the left";
    return definition;
}

ScenarioDefinition MakeCannonballRight()
{
    ScenarioBuilder builder(ScenarioId::CannonballRight);
    builder.AddPolygon({ -1600, 0 }, { 1600, 160 }, { 1600, 400 }, PMSPolygonType::Normal);
    builder.AddPolygon({ -1600, 0 }, { 1600, 400 }, { -1600, 400 }, PMSPolygonType::Normal);
    builder.SpawnSoldier({ -400, -94 });
    builder.AimRelativeToSoldier({ -6400, 0 });
    builder.SettleBeforePlayback(5, 5000);
    builder.SetDurationTicks(180);
    builder.ChangeAt(0, ControlActionType::MoveRight, true);
    builder.ChangeAt(15, ControlActionType::Jump, true);
    builder.ChangeAt(25, ControlActionType::Jump, false);
    builder.ChangeAt(25, ControlActionType::UseJets, true);
    builder.ChangeAt(26, ControlActionType::UseJets, false);
    auto definition = builder.Build();
    definition.description = "Soldier doing cannonball to the left";
    return definition;
}

struct Registration
{
    ScenarioId id;
    ScenarioDefinition (*factory)();
};
constexpr std::array REGISTRATIONS{
    Registration{ ScenarioId::IdleLeft, MakeIdleLeft },
    Registration{ ScenarioId::IdleRight, MakeIdleRight },
    Registration{ ScenarioId::RunLeft, MakeRunLeft },
    Registration{ ScenarioId::RunRight, MakeRunRight },
    Registration{ ScenarioId::SideJumpLeft, MakeSideJumpLeft },
    Registration{ ScenarioId::SideJumpRight, MakeSideJumpRight },
    Registration{ ScenarioId::LateBackflipLeft, MakeLateBackflipLeft },
    Registration{ ScenarioId::LateBackflipRight, MakeLateBackflipRight },
    Registration{ ScenarioId::StaggeredCannonballLeft, MakeStaggeredCannonballLeft },
    Registration{ ScenarioId::StaggeredCannonballRight, MakeStaggeredCannonballRight },
    Registration{ ScenarioId::SimultaneousCannonballLeft, MakeSimultaneousCannonballLeft },
    Registration{ ScenarioId::SimultaneousCannonballRight, MakeSimultaneousCannonballRight },
    Registration{ ScenarioId::EarlyBackflipLeft, MakeEarlyBackflipLeft },
    Registration{ ScenarioId::EarlyBackflipRight, MakeEarlyBackflipRight },
    Registration{ ScenarioId::MiniflipLeft, MakeMiniflipLeft },
    Registration{ ScenarioId::MiniflipRight, MakeMiniflipRight },
    Registration{ ScenarioId::SlopeJumpLeft, MakeSlopeJumpLeft },
    Registration{ ScenarioId::SlopeJumpRight, MakeSlopeJumpRight },
    Registration{ ScenarioId::SlopeTapsLeft, MakeSlopeTapsLeft },
    Registration{ ScenarioId::SlopeTapsRight, MakeSlopeTapsRight },
    Registration{ ScenarioId::CannonballLeft, MakeCannonballLeft },
    Registration{ ScenarioId::CannonballRight, MakeCannonballRight }
};
static_assert(
  [] {
      std::array<bool, static_cast<unsigned int>(ScenarioId::Count)> seen{};
      if (seen.size() != REGISTRATIONS.size()) {
          return false;
      }
      for (const auto& registration : REGISTRATIONS) {
          const auto index = static_cast<unsigned int>(registration.id);
          if (index >= seen.size() || seen[index] || registration.factory == nullptr) {
              return false;
          }
          seen[index] = true;
      }
      return true;
  }(),
  "Register every ScenarioId exactly once");

ScenarioDefinition BuildScenario(const Registration& registration)
{
    auto definition = registration.factory();
    if (definition.id != registration.id) {
        throw std::logic_error("Scenario factory returned a different registered ID");
    }
    return definition;
}
} // namespace Soldank::MovementScenarios
export namespace Soldank::MovementScenarios
{
std::vector<ScenarioDefinition> MakeScenarios()
{
    std::vector<ScenarioDefinition> result;
    result.reserve(REGISTRATIONS.size());
    for (const auto& registration : REGISTRATIONS) {
        result.push_back(BuildScenario(registration));
    }
    return result;
}

ScenarioDefinition GetScenario(ScenarioId id)
{
    for (const auto& registration : REGISTRATIONS) {
        if (registration.id == id) {
            return BuildScenario(registration);
        }
    }
    throw std::invalid_argument("Unregistered scenario ID");
}
} // namespace Soldank::MovementScenarios
