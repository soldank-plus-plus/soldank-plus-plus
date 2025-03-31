#ifndef __SOLDIER_PHYSICS_HPP__
#define __SOLDIER_PHYSICS_HPP__

#include "core/animations/AnimationData.hpp"
#include "core/math/Glm.hpp"
#include "core/physics/Particles.hpp"
#include "core/physics/PhysicsEvents.hpp"
#include "core/state/Control.hpp"
#include "core/state/StateManager.hpp"
#include "core/entities/Weapon.hpp"
#include "core/entities/Bullet.hpp"
#include "core/entities/Soldier.hpp"
#include "core/map/Map.hpp"

#include <cstdint>
#include <vector>
#include <memory>

namespace Soldank::SoldierPhysics
{
const Weapon& GetPrimaryWeapon(Soldier& soldier);
const Weapon& GetSecondaryWeapon(Soldier& soldier);
const Weapon& GetTertiaryWeapon(Soldier& soldier);

void SwitchWeapon(Soldier& soldier);
void UpdateKeys(Soldier& soldier, const Control& control);
void HandleSpecialPolytypes(const Map& map, PMSPolygonType polytype, Soldier& soldier);
void Update(StateManager& state_manager,
            Soldier& soldier,
            const PhysicsEvents& physics_events,
            const AnimationDataManager& animation_data_manager,
            std::vector<BulletParams>& bullet_emitter,
            float gravity,
            bool can_soldier_shoot = false);

bool CheckMapCollision(Soldier& soldier,
                       const Map& map,
                       float x,
                       float y,
                       int area,
                       const PhysicsEvents& physics_events);
bool CheckMapVerticesCollision(Soldier& soldier,
                               const Map& map,
                               float x,
                               float y,
                               float r,
                               bool has_collided,
                               const PhysicsEvents& physics_events);
bool CheckRadiusMapCollision(Soldier& soldier,
                             const Map& map,
                             float x,
                             float y,
                             bool has_collided,
                             const PhysicsEvents& physics_events);
bool CheckSkeletonMapCollision(Soldier& soldier, const Map& map, unsigned int i, float x, float y);
void Fire(Soldier& soldier, std::vector<BulletParams>& bullet_emitter);
} // namespace Soldank::SoldierPhysics

#endif
