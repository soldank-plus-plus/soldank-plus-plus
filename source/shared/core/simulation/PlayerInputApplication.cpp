export module Shared.Core.Simulation.PlayerInputApplication;

import Shared.Core.Entities.Soldier;
import Shared.Core.Types.WeaponType;
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
    if (command.appearance && command.appearance->IsValid()) {
        state_manager.TransformSoldier(command.soldier_id, [&](Soldier& soldier) {
            if (soldier.appearance.headgear != command.appearance->headgear) {
                soldier.wear_helmet = command.appearance->headgear != 0 ? 1 : 0;
            }
            if (soldier.appearance.cigar != command.appearance->cigar) {
                soldier.has_cigar = command.appearance->cigar != 0 ? 5 : 0;
            }
            soldier.appearance = *command.appearance;
        });
    }
    if (command.weapon_choices && IsPrimaryWeaponChoice(command.weapon_choices->at(0)) &&
        IsSecondaryWeaponChoice(command.weapon_choices->at(1))) {
        state_manager.TransformSoldier(command.soldier_id, [&](Soldier& soldier) {
            soldier.weapon_choices = *command.weapon_choices;
        });
    }
}
} // namespace Soldank
