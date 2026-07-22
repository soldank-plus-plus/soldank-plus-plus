module;

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

export module Runtime.ServerCommandQueues;

import Shared.Core.Simulation.SimulationCommands;
import Shared.Core.Config.Config;
import Shared.Core.Utility.SerialNumber;

export namespace Soldank
{
#ifndef NDEBUG
struct ServerInputQueueDebugStats
{
    std::size_t received_input_count;
    std::size_t late_applied_input_count;
    std::size_t superseded_input_count;
};
#endif

class ServerCommandQueues
{
public:
    void StorePendingPlayerInput(PlayerInputCommand command)
    {
        pending_player_inputs_.at(command.soldier_id).push_back(std::move(command));
#ifndef NDEBUG
        received_input_count_++;
#endif
    }

    void EnqueueSimulationCommand(SimulationCommand command)
    {
        simulation_commands_.push_back(std::move(command));
    }

    std::vector<PlayerInputCommand> SelectPlayerInputsForSimulation(std::uint32_t server_tick)
    {
        std::vector<PlayerInputCommand> player_inputs;
        for (auto& pending_player_inputs : pending_player_inputs_) {
            std::optional<PlayerInputCommand> newest_due_input;
            for (auto input = pending_player_inputs.begin();
                 input != pending_player_inputs.end();) {
                if (!IsSerialNumberNewer(input->apply_server_tick, server_tick)) {
#ifndef NDEBUG
                    if (IsSerialNumberOlder(input->apply_server_tick, server_tick)) {
                        late_applied_input_count_++;
                    }
                    if (newest_due_input.has_value()) {
                        superseded_input_count_++;
                    }
#endif
                    newest_due_input = std::move(*input);
                    input = pending_player_inputs.erase(input);
                } else {
                    ++input;
                }
            }
            if (newest_due_input.has_value()) {
                player_inputs.push_back(std::move(*newest_due_input));
            }
        }
        return player_inputs;
    }

#ifndef NDEBUG
    ServerInputQueueDebugStats GetInputDebugStats() const
    {
        return { .received_input_count = received_input_count_,
                 .late_applied_input_count = late_applied_input_count_,
                 .superseded_input_count = superseded_input_count_ };
    }

    void ResetInputDebugStats()
    {
        received_input_count_ = 0;
        late_applied_input_count_ = 0;
        superseded_input_count_ = 0;
    }
#endif

    std::vector<SimulationCommand> DrainSimulationCommands()
    {
        std::vector<SimulationCommand> simulation_commands;
        simulation_commands.swap(simulation_commands_);
        return simulation_commands;
    }

private:
    std::array<std::vector<PlayerInputCommand>, Config::MAX_PLAYERS> pending_player_inputs_{};
    std::vector<SimulationCommand> simulation_commands_;
#ifndef NDEBUG
    std::size_t received_input_count_ = 0;
    std::size_t late_applied_input_count_ = 0;
    std::size_t superseded_input_count_ = 0;
#endif
};
} // namespace Soldank
