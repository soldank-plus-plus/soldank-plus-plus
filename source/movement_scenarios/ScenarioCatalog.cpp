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
    definition.description = "Soldier idling while looking left.";
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
    definition.description = "Soldier idling while looking right.";
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
    definition.description = "Soldier running left.";
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
    definition.description = "Soldier running right.";
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
    definition.description = "Soldier jumping left and stopping.";
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
    definition.description = "Soldier jumping right and stopping.";
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
    builder.ChangeAt(33, ControlActionType::UseJets, false);
    builder.ChangeAt(40, ControlActionType::Jump, false);
    builder.ChangeAt(120, ControlActionType::MoveLeft, false);
    auto definition = builder.Build();
    definition.description = "Late backflip to the left.";
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
    builder.ChangeAt(33, ControlActionType::UseJets, false);
    builder.ChangeAt(40, ControlActionType::Jump, false);
    builder.ChangeAt(120, ControlActionType::MoveRight, false);
    auto definition = builder.Build();
    definition.description = "Late backflip to the right.";
    return definition;
}

ScenarioDefinition MakeBackflipLeft()
{
    ScenarioBuilder builder(ScenarioId::BackflipLeft);
    builder.AddPolygon({ -1600, 0 }, { 1600, 0 }, { 1600, 400 }, PMSPolygonType::Normal);
    builder.AddPolygon({ -1600, 0 }, { 1600, 400 }, { -1600, 400 }, PMSPolygonType::Normal);
    builder.SpawnSoldier({ 400, -34 });
    builder.AimRelativeToSoldier({ 6400, 0 });
    builder.SettleBeforePlayback(5, 5000);
    builder.SetDurationTicks(180);
    builder.ChangeAt(0, ControlActionType::MoveLeft, true);
    builder.ChangeAt(0, ControlActionType::Jump, true);
    builder.ChangeAt(9, ControlActionType::UseJets, true);
    builder.ChangeAt(10, ControlActionType::UseJets, false);
    builder.ChangeAt(17, ControlActionType::Jump, false);
    builder.ChangeAt(36, ControlActionType::MoveLeft, false);
    auto definition = builder.Build();
    definition.description = "Soldier performs a backflip to the left.";
    return definition;
}

ScenarioDefinition MakeBackflipRight()
{
    ScenarioBuilder builder(ScenarioId::BackflipRight);
    builder.AddPolygon({ -1600, 0 }, { 1600, 0 }, { 1600, 400 }, PMSPolygonType::Normal);
    builder.AddPolygon({ -1600, 0 }, { 1600, 400 }, { -1600, 400 }, PMSPolygonType::Normal);
    builder.SpawnSoldier({ -400, -34 });
    builder.AimRelativeToSoldier({ -6400, 0 });
    builder.SettleBeforePlayback(5, 5000);
    builder.SetDurationTicks(180);
    builder.ChangeAt(0, ControlActionType::MoveRight, true);
    builder.ChangeAt(0, ControlActionType::Jump, true);
    builder.ChangeAt(9, ControlActionType::UseJets, true);
    builder.ChangeAt(10, ControlActionType::UseJets, false);
    builder.ChangeAt(17, ControlActionType::Jump, false);
    builder.ChangeAt(36, ControlActionType::MoveRight, false);
    auto definition = builder.Build();
    definition.description = "Soldier performs a backflip to the right.";
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
    builder.ChangeAt(1, ControlActionType::UseJets, true);
    builder.ChangeAt(7, ControlActionType::Jump, false);
    builder.ChangeAt(7, ControlActionType::UseJets, false);
    builder.ChangeAt(9, ControlActionType::MoveLeft, false);
    auto definition = builder.Build();
    definition.description = "Soldier performs miniflip left.";
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
    builder.ChangeAt(1, ControlActionType::UseJets, true);
    builder.ChangeAt(7, ControlActionType::Jump, false);
    builder.ChangeAt(7, ControlActionType::UseJets, false);
    builder.ChangeAt(9, ControlActionType::MoveRight, false);
    auto definition = builder.Build();
    definition.description = "Soldier performs miniflip right.";
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
    builder.ChangeAt(10, ControlActionType::Jump, true);
    builder.ChangeAt(18, ControlActionType::Jump, false);
    builder.ChangeAt(45, ControlActionType::Jump, true);
    builder.ChangeAt(53, ControlActionType::Jump, false);
    builder.ChangeAt(82, ControlActionType::Jump, true);
    builder.ChangeAt(90, ControlActionType::Jump, false);
    builder.ChangeAt(119, ControlActionType::Jump, true);
    builder.ChangeAt(127, ControlActionType::Jump, false);
    auto definition = builder.Build();
    definition.description = "Soldier performs small tap jumps left.";
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
    builder.ChangeAt(10, ControlActionType::Jump, true);
    builder.ChangeAt(18, ControlActionType::Jump, false);
    builder.ChangeAt(45, ControlActionType::Jump, true);
    builder.ChangeAt(53, ControlActionType::Jump, false);
    builder.ChangeAt(82, ControlActionType::Jump, true);
    builder.ChangeAt(90, ControlActionType::Jump, false);
    builder.ChangeAt(119, ControlActionType::Jump, true);
    builder.ChangeAt(127, ControlActionType::Jump, false);
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

ScenarioDefinition MakeFakeCannonballLeft()
{
    ScenarioBuilder builder(ScenarioId::FakeCannonballLeft);
    builder.AddPolygon({ -1600, 0 }, { 1600, -160 }, { 1600, 400 }, PMSPolygonType::Normal);
    builder.AddPolygon({ -1600, 0 }, { 1600, 400 }, { -1600, 400 }, PMSPolygonType::Normal);
    builder.SpawnSoldier({ -400, -94 });
    builder.AimRelativeToSoldier({ 6400, 0 });
    builder.SettleBeforePlayback(5, 5000);
    builder.SetDurationTicks(180);
    builder.ChangeAt(0, ControlActionType::MoveLeft, true);
    builder.ChangeAt(15, ControlActionType::Jump, true);
    builder.ChangeAt(20, ControlActionType::Crouch, true);
    builder.ChangeAt(21, ControlActionType::Jump, false);
    builder.ChangeAt(38, ControlActionType::UseJets, true);
    builder.ChangeAt(39, ControlActionType::Crouch, false);
    builder.ChangeAt(39, ControlActionType::UseJets, false);
    auto definition = builder.Build();
    definition.description = "Soldier doing fake cannonball to the left";
    return definition;
}

ScenarioDefinition MakeFakeCannonballRight()
{
    ScenarioBuilder builder(ScenarioId::FakeCannonballRight);
    builder.AddPolygon({ -1600, 0 }, { 1600, 160 }, { 1600, 400 }, PMSPolygonType::Normal);
    builder.AddPolygon({ -1600, 0 }, { 1600, 400 }, { -1600, 400 }, PMSPolygonType::Normal);
    builder.SpawnSoldier({ -400, -94 });
    builder.AimRelativeToSoldier({ -6400, 0 });
    builder.SettleBeforePlayback(5, 5000);
    builder.SetDurationTicks(180);
    builder.ChangeAt(0, ControlActionType::MoveRight, true);
    builder.ChangeAt(15, ControlActionType::Jump, true);
    builder.ChangeAt(20, ControlActionType::Crouch, true);
    builder.ChangeAt(21, ControlActionType::Jump, false);
    builder.ChangeAt(38, ControlActionType::UseJets, true);
    builder.ChangeAt(39, ControlActionType::Crouch, false);
    builder.ChangeAt(39, ControlActionType::UseJets, false);
    auto definition = builder.Build();
    definition.description = "Soldier doing fake cannonball to the left";
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
    Registration{ ScenarioId::BackflipLeft, MakeBackflipLeft },
    Registration{ ScenarioId::BackflipRight, MakeBackflipRight },
    Registration{ ScenarioId::MiniflipLeft, MakeMiniflipLeft },
    Registration{ ScenarioId::MiniflipRight, MakeMiniflipRight },
    Registration{ ScenarioId::SlopeJumpLeft, MakeSlopeJumpLeft },
    Registration{ ScenarioId::SlopeJumpRight, MakeSlopeJumpRight },
    Registration{ ScenarioId::SlopeTapsLeft, MakeSlopeTapsLeft },
    Registration{ ScenarioId::SlopeTapsRight, MakeSlopeTapsRight },
    Registration{ ScenarioId::CannonballLeft, MakeCannonballLeft },
    Registration{ ScenarioId::CannonballRight, MakeCannonballRight },
    Registration{ ScenarioId::FakeCannonballLeft, MakeFakeCannonballLeft },
    Registration{ ScenarioId::FakeCannonballRight, MakeFakeCannonballRight }
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
