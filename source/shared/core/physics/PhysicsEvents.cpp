module;

#include "core/math/Glm.hpp"

export module Shared.Core.Physics.PhysicsEvents;

import Shared.Core.Map.PMSStructs;
import Shared.Core.Utility.Observable;
import Shared.Core.Entities.Soldier;
import Shared.Core.Entities.Item;
import Shared.Core.Entities.Bullet;

export namespace Soldank
{
struct PhysicsEvents
{
    Observable<Soldier&, float /* damage */> soldier_hit_by_bullet;
    Observable<Soldier&> soldier_switches_weapon;
    Observable<const Soldier&> soldier_throws_active_weapon;
    Observable<Soldier&> soldier_fires_primary_weapon;
    Observable<const Soldier&, const PMSPolygon&> soldier_collides_with_polygon;
    Observable<Soldier&, Item&> soldier_collides_with_item;
    Observable<Soldier&> soldier_throws_flags;

    Observable<Item&, const PMSPolygon&> item_collides_with_polygon;

    Observable<const Bullet&, const glm::vec2&> bullet_collides_with_polygon;
};
} // namespace Soldank
