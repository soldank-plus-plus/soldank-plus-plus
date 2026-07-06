module;

#include <utility>
#include <vector>

export module Runtime.ServerCommandQueues;

import Shared.Core.Simulation.SimulationCommands;

export namespace Soldank
{
class ServerCommandQueues
{
public:
    void EnqueuePlayerInput(PlayerInputCommand command)
    {
        player_inputs_.push_back(std::move(command));
    }

    void EnqueueSimulationCommand(SimulationCommand command)
    {
        simulation_commands_.push_back(std::move(command));
    }

    std::vector<PlayerInputCommand> DrainPlayerInputs()
    {
        std::vector<PlayerInputCommand> player_inputs;
        player_inputs.swap(player_inputs_);
        return player_inputs;
    }

    std::vector<SimulationCommand> DrainSimulationCommands()
    {
        std::vector<SimulationCommand> simulation_commands;
        simulation_commands.swap(simulation_commands_);
        return simulation_commands;
    }

private:
    std::vector<PlayerInputCommand> player_inputs_;
    std::vector<SimulationCommand> simulation_commands_;
};
} // namespace Soldank
