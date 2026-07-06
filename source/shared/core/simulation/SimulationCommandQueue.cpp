module;

#include <utility>
#include <vector>

export module Shared.Core.Simulation.SimulationCommandQueue;

import Shared.Core.Simulation.SimulationCommands;

export namespace Soldank
{
class SimulationCommandQueue
{
public:
    void Enqueue(SimulationCommand command) { commands_.push_back(std::move(command)); }

    std::vector<SimulationCommand> Drain()
    {
        std::vector<SimulationCommand> drained_commands;
        drained_commands.swap(commands_);
        return drained_commands;
    }

    bool IsEmpty() const { return commands_.empty(); }

private:
    std::vector<SimulationCommand> commands_;
};
} // namespace Soldank
