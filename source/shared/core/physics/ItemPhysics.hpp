#ifndef __ITEM_PHYSICS_HPP__
#define __ITEM_PHYSICS_HPP__

#include <vector>

namespace Soldank
{
struct Item;
class Map;
class StateManager;
struct PhysicsEvents;
} // namespace Soldank

namespace Soldank::ItemPhysics
{
void Update(StateManager& state_manager, Item& item, const PhysicsEvents& physics_events);
bool CheckMapCollision(Item& item,
                       const Map& map,
                       float x,
                       float y,
                       int i,
                       const PhysicsEvents& physics_events);
int CheckSoldierCollision(Item& item, const StateManager& state_manager);
} // namespace Soldank::ItemPhysics

#endif
