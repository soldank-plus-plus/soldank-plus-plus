module;

#include <cstdint>

export module Shared.Core.WorldEvents;

import Shared.Core.Utility.Observable;
import Shared.Core.Entities.Soldier;
import Shared.Core.Entities.Item;

export namespace Soldank
{
struct WorldEvents
{
    Observable<const Soldier&> after_soldier_spawns;
    Observable<Soldier&> soldier_died;
    Observable<const Soldier&, unsigned int> after_soldier_updates;
    Observable<const Item&, unsigned int> after_item_updates;
};
} // namespace Soldank
