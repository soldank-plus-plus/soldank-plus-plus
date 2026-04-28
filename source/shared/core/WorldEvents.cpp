export module Shared.Core.WorldEvents;

import Shared.Core.Utility.Observable;
import Shared.Core.Entities.Soldier;

export namespace Soldank
{
struct WorldEvents
{
    Observable<const Soldier&> after_soldier_spawns;
    Observable<Soldier&> soldier_died;
};
} // namespace Soldank
