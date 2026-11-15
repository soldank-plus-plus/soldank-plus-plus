module;

export module Shared.Core.Physics.Polygons.FlagCarry;

import Shared.Core.State.StateManager;
import Shared.Core.Entities.Item;
import Shared.Core.Entities.Soldier;
import Shared.Core.Types.ItemType;
import Shared.Core.GameModes.RuleContracts;

export namespace Soldank::FlagCarry
{
bool TryAcquire(StateManager& state, Soldier& soldier, Item& item)
{
    if (!state.HasAuthoritativeProjectiles() || !IsItemTypeFlag(item.style) || !item.active ||
        item.holding_soldier_id >= 0 || !soldier.IsAlive() || soldier.IsSpectator() ||
        soldier.GetCeaseFireCounter() >= 0) {
        return false;
    }
    const auto interaction = state.DecideFlagInteraction(soldier, item);
    if (interaction == ObjectiveInteraction::Ignore) {
        return false;
    }
    if (interaction == ObjectiveInteraction::Return) {
        item.return_requested = true;
        item.time_out = 0;
        return false;
    }
    if (soldier.is_holding_flags) {
        return false;
    }
    item.static_type = false;
    item.holding_soldier_id = soldier.id;
    item.polygon_background = soldier.polygon_background;
    soldier.is_holding_flags = true;
    return true;
}
void Refresh(StateManager& state, Soldier& soldier)
{
    if (!state.HasAuthoritativeProjectiles()) {
        return;
    }
    soldier.is_holding_flags = false;
    state.ForEachItem([&](const Item& item) {
        if (IsItemTypeFlag(item.style) && item.holding_soldier_id == soldier.id) {
            soldier.is_holding_flags = true;
        }
    });
}
} // namespace Soldank::FlagCarry
