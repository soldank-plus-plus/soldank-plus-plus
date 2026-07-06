module;

#include <span>
#include <vector>

export module Shared.Core.Simulation.WorldTick;

import Shared.Core.Simulation.SimulationCommands;
import Shared.Core.Simulation.SimulationEvents;

export namespace Soldank
{
struct WorldTickInput
{
    std::uint32_t tick = 0;
    std::span<const PlayerInputCommand> player_inputs;
    std::span<const SimulationCommand> commands;
};

struct WorldTickResult
{
    std::vector<SimulationEvent> events;
};
} // namespace Soldank
