export module Shared.Core.Simulation.PlayerInputApplication;

import Shared.Core.Entities.Soldier;
import Shared.Core.Simulation.SimulationCommands;
import Shared.Core.State.Control;
import Shared.Core.State.StateManager;

export namespace Soldank
{
void ApplyPlayerInputCommand(StateManager& state_manager, const PlayerInputCommand& command)
{
    state_manager.SoldierControlApply(
      command.soldier_id,
      [&](const Soldier& /*soldier*/, Control& control) { control = command.control; });
    state_manager.ChangeSoldierMouseMapPosition(command.soldier_id, command.mouse_map_position);
}
} // namespace Soldank
