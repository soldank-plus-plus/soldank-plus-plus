module;
#include <array>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
export module MovementScenarios.Id;
export namespace Soldank::MovementScenarios
{
enum class ScenarioId
{
    IdleLeft,
    IdleRight,
    RunLeft,
    RunRight,
    SideJumpLeft,
    SideJumpRight,
    LateBackflipLeft,
    LateBackflipRight,
    StaggeredCannonballLeft,
    StaggeredCannonballRight,
    SimultaneousCannonballLeft,
    SimultaneousCannonballRight,
    EarlyBackflipLeft,
    EarlyBackflipRight,
    MiniflipLeft,
    MiniflipRight,
    SlopeJumpLeft,
    SlopeJumpRight,
    SlopeTapsLeft,
    SlopeTapsRight,
    CannonballLeft,
    CannonballRight,
    Count
};
} // namespace Soldank::MovementScenarios
namespace Soldank::MovementScenarios
{
struct ScenarioName
{
    ScenarioId id;
    std::string_view name;
};
constexpr std::array SCENARIO_NAMES{
    ScenarioName{ ScenarioId::IdleLeft, "idle_left" },
    ScenarioName{ ScenarioId::IdleRight, "idle_right" },
    ScenarioName{ ScenarioId::RunLeft, "run_left" },
    ScenarioName{ ScenarioId::RunRight, "run_right" },
    ScenarioName{ ScenarioId::SideJumpLeft, "side_jump_left" },
    ScenarioName{ ScenarioId::SideJumpRight, "side_jump_right" },
    ScenarioName{ ScenarioId::LateBackflipLeft, "late_backflip_left" },
    ScenarioName{ ScenarioId::LateBackflipRight, "late_backflip_right" },
    ScenarioName{ ScenarioId::StaggeredCannonballLeft, "staggered_cannonball_left" },
    ScenarioName{ ScenarioId::StaggeredCannonballRight, "staggered_cannonball_right" },
    ScenarioName{ ScenarioId::SimultaneousCannonballLeft, "simultaneous_cannonball_left" },
    ScenarioName{ ScenarioId::SimultaneousCannonballRight, "simultaneous_cannonball_right" },
    ScenarioName{ ScenarioId::EarlyBackflipLeft, "early_backflip_left" },
    ScenarioName{ ScenarioId::EarlyBackflipRight, "early_backflip_right" },
    ScenarioName{ ScenarioId::MiniflipLeft, "miniflip_left" },
    ScenarioName{ ScenarioId::MiniflipRight, "miniflip_right" },
    ScenarioName{ ScenarioId::SlopeJumpLeft, "slope_jump_left" },
    ScenarioName{ ScenarioId::SlopeJumpRight, "slope_jump_right" },
    ScenarioName{ ScenarioId::SlopeTapsLeft, "slope_taps_left" },
    ScenarioName{ ScenarioId::SlopeTapsRight, "slope_taps_right" },
    ScenarioName{ ScenarioId::CannonballLeft, "cannonball_left" },
    ScenarioName{ ScenarioId::CannonballRight, "cannonball_right" }
};
static_assert(
  [] {
      std::array<bool, static_cast<unsigned int>(ScenarioId::Count)> seen{};
      if (seen.size() != SCENARIO_NAMES.size()) {
          return false;
      }
      for (const auto& entry : SCENARIO_NAMES) {
          const auto index = static_cast<unsigned int>(entry.id);
          if (index >= seen.size() || seen[index]) {
              return false;
          }
          seen[index] = true;
          if (entry.name.empty()) {
              return false;
          }
          for (const auto& other : SCENARIO_NAMES) {
              if (entry.id != other.id && entry.name == other.name) {
                  return false;
              }
          }
      }
      return true;
  }(),
  "Every ScenarioId needs a unique readable name");
} // namespace Soldank::MovementScenarios
export namespace Soldank::MovementScenarios
{
std::string ScenarioIdToString(ScenarioId id)
{
    for (const auto& entry : SCENARIO_NAMES) {
        if (entry.id == id) {
            return std::string(entry.name);
        }
    }
    throw std::invalid_argument("Invalid scenario ID");
}
ScenarioId ScenarioIdFromString(std::string_view name)
{
    for (const auto& entry : SCENARIO_NAMES) {
        if (entry.name == name) {
            return entry.id;
        }
    }
    throw std::invalid_argument("Unknown scenario: " + std::string(name));
}
std::vector<ScenarioId> GetScenarioIds()
{
    std::vector<ScenarioId> ids;
    for (const auto& entry : SCENARIO_NAMES) {
        ids.push_back(entry.id);
    }
    return ids;
}
} // namespace Soldank::MovementScenarios
