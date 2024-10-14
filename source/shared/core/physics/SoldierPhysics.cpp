#include "SoldierPhysics.hpp"

#include "core/animations/AnimationData.hpp"
#include "core/animations/AnimationState.hpp"
#include "core/physics/SoldierPhysics.hpp"
#include "core/physics/SoldierSkeletonPhysics.hpp"

#include "core/math/Calc.hpp"
#include "core/entities/Bullet.hpp"
#include "core/types/TeamType.hpp"
#include "core/types/WeaponType.hpp"
#include "core/entities/WeaponParametersFactory.hpp"

#include "core/physics/Constants.hpp"

#include "spdlog/spdlog.h"

#include <cmath>
#include <utility>
#include <vector>
#include <algorithm>

namespace Soldank::SoldierPhysics
{
const Weapon& GetPrimaryWeapon(Soldier& soldier)
{
    return soldier.weapons[soldier.active_weapon];
}

const Weapon& GetSecondaryWeapon(Soldier& soldier)
{
    return soldier.weapons[(soldier.active_weapon + 1) % 2];
}

const Weapon& GetTertiaryWeapon(Soldier& soldier)
{
    return soldier.weapons[2];
}

void SwitchWeapon(Soldier& soldier)
{
    int new_active_weapon = (soldier.active_weapon + 1) % 2;
    soldier.active_weapon = new_active_weapon;
    // weapons[new_active_weapon].start_up_time_count =
    //   weapons[new_active_weapon].GetWeaponParameters().start_up_time;
    soldier.weapons[new_active_weapon].ResetStartUpTimeCount();
    // weapons[new_active_weapon].reload_time_prev = weapons[new_active_weapon].reload_time_count;
    soldier.weapons[new_active_weapon].SetReloadTimePrev(
      soldier.weapons[new_active_weapon].GetReloadTimeCount());
}

void UpdateKeys(Soldier& soldier, const Control& control)
{
    soldier.control = control;
}

void HandleSpecialPolytypes(const Map& map, PMSPolygonType polytype, Soldier& soldier)
{
    if (polytype == PMSPolygonType::Deadly || polytype == PMSPolygonType::BloodyDeadly ||
        polytype == PMSPolygonType::Explosive) {
        soldier.particle.position = glm::vec2(map.GetSpawnPoints()[0].x, map.GetSpawnPoints()[0].y);
    }
}

void Update(State& state,
            Soldier& soldier,
            const PhysicsEvents& physics_events,
            const AnimationDataManager& animation_data_manager,
            std::vector<BulletParams>& bullet_emitter)
{
    const Map& map = state.map;
    float body_y = 0.0f;
    float arm_s;

    soldier.particle.Euler();

    if (!soldier.control.fire) {
        soldier.is_shooting = false;
    }

    if (soldier.legs_animation->GetSpeed() < 1) {
        soldier.legs_animation->SetSpeed(1);
    }

    if (soldier.legs_animation->GetSpeed() < 1) {
        soldier.legs_animation->SetSpeed(1);
    }

    if (soldier.body_animation->GetSpeed() < 1) {
        soldier.body_animation->SetSpeed(1);
    }

    auto conflicting_keys_pressed = [](const Control& c) {
        return ((int)c.throw_grenade + (int)c.change + (int)c.drop + (int)c.reload) > 1;
    };

    // Handle simultaneous key presses that would conflict
    if (conflicting_keys_pressed(soldier.control)) {
        // At least two buttons pressed, so deactivate any previous one
        if (soldier.control.was_throwing_grenade) {
            soldier.control.throw_grenade = false;
        } else if (soldier.control.was_changing_weapon) {
            soldier.control.change = false;
        } else if (soldier.control.was_throwing_weapon) {
            soldier.control.drop = false;
        } else if (soldier.control.was_reloading_weapon) {
            soldier.control.reload = false;
        }

        // If simultaneously pressing two or more new buttons, then deactivate them
        // in order of least preference
        while (conflicting_keys_pressed(soldier.control)) {
            if (soldier.control.reload) {
                soldier.control.reload = false;
            } else if (soldier.control.change) {
                soldier.control.change = false;
            } else if (soldier.control.drop) {
                soldier.control.drop = false;
            } else if (soldier.control.throw_grenade) {
                soldier.control.throw_grenade = false;
            }
        }
    } else {
        soldier.control.was_throwing_grenade = soldier.control.throw_grenade;
        soldier.control.was_changing_weapon = soldier.control.change;
        soldier.control.was_throwing_weapon = soldier.control.drop;
        soldier.control.was_reloading_weapon = soldier.control.reload;
    }

    // self.fired = 0;
    soldier.control.mouse_aim_x =
      (int)((float)soldier.control.mouse_aim_x + soldier.particle.GetVelocity().x);
    soldier.control.mouse_aim_y =
      (int)((float)soldier.control.mouse_aim_y + soldier.particle.GetVelocity().y);

    auto maybe_new_legs_animation_state_machine = soldier.legs_animation->HandleInput(soldier);
    if (maybe_new_legs_animation_state_machine.has_value()) {
        soldier.legs_animation->Exit(soldier, physics_events);
        soldier.legs_animation = *maybe_new_legs_animation_state_machine;
        soldier.legs_animation->Enter(soldier);
    }

    soldier.body_animation->TryToShoot(soldier, physics_events);
    soldier.body_animation->TryToThrowFlags(soldier, physics_events);
    auto maybe_new_body_animation_state_machine = soldier.body_animation->HandleInput(soldier);
    if (maybe_new_body_animation_state_machine.has_value()) {
        soldier.body_animation->Exit(soldier, physics_events);
        soldier.body_animation = *maybe_new_body_animation_state_machine;
        soldier.body_animation->Enter(soldier);
    }

    soldier.legs_animation->Update(soldier, physics_events);
    soldier.body_animation->Update(soldier, physics_events);

    bool jets_can_be_applied = true;
    if (soldier.legs_animation->GetType() == AnimationType::RollBack && soldier.control.up) {
        // jets cannot be applied during RollBack animation when "up" is pressed because
        // that animation adds its forces on the soldier
        jets_can_be_applied = false;
    }

    if (jets_can_be_applied && soldier.control.jets && (soldier.jets_count > 0)) {
        if (soldier.on_ground) {
            glm::vec2 particle_force = soldier.particle.GetForce();
            if (state.gravity > 0.05F) {
                soldier.particle.SetForce({ particle_force.x, -2.5F * PhysicsConstants::JETSPEED });
            } else {
                soldier.particle.SetForce({ particle_force.x, -2.5F * (state.gravity * 2.0F) });
            }
        } else if (soldier.stance != PhysicsConstants::STANCE_PRONE) {
            glm::vec2 particle_force = soldier.particle.GetForce();
            if (state.gravity > 0.05F) {
                soldier.particle.SetForce(
                  { particle_force.x, particle_force.y - PhysicsConstants::JETSPEED });
            } else {
                soldier.particle.SetForce(
                  { particle_force.x, particle_force.y - state.gravity * 2.0F });
            }
        } else {
            glm::vec2 particle_force = soldier.particle.GetForce();
            soldier.particle.SetForce(
              { particle_force.x +
                  (float)soldier.direction *
                    (state.gravity > 0.05F ? PhysicsConstants::JETSPEED / 2.0F : state.gravity),
                particle_force.y });
        }
        soldier.jets_count -= 1;
    }

    RepositionSoldierSkeletonParts(soldier);

    for (int i = 1; i <= 20; i++) {
        if ((soldier.dead_meat || soldier.half_dead) && (i < 17) && (i != 7) && (i != 8)) {
            auto xy = soldier.particle.position;
            soldier.on_ground = CheckSkeletonMapCollision(soldier, map, i, xy.x, xy.y, state);
        }
    }

    if (!soldier.dead_meat) {
        soldier.body_animation->DoAnimation();
        soldier.legs_animation->DoAnimation();

        soldier.on_ground = false;

        auto xy = soldier.particle.position;

        CheckMapCollision(soldier, map, xy.x - 3.5, xy.y - 12.0, 1, state, physics_events);

        xy = soldier.particle.position;

        CheckMapCollision(soldier, map, xy.x + 3.5, xy.y - 12.0, 1, state, physics_events);

        body_y = 0.0;
        arm_s = 0.0;

        // Walking either left or right (though only one can be active at once)
        if (soldier.control.left ^ soldier.control.right) {
            if (soldier.control.left ^ (soldier.direction == 1)) {
                // WRONG
                arm_s = 0.25;
            } else {
                body_y = 0.25;
            }
        }
        // If a leg is inside a polygon, caused by the modification of ArmS and
        // BodyY, this is there to not lose contact to ground on slope polygons
        if (body_y == 0.0) {
            // let leg_vector = vec2(
            //   self.particle.pos.x + 2.0,
            //   self.particle.pos.y + 1.9,
            //);
            //     if Map.RayCast(LegVector, LegVector, LegDistance, 10) {
            body_y = 0.25;
            // }
        }
        if (arm_s == 0.0) {
            // let leg_vector = vec2(
            //   self.particle.pos.x - 2.0,
            //   self.particle.pos.y + 1.9,
            //);
            //     if Map.RayCast(LegVector, LegVector, LegDistance, 10) {
            arm_s = 0.25;
            // }
        }

        xy = soldier.particle.position;
        soldier.on_ground = CheckMapCollision(
          soldier, map, xy.x + 2.0, xy.y + 2.0 - body_y, 0, state, physics_events);

        xy = soldier.particle.position;
        soldier.on_ground =
          soldier.on_ground ||
          CheckMapCollision(soldier, map, xy.x - 2.0, xy.y + 2.0 - arm_s, 0, state, physics_events);

        xy = soldier.particle.position;
        auto grounded = soldier.on_ground;
        soldier.on_ground_for_law =
          CheckRadiusMapCollision(soldier, map, xy.x, xy.y - 1.0, grounded, state, physics_events);

        xy = soldier.particle.position;
        grounded = soldier.on_ground || soldier.on_ground_for_law;
        soldier.on_ground =
          soldier.on_ground ||
          CheckMapVerticesCollision(soldier, map, xy.x, xy.y, 3.0, grounded, state, physics_events);

        if (!(soldier.on_ground ^ soldier.on_ground_last_frame)) {
            soldier.on_ground_permanent = soldier.on_ground;
        }

        soldier.on_ground_last_frame = soldier.on_ground;

        if ((soldier.jets_count < map.GetJetCount()) && !(soldier.control.jets)) {
            // if self.on_ground
            /* (MainTickCounter mod 2 = 0) */
            {
                soldier.jets_count += 1;
            }
        }
        soldier.alpha = 255;

        for (auto& item : state.items) {
            if (item.holding_soldier_id == soldier.id && item.style == ItemType::Parachute) {
                glm::vec2 force = soldier.particle.GetForce();
                soldier.particle.SetForce(
                  { force.x, PhysicsConstants::SOLDIER_SPEED_WITH_PARACHUTE });
                // {$IFDEF SERVER}
                // if CeaseFireCounter < 1 then
                // {$ELSE}
                // if ((sv_survivalmode.Value) and (CeaseFireCounter < CeaseFireTime * 3 - 30)) or
                //    (CeaseFireCounter < CeaseFireTime - 30) then
                // {$ENDIF}
                if (soldier.on_ground || soldier.control.jets) {
                    item.holding_soldier_id = 0;
                    // TODO
                    // Dec(Thing[HoldedThing].Skeleton.ConstraintCount);
                    item.time_out = 3 * 60;
                    // HoldedThing := 0;
                }
            }
        }

        soldier.skeleton->DoVerletTimestepFor(22, 29);
        soldier.skeleton->DoVerletTimestepFor(24, 30);
    }

    if (soldier.dead_meat) {
        soldier.skeleton->DoVerletTimestep();
        soldier.particle.position = soldier.skeleton->GetPos(12);
        // CheckSkeletonOutOfBounds;

        for (auto& item : state.items) {
            if (item.holding_soldier_id == soldier.id && item.style == ItemType::Parachute) {
                glm::vec2 force = soldier.skeleton->GetForce(12);
                soldier.skeleton->SetForce(
                  12, { force.x, 25 * PhysicsConstants::SOLDIER_SPEED_WITH_PARACHUTE });
                if (soldier.on_ground) {
                    item.holding_soldier_id = 0;
                    // TODO
                    // Dec(Thing[HoldedThing].Skeleton.ConstraintCount);
                    item.time_out = 3 * 60;
                }
            }
        }
    }

    if (soldier.particle.velocity_.x > PhysicsConstants::MAX_VELOCITY) {
        soldier.particle.velocity_.x = PhysicsConstants::MAX_VELOCITY;
    }
    if (soldier.particle.velocity_.x < -PhysicsConstants::MAX_VELOCITY) {
        soldier.particle.velocity_.x = -PhysicsConstants::MAX_VELOCITY;
    }
    if (soldier.particle.velocity_.y > PhysicsConstants::MAX_VELOCITY) {
        soldier.particle.velocity_.y = PhysicsConstants::MAX_VELOCITY;
    }
    if (soldier.particle.velocity_.y < -PhysicsConstants::MAX_VELOCITY) {
        soldier.particle.velocity_.y = PhysicsConstants::MAX_VELOCITY;
    }
}

bool CheckMapCollision(Soldier& soldier,
                       const Map& map,
                       float x,
                       float y,
                       int area,
                       State& state,
                       const PhysicsEvents& physics_events)
{
    auto pos = glm::vec2(x, y) + soldier.particle.velocity_;
    auto rx = ((int)std::round((pos.x / (float)map.GetSectorsSize()))) + 25;
    auto ry = ((int)std::round((pos.y / (float)map.GetSectorsSize()))) + 25;

    if ((rx > 0) && (rx < map.GetSectorsCount() + 25) && (ry > 0) &&
        (ry < map.GetSectorsCount() + 25)) {
        for (int j = 0; j < map.GetSector(rx, ry).polygons.size(); j++) {
            auto poly = map.GetSector(rx, ry).polygons[j] - 1;
            auto polytype = map.GetPolygons()[poly].polygon_type;

            if ((polytype != PMSPolygonType::NoCollide) &&
                (polytype != PMSPolygonType::OnlyBulletsCollide)) {
                auto polygons = map.GetPolygons()[poly];

                if (map.PointInPoly(pos, polygons)) {
                    HandleSpecialPolytypes(map, polytype, soldier);

                    auto dist = 0.0f;
                    auto k = 0;

                    auto perp = map.ClosestPerpendicular(poly, pos, &dist, &k);

                    auto step = perp;

                    perp = Calc::Vec2Normalize(perp);
                    perp *= dist;
                    dist = Calc::Vec2Length(soldier.particle.velocity_);

                    if (Calc::Vec2Length(perp) > dist) {
                        perp = Calc::Vec2Normalize(perp);
                        perp *= dist;
                    }
                    if ((area == 0) ||
                        ((area == 1) &&
                         ((soldier.particle.velocity_.y < 0.0) ||
                          (soldier.particle.velocity_.x > PhysicsConstants::SLIDELIMIT) ||
                          (soldier.particle.velocity_.x < -PhysicsConstants::SLIDELIMIT)))) {
                        soldier.particle.old_position = soldier.particle.position;
                        soldier.particle.position -= perp;
                        if (map.GetPolygons()[poly].polygon_type == PMSPolygonType::Bouncy) {
                            perp = Calc::Vec2Normalize(perp);
                            perp *= map.GetPolygons()[poly].bounciness * dist;
                        }
                        soldier.particle.velocity_ -= perp;
                    }

                    if (area == 0) {
                        if ((soldier.legs_animation->GetType() == AnimationType::Stand) ||
                            (soldier.legs_animation->GetType() == AnimationType::Crouch) ||
                            (soldier.legs_animation->GetType() == AnimationType::Prone) ||
                            (soldier.legs_animation->GetType() == AnimationType::ProneMove) ||
                            (soldier.legs_animation->GetType() == AnimationType::GetUp) ||
                            (soldier.legs_animation->GetType() == AnimationType::Fall) ||
                            (soldier.legs_animation->GetType() == AnimationType::Mercy) ||
                            (soldier.legs_animation->GetType() == AnimationType::Mercy2) ||
                            (soldier.legs_animation->GetType() == AnimationType::Own)) {
                            if ((soldier.particle.velocity_.x < PhysicsConstants::SLIDELIMIT) &&
                                (soldier.particle.velocity_.x > -PhysicsConstants::SLIDELIMIT) &&
                                (step.y > PhysicsConstants::SLIDELIMIT)) {
                                soldier.particle.position = soldier.particle.old_position;
                                glm::vec2 particle_force = soldier.particle.GetForce();
                                particle_force.y -= PhysicsConstants::GRAV;
                                soldier.particle.SetForce(particle_force);
                            }

                            if ((step.y > PhysicsConstants::SLIDELIMIT) &&
                                (polytype != PMSPolygonType::Ice) &&
                                (polytype != PMSPolygonType::Bouncy)) {
                                if ((soldier.legs_animation->GetType() == AnimationType::Stand) ||
                                    (soldier.legs_animation->GetType() == AnimationType::Fall) ||
                                    (soldier.legs_animation->GetType() == AnimationType::Crouch)) {
                                    soldier.particle.velocity_.x *=
                                      PhysicsConstants::STANDSURFACECOEFX;
                                    soldier.particle.velocity_.y *=
                                      PhysicsConstants::STANDSURFACECOEFY;

                                    glm::vec2 particle_force = soldier.particle.GetForce();
                                    particle_force.x -= soldier.particle.velocity_.x;
                                    soldier.particle.SetForce(particle_force);
                                } else if (soldier.legs_animation->GetType() ==
                                           AnimationType::Prone) {
                                    if (soldier.legs_animation->GetFrame() > 24) {
                                        if (!(soldier.control.down &&
                                              (soldier.control.left || soldier.control.right))) {
                                            soldier.particle.velocity_.x *=
                                              PhysicsConstants::STANDSURFACECOEFX;
                                            soldier.particle.velocity_.y *=
                                              PhysicsConstants::STANDSURFACECOEFY;

                                            glm::vec2 particle_force = soldier.particle.GetForce();
                                            particle_force.x -= soldier.particle.velocity_.x;
                                            soldier.particle.SetForce(particle_force);
                                        }
                                    } else {
                                        soldier.particle.velocity_.x *=
                                          PhysicsConstants::SURFACECOEFX;
                                        soldier.particle.velocity_.y *=
                                          PhysicsConstants::SURFACECOEFY;
                                    }
                                } else if (soldier.legs_animation->GetType() ==
                                           AnimationType::GetUp) {
                                    soldier.particle.velocity_.x *= PhysicsConstants::SURFACECOEFX;
                                    soldier.particle.velocity_.y *= PhysicsConstants::SURFACECOEFY;
                                } else if (soldier.legs_animation->GetType() ==
                                           AnimationType::ProneMove) {
                                    soldier.particle.velocity_.x *=
                                      PhysicsConstants::STANDSURFACECOEFX;
                                    soldier.particle.velocity_.y *=
                                      PhysicsConstants::STANDSURFACECOEFY;
                                }
                            }
                        } else if ((soldier.legs_animation->GetType() ==
                                    AnimationType::CrouchRun) ||
                                   (soldier.legs_animation->GetType() ==
                                    AnimationType::CrouchRunBack)) {
                            soldier.particle.velocity_.x *=
                              PhysicsConstants::CROUCHMOVESURFACECOEFX;
                            soldier.particle.velocity_.y *=
                              PhysicsConstants::CROUCHMOVESURFACECOEFY;
                        } else {
                            soldier.particle.velocity_.x *= PhysicsConstants::SURFACECOEFX;
                            soldier.particle.velocity_.y *= PhysicsConstants::SURFACECOEFY;
                        }
                    }

                    physics_events.soldier_collides_with_polygon.Notify(soldier, polygons);
                    return true;
                }
            }
        }
    }

    return false;
}

bool CheckMapVerticesCollision(Soldier& soldier,
                               const Map& map,
                               float x,
                               float y,
                               float r,
                               bool has_collided,
                               State& state,
                               const PhysicsEvents& physics_events)
{
    auto pos = glm::vec2(x, y) + soldier.particle.velocity_;
    auto rx = ((int)std::round(pos.x / (float)map.GetSectorsSize())) + 25;
    auto ry = ((int)std::round(pos.y / (float)map.GetSectorsSize())) + 25;

    if ((rx > 0) && (rx < map.GetSectorsCount() + 25) && (ry > 0) &&
        (ry < map.GetSectorsCount() + 25)) {
        for (int j = 0; j < map.GetSector(rx, ry).polygons.size(); j++) {
            auto poly = map.GetSector(rx, ry).polygons[j] - 1;
            PMSPolygonType polytype = map.GetPolygons()[poly].polygon_type;

            if (polytype != PMSPolygonType::NoCollide &&
                polytype != PMSPolygonType::OnlyBulletsCollide) {
                for (int i = 0; i < 3; i++) {
                    auto vert = glm::vec2(map.GetPolygons()[poly].vertices[i].x,
                                          map.GetPolygons()[poly].vertices[i].y);

                    auto dist = Calc::Distance(vert, pos);
                    if (dist < r) {
                        if (!has_collided) {
                            HandleSpecialPolytypes(map, polytype, soldier);
                        }
                        auto dir = pos - vert;
                        dir = Calc::Vec2Normalize(dir);
                        soldier.particle.position += dir;
                        physics_events.soldier_collides_with_polygon.Notify(
                          soldier, map.GetPolygons()[poly]);

                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool CheckRadiusMapCollision(Soldier& soldier,
                             const Map& map,
                             float x,
                             float y,
                             bool has_collided,
                             State& state,
                             const PhysicsEvents& physics_events)
{
    auto s_pos = glm::vec2(x, y - 3.0f);

    auto det_acc = (int)Calc::Vec2Length(soldier.particle.velocity_);
    if (det_acc == 0) {
        det_acc = 1;
    }

    auto step = soldier.particle.velocity_ * (1.0f / (float)det_acc);

    for (int _z = 0; _z < det_acc; _z++) {
        s_pos += step;

        auto rx = ((int)std::round(s_pos.x / (float)map.GetSectorsSize())) + 25;
        auto ry = ((int)std::round(s_pos.y / (float)map.GetSectorsSize())) + 25;

        if ((rx > 0) && (rx < map.GetSectorsCount() + 25) && (ry > 0) &&
            (ry < map.GetSectorsCount() + 25)) {
            for (int j = 0; j < map.GetSector(rx, ry).polygons.size(); j++) {
                auto poly = map.GetSector(rx, ry).polygons[j] - 1;
                auto polytype = map.GetPolygons()[poly].polygon_type;

                if (polytype != PMSPolygonType::NoCollide &&
                    polytype != PMSPolygonType::OnlyBulletsCollide) {
                    for (int k = 0; k < 2; k++) { // TODO: czy tu powinno być k < 3?
                        auto norm = glm::vec2(map.GetPolygons()[poly].perpendiculars[k].x,
                                              map.GetPolygons()[poly].perpendiculars[k].y);
                        norm *= -PhysicsConstants::SOLDIER_COL_RADIUS;

                        auto pos = s_pos + norm;

                        if (map.PointInPolyEdges(pos.x, pos.y, poly)) {
                            if (!has_collided) {
                                HandleSpecialPolytypes(map, polytype, soldier);
                            }
                            auto d = 0.0f;
                            auto b = 0;
                            auto perp = map.ClosestPerpendicular(poly, pos, &d, &b);

                            auto p1 = glm::vec2(0.0, 0.0);
                            auto p2 = glm::vec2(0.0, 0.0);
                            switch (b) {
                                case 1: {
                                    p1 = glm::vec2(map.GetPolygons()[poly].vertices[0].x,
                                                   map.GetPolygons()[poly].vertices[0].y);
                                    p2 = glm::vec2(map.GetPolygons()[poly].vertices[1].x,
                                                   map.GetPolygons()[poly].vertices[1].y);
                                    break;
                                }
                                case 2: {
                                    p1 = glm::vec2(map.GetPolygons()[poly].vertices[1].x,
                                                   map.GetPolygons()[poly].vertices[1].y);
                                    p2 = glm::vec2(map.GetPolygons()[poly].vertices[2].x,
                                                   map.GetPolygons()[poly].vertices[2].y);
                                    break;
                                }
                                case 3: {
                                    p1 = glm::vec2(map.GetPolygons()[poly].vertices[2].x,
                                                   map.GetPolygons()[poly].vertices[2].y);
                                    p2 = glm::vec2(map.GetPolygons()[poly].vertices[0].x,
                                                   map.GetPolygons()[poly].vertices[0].y);
                                    break;
                                }
                            }

                            auto p3 = pos;
                            d = Calc::PointLineDistance(p1, p2, p3);
                            perp *= d;

                            soldier.particle.position = soldier.particle.old_position;
                            soldier.particle.velocity_ = soldier.particle.GetForce() - perp;
                            physics_events.soldier_collides_with_polygon.Notify(
                              soldier, map.GetPolygons()[poly]);

                            return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}

bool CheckSkeletonMapCollision(Soldier& soldier,
                               const Map& map,
                               unsigned int i,
                               float x,
                               float y,
                               State& state)
{
    auto result = false;
    auto pos = glm::vec2(x - 1.0f, y + 4.0f);
    auto rx = ((int)std::round(pos.x / (float)map.GetSectorsSize())) + 25;
    auto ry = ((int)std::round(pos.y / (float)map.GetSectorsSize())) + 25;

    if ((rx > 0) && (rx < map.GetSectorsCount() + 25) && (ry > 0) &&
        (ry < map.GetSectorsCount() + 25)) {
        for (int j = 0; j < map.GetSector(rx, ry).polygons.size(); j++) {
            auto poly = map.GetSector(rx, ry).polygons[j] - 1;

            if (map.PointInPolyEdges(pos.x, pos.y, poly)) {
                auto dist = 0.0f;
                auto b = 0;
                auto perp = map.ClosestPerpendicular(poly, pos, &dist, &b);
                perp = Calc::Vec2Normalize(perp);
                perp *= dist;

                soldier.skeleton->SetPos(i, soldier.skeleton->GetOldPos(i) - perp);

                result = true;
            }
        }
    }

    if (result) {
        auto pos = glm::vec2(x, y + 1.0);
        auto rx = ((int)std::round(pos.x / (float)map.GetSectorsSize())) + 25;
        auto ry = ((int)std::round(pos.y / (float)map.GetSectorsSize())) + 25;

        if ((rx > 0) && (rx < map.GetSectorsCount() + 25) && (ry > 0) &&
            (ry < map.GetSectorsCount() + 25)) {
            for (int j = 0; j < map.GetSector(rx, ry).polygons.size(); j++) {
                auto poly = map.GetSector(rx, ry).polygons[j] - 1;
                // if (Map.PolyType[poly] <> POLY_TYPE_DOESNT) and (Map.PolyType[poly] <>
                // POLY_TYPE_ONLY_BULLETS) then
                if (map.PointInPolyEdges(pos.x, pos.y, poly)) {
                    auto dist = 0.0F;
                    auto b = 0;
                    auto perp = map.ClosestPerpendicular(poly, pos, &dist, &b);
                    perp = Calc::Vec2Normalize(perp);
                    perp *= dist;

                    soldier.skeleton->SetPos(i, soldier.skeleton->GetOldPos(i) - perp);

                    result = true;
                }
            }
        }
    }

    return result;
}

void Fire(Soldier& soldier, std::vector<BulletParams>& bullet_emitter)
{
    auto weapon = GetPrimaryWeapon(soldier);

    glm::vec2 dir;
    if (weapon.GetWeaponParameters().bullet_style == BulletType::Blade ||
        soldier.body_animation->GetType() == AnimationType::Mercy ||
        soldier.body_animation->GetType() == AnimationType::Mercy2) {
        dir = Calc::Vec2Normalize(soldier.skeleton->GetPos(15) - soldier.skeleton->GetPos(16));
    } else {
        auto aim_x = (float)soldier.control.mouse_aim_x;
        auto aim_y = (float)soldier.control.mouse_aim_y;
        dir = Calc::Vec2Normalize(glm::vec2(aim_x, aim_y) - soldier.skeleton->GetPos(15));
    };

    auto pos = soldier.skeleton->GetPos(15) + dir * 4.0F - glm::vec2(0.0, 2.0);
    auto bullet_velocity = dir * weapon.GetWeaponParameters().speed;
    auto inherited_velocity =
      soldier.particle.velocity_ * weapon.GetWeaponParameters().inherited_velocity;

    BulletParams params{
        weapon.GetWeaponParameters().bullet_style,
        weapon.GetWeaponParameters().kind,
        pos,
        bullet_velocity + inherited_velocity,
        (std::int16_t)weapon.GetWeaponParameters().timeout,
        weapon.GetWeaponParameters().hit_multiply,
        TeamType::None,
        soldier.id,
    };

    switch (weapon.GetWeaponParameters().kind) {
        case WeaponType::DesertEagles: {
            if (!soldier.is_shooting) {
                bullet_emitter.push_back(params);

                auto signx = dir.x > 0.0F ? 1.0F : (dir.x < 0.0F ? -1.0F : 0.0F);
                auto signy = dir.x > 0.0F ? 1.0F : (dir.x < 0.0F ? -1.0F : 0.0F);

                params.position += glm::vec2(-signx * dir.y, signy * dir.x) * 3.0F;
                bullet_emitter.push_back(params);
            }
            break;
        }
        case WeaponType::Spas12:
        case WeaponType::Flamer:
        case WeaponType::NoWeapon:
        case WeaponType::Knife:
        case WeaponType::Chainsaw:
            break;
        case WeaponType::LAW: {
            if ((soldier.on_ground || soldier.on_ground_permanent || soldier.on_ground_for_law) &&
                (((soldier.legs_animation->GetType() == AnimationType::Crouch &&
                   soldier.legs_animation->GetFrame() > 13) ||
                  soldier.legs_animation->GetType() == AnimationType::CrouchRun ||
                  soldier.legs_animation->GetType() == AnimationType::CrouchRunBack) ||
                 (soldier.legs_animation->GetType() == AnimationType::Prone &&
                  soldier.legs_animation->GetFrame() > 23))) {
                bullet_emitter.push_back(params);
            }
            break;
        }
        default: {
            bullet_emitter.push_back(params);
        }
    };

    soldier.is_shooting = true;
}
} // namespace Soldank::SoldierPhysics
