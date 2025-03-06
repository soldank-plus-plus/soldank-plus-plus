#include "core/physics/ItemPhysics.hpp"

#include "core/entities/Item.hpp"
#include "core/entities/Soldier.hpp"
#include "core/map/Map.hpp"
#include "core/map/PMSEnums.hpp"
#include "core/math/Calc.hpp"
#include "core/state/StateManager.hpp"
#include "core/physics/PhysicsEvents.hpp"
#include "core/physics/Constants.hpp"
#include "core/types/BulletType.hpp"
#include "core/types/ItemType.hpp"

#include <cmath>

namespace Soldank::ItemPhysics
{
// TODO: move somewhere
const int SECOND = 60;
const int FLAG_TIMEOUT = SECOND * 25;
const int FLAG_HOLDING_FORCEUP = -14;
const float GRAV = 0.06;
const int BASE_RADIUS = 75;

void Update(StateManager& state_manager, Item& item, const PhysicsEvents& physics_events)
{
    bool was_static = item.static_type;

    if (!item.static_type) {
        bool collided = false;
        bool collided2 = false;

        // BGState.BackgroundTestPrepare();

        for (int i = 1; i <= (int)item.skeleton->GetParticles().size(); ++i) {
            if (item.skeleton->GetActive(i) &&
                (item.holding_soldier_id == 0 || (item.holding_soldier_id > 0 && i == 1))) {

                if (IsItemTypeFlag(item.style)) {
                    if (i == 1) {
                        if (CheckMapCollision(item,
                                              state_manager.GetConstMap(),
                                              item.skeleton->GetPos(i).x - 10,
                                              item.skeleton->GetPos(i).y - 8,
                                              i,
                                              physics_events) ||
                            CheckMapCollision(item,
                                              state_manager.GetConstMap(),
                                              item.skeleton->GetPos(i).x + 10,
                                              item.skeleton->GetPos(i).y - 8,
                                              i,
                                              physics_events) ||
                            CheckMapCollision(item,
                                              state_manager.GetConstMap(),
                                              item.skeleton->GetPos(i).x - 10,
                                              item.skeleton->GetPos(i).y,
                                              i,
                                              physics_events) ||
                            CheckMapCollision(item,
                                              state_manager.GetConstMap(),
                                              item.skeleton->GetPos(i).x + 10,
                                              item.skeleton->GetPos(i).y,
                                              i,
                                              physics_events)) {
                            if (collided) {
                                collided2 = true;
                            }

                            collided = true;
                            if (collided) {
                                auto force = item.skeleton->GetForce(2);
                                force.y +=
                                  PhysicsConstants::FLAG_STAND_FORCEUP * PhysicsConstants::GRAV;
                                item.skeleton->SetForce(2, force);
                            }
                        }
                    } else {
                        if (CheckMapCollision(item,
                                              state_manager.GetConstMap(),
                                              item.skeleton->GetPos(i).x,
                                              item.skeleton->GetPos(i).y,
                                              i,
                                              physics_events)) {
                            if (collided) {
                                collided2 = true;
                            }
                            collided = true;
                        }
                    }
                } else {
                    if (CheckMapCollision(item,
                                          state_manager.GetConstMap(),
                                          item.skeleton->GetPos(i).x,
                                          item.skeleton->GetPos(i).y,
                                          i,
                                          physics_events)) {
                        if (collided) {
                            collided2 = true;
                        }
                        collided = true;
                    }
                }
            }
        }

        // BGState.BackgroundTestReset();

        item.skeleton->DoVerletTimestep();

        if (item.style == ItemType::M2 && item.time_out < 0) {
            item.skeleton->SetPos(2, item.skeleton->GetOldPos(2));
            item.skeleton->SetPos(3, item.skeleton->GetOldPos(3));
        }

        auto a = item.skeleton->GetPos(1) - item.skeleton->GetOldPos(1);
        auto b = item.skeleton->GetPos(2) - item.skeleton->GetOldPos(2);
        if (item.style != ItemType::M2) {
            if (collided && collided2) {
                if ((Calc::Vec2Length(a) + Calc::Vec2Length(b)) / 2 <
                    PhysicsConstants::MINMOVEDELTA) {
                    item.static_type = true;
                }
            }
        }

        if (IsItemTypeFlag(item.style) && item.holding_soldier_id > 0) {
            state_manager.ForEachSoldier([&](const auto& soldier) {
                if (soldier.id == item.holding_soldier_id) {
                    item.skeleton->SetPos(1, soldier.skeleton->GetPos(8));
                    auto new_force = item.skeleton->GetForce(2);
                    new_force.y += (float)FLAG_HOLDING_FORCEUP * GRAV;
                    item.skeleton->SetForce(2, new_force);
                    item.time_out = FLAG_TIMEOUT;
                }
            });
        }
    }

    if (item.style == ItemType::AlphaFlag || item.style == ItemType::BravoFlag) {
        PMSSpawnPointType spawn_point_type = PMSSpawnPointType::AlphaFlag;
        if (item.style == ItemType::BravoFlag) {
            spawn_point_type = PMSSpawnPointType::BravoFlag;
        }

        auto spawn_point = state_manager.GetConstMap().FindFirstSpawnPoint(spawn_point_type);
        if (spawn_point && Calc::Distance(item.skeleton->GetPos(1),
                                          { spawn_point->x, spawn_point->y }) < BASE_RADIUS) {
            item.in_base = true;
            item.time_out = FLAG_TIMEOUT;
        } else {
            item.in_base = false;
        }
    }

    if (item.style != ItemType::M2) {
        int colliding_player_id = CheckSoldierCollision(item, state_manager);
        if (colliding_player_id >= 0) {
            state_manager.TransformSoldiers([&](auto& soldier) {
                if (colliding_player_id == soldier.id) {
                    physics_events.soldier_collides_with_item.Notify(soldier, item);
                }
            });
        }
    }

    // Parachute
    if (item.style == ItemType::Parachute) {
        if (item.holding_soldier_id != 0) {
            state_manager.TransformSoldiers([&](auto& soldier) {
                if (soldier.id == item.holding_soldier_id) {
                    item.skeleton->SetPos(4, soldier.skeleton->GetPos(12));
                    glm::vec2 force = soldier.particle.GetVelocity();
                    force.y = -force.y;
                    item.skeleton->SetForce(1, force);

                    if (item.skeleton->GetPos(3).x < item.skeleton->GetPos(4).x) {
                        glm::vec2 a = item.skeleton->GetPos(4);
                        item.skeleton->SetPos(4, item.skeleton->GetPos(3));
                        item.skeleton->SetOldPos(4, item.skeleton->GetPos(3));
                        item.skeleton->SetPos(3, a);
                        item.skeleton->SetOldPos(3, a);
                        force = soldier.particle.GetForce();
                        soldier.particle.SetForce({ force.x, GRAV });
                    }
                }
            });
        } else {
            if (item.time_out > 180) {
                item.time_out = 180;
            }
        }
    }

    --item.time_out;
    if (item.time_out < -1000) {
        item.time_out = -1000;
    }

    if (item.time_out == 0) {
        switch (item.style) {
            case ItemType::AlphaFlag:
            case ItemType::BravoFlag:
            case ItemType::PointmatchFlag:
            case ItemType::Bow:
                // begin
                // {$IFDEF SERVER}
                // if HoldingSprite > 0 then
                //     TimeOut := FLAG_TIMEOUT else
                //     Respawn;
                // {$ENDIF}
                // end;
                break;
            default:
                item.active = false;
                break;
        }
    }

    // CheckOutOfBounds;

    if ((!was_static) && item.static_type) {
        for (unsigned int i = 1; i <= item.skeleton->GetParticles().size(); ++i) {
            item.skeleton->SetOldPos(i, item.skeleton->GetPos(i));
        }
    }
}

bool CheckMapCollision(Item& item,
                       const Map& map,
                       float x,
                       float y,
                       int i,
                       const PhysicsEvents& physics_events)
{
    glm::vec2 pos = { x, y - 0.5F }; // TODO: this looks wrong
    auto rx = ((int)std::round((pos.x / (float)map.GetSectorsSize()))) + 25;
    auto ry = ((int)std::round((pos.y / (float)map.GetSectorsSize()))) + 25;
    if ((rx > 0) && (rx < map.GetSectorsCount() + 25) && (ry > 0) &&
        (ry < map.GetSectorsCount() + 25)) {
        // TODO
        //  BGState.BackgroundTestBigPolyCenter(Pos);

        for (unsigned int j = 0; j < map.GetSector(rx, ry).polygons.size(); j++) {
            auto w = map.GetSector(rx, ry).polygons[j] - 1;

            auto teamcol = true;

            // if (Owner > 0) and (Owner < MAX_SPRITES + 1) then
            // begin
            //     teamcol := TeamCollides(w, Sprite[Owner].Player.Team, False);
            // end;

            // if (Style < OBJECT_USSOCOM) and (Map.PolyType[w] > POLY_TYPE_LAVA) and
            // (Map.PolyType[w] < POLY_TYPE_BOUNCY) then
            //     teamcol := False;

            if (teamcol &&
                (map.GetPolygons().at(w).polygon_type != PMSPolygonType::OnlyBulletsCollide) &&
                (map.GetPolygons().at(w).polygon_type != PMSPolygonType::OnlyPlayersCollide) &&
                (map.GetPolygons().at(w).polygon_type != PMSPolygonType::NoCollide) &&
                (map.GetPolygons().at(w).polygon_type != PMSPolygonType::FlaggerCollides) &&
                (map.GetPolygons().at(w).polygon_type != PMSPolygonType::NonFlaggerCollides)) {
                if (map.PointInPolyEdges(pos.x, pos.y, w)) {
                    // if BGState.BackgroundTest(w) then
                    //     Continue;

                    float d = NAN;
                    int b = 0;
                    auto perp = map.ClosestPerpendicular(w, pos, &d, &b);

                    perp = Calc::Vec2Normalize(perp);
                    perp = Calc::Vec2Scale(perp, d);

                    switch (item.style) {
                        case ItemType::AlphaFlag:
                        case ItemType::BravoFlag:
                        case ItemType::PointmatchFlag: {
                            if (i == 1) {
                                item.skeleton->SetPos(i, item.skeleton->GetOldPos(i));
                            } else {
                                auto pos_diff =
                                  item.skeleton->GetPos(i) - item.skeleton->GetOldPos(i);
                                auto pos_diff_len = Calc::Vec2Length(pos_diff);
                                auto pos_diff_perp = Calc::Vec2Normalize(perp);
                                pos_diff_perp = Calc::Vec2Scale(pos_diff_perp, pos_diff_len);
                                item.skeleton->SetPos(i, item.skeleton->GetPos(i) - perp);
                                item.skeleton->SetOldPos(i,
                                                         item.skeleton->GetPos(i) + pos_diff_perp);

                                if (i == 2 && item.holding_soldier_id == 0) {
                                    auto new_force = item.skeleton->GetForce(i);
                                    new_force.y -= 1;
                                    item.skeleton->SetForce(i, new_force);
                                }
                            }
                            break;
                        }
                        case ItemType::USSOCOM:
                        case ItemType::DesertEagles:
                        case ItemType::MP5:
                        case ItemType::Ak74:
                        case ItemType::SteyrAUG:
                        case ItemType::Spas12:
                        case ItemType::Ruger77:
                        case ItemType::M79:
                        case ItemType::Barrett:
                        case ItemType::Minimi:
                        case ItemType::Minigun:
                        case ItemType::Knife:
                        case ItemType::Chainsaw:
                        case ItemType::LAW: {
                            item.skeleton->SetPos(i, item.skeleton->GetOldPos(i));
                            item.skeleton->SetPos(i, item.skeleton->GetPos(i) - perp);
                            break;
                        }
                        case ItemType::Bow:
                        case ItemType::MedicalKit:
                        case ItemType::GrenadeKit:
                        case ItemType::FlamerKit:
                        case ItemType::PredatorKit:
                        case ItemType::VestKit:
                        case ItemType::BerserkKit:
                        case ItemType::ClusterKit: {
                            item.skeleton->SetPos(i, item.skeleton->GetOldPos(i));
                            item.skeleton->SetPos(i, item.skeleton->GetPos(i) - perp);
                            break;
                        }
                        case ItemType::Parachute:
                        case ItemType::M2:
                            break;
                    }

                    ++item.collide_count.at(i - 1);

                    physics_events.item_collides_with_polygon.Notify(item, map.GetPolygons().at(w));
                    return true;
                }
            }
        }
    }
    return false;
}

const int STARTHEALTH = 150; // TODO: move me somewhere else
const int MAXGRENADES = 5;   // TODO: move me somewhere else

int CheckSoldierCollision(Item& item, const StateManager& state_manager)
{
    glm::vec2 a = item.skeleton->GetPos(1) - item.skeleton->GetPos(2);
    float k = Calc::Vec2Length(a) / 2;
    a = Calc::Vec2Normalize(a);
    a = Calc::Vec2Scale(a, -k);
    glm::vec2 pos = item.skeleton->GetPos(1) + a;

    float closestdist = 99999999.0;
    int closestplayer = -1;

    state_manager.ForEachSoldier([&](const auto& soldier) {
        if (soldier.active && !soldier.dead_meat /* && soldier_it->team != SPECTATOR TODO*/) {
            glm::vec2 col_pos = soldier.particle.position;
            glm::vec2 norm = pos - col_pos;
            if (Calc::Vec2Length(norm) >= item.radius) {
                pos = item.skeleton->GetPos(1);
                col_pos = soldier.particle.position;
                norm = pos - col_pos;

                if (Calc::Vec2Length(norm) >= item.radius) {
                    pos = item.skeleton->GetPos(2);
                    col_pos = soldier.particle.position;
                    norm = pos - col_pos;
                }
            }

            float dist = Calc::Vec2Length(norm);
            if (dist < item.radius) {
                if (dist < closestdist) {
                    // TODO: Don't pick flag if player just spawned
                    // cease_fire_counter is the invincibility time for the spawned
                    // player...
                    // if (!(IsItemTypeFlag(item.style) && soldier_it->cease_fire_counter >
                    // 0)) {
                    closestdist = dist;
                    closestplayer = soldier.id;
                    // }
                }
            }
        }
    });

    return closestplayer;
}

} // namespace Soldank::ItemPhysics
