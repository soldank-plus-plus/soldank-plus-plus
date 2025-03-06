#ifndef __BULLET_PHYSICS_HPP__
#define __BULLET_PHYSICS_HPP__

#include "core/entities/Bullet.hpp"
#include "core/map/Map.hpp"
#include "core/state/StateManager.hpp"
#include "core/physics/PhysicsEvents.hpp"

#include <optional>

namespace Soldank::BulletPhysics
{
void UpdateBullet(const PhysicsEvents& physics_events,
                  Bullet& bullet,
                  const Map& map,
                  StateManager& state_manager);
} // namespace Soldank::BulletPhysics

#endif
