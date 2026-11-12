module;

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>

export module Shared.Core.Physics.Bullets.BulletCollision;

import Extern.Glm;

import Shared.Core.Entities.Bullet;
import Shared.Core.Physics.Bullets.BulletLagCompensation;
import Shared.Core.Entities.Item;
import Shared.Core.Entities.Soldier;
import Shared.Core.Entities.WeaponParameters;
import Shared.Core.Entities.WeaponParametersFactory;
import Shared.Core.Types.TeamType;
import Shared.Core.Map.Map;
import Shared.Core.Map.PMSStructs;
import Shared.Core.Math.Calc;
import Shared.Core.Physics.Bullets.BulletTypes;
import Shared.Core.State.StateManager;
import Shared.Core.Types.BulletType;
import Shared.Core.Types.ItemType;

namespace Soldank
{
namespace
{
glm::vec2 GetMapImpactNormal(const PMSPolygon& polygon,
                             const glm::vec2& position,
                             const glm::vec2& travel_direction)
{
    float closest_distance_squared = std::numeric_limits<float>::max();
    glm::vec2 normal{};

    for (std::size_t edge = 0; edge < polygon.vertices.size(); ++edge) {
        const glm::vec2 edge_start{ polygon.vertices.at(edge).x, polygon.vertices.at(edge).y };
        const glm::vec2 edge_end{ polygon.vertices.at((edge + 1) % polygon.vertices.size()).x,
                                  polygon.vertices.at((edge + 1) % polygon.vertices.size()).y };
        const glm::vec2 edge_vector = edge_end - edge_start;
        const float edge_length_squared = glm::dot(edge_vector, edge_vector);
        if (edge_length_squared == 0.0F) {
            continue;
        }

        const float projection = std::clamp(
          glm::dot(position - edge_start, edge_vector) / edge_length_squared, 0.0F, 1.0F);
        const glm::vec2 closest_point = edge_start + edge_vector * projection;
        const float distance_squared = glm::dot(position - closest_point, position - closest_point);
        if (distance_squared >= closest_distance_squared) {
            continue;
        }

        closest_distance_squared = distance_squared;
        normal = { polygon.perpendiculars.at(edge).x, polygon.perpendiculars.at(edge).y };
    }

    if (glm::dot(normal, travel_direction) > 0.0F) {
        normal = -normal;
    }

    const float normal_length = glm::length(normal);
    return normal_length == 0.0F ? glm::vec2{} : normal / normal_length;
}

std::optional<BulletCollisionResult> FindSoldierCollisionPoint(const Soldier& soldier,
                                                               const Bullet& bullet,
                                                               unsigned int tick)
{
    constexpr auto BODY_PARTS_PRIORITY = std::array{ 12, 11, 10, 6, 5, 4, 3 };
    constexpr int PART_RADIUS = 7;

    const int radius = bullet.style == BulletType::FragGrenade ? PART_RADIUS + 1 : PART_RADIUS;
    const glm::vec2 collision_origin =
      BulletLagCompensation::GetCollisionOrigin(soldier, bullet, tick);
    const glm::vec2 start_point = bullet.particle.old_position;
    const glm::vec2 end_point = bullet.particle.position;
    float min_distance = std::numeric_limits<float>::max();
    std::optional<BulletCollisionResult> closest_collision;

    for (const int body_part_id : BODY_PARTS_PRIORITY) {
        glm::vec2 body_part_offset =
          soldier.skeleton->GetPos(body_part_id) - soldier.particle.position;
        glm::vec2 collision_position = collision_origin + body_part_offset;

        const auto hit_position =
          Calc::LineCircleCollision(start_point, end_point, collision_position, radius);
        if (!hit_position.has_value()) {
            continue;
        }

        const float distance = Calc::Vec2Length(*hit_position - bullet.particle.old_position);
        if (distance < min_distance) {
            min_distance = distance;
            closest_collision = BulletCollisionResult{
                .kind = BulletCollisionKind::Soldier,
                .position = *hit_position,
                .distance = distance,
                .soldier_id = soldier.id,
                .body_part_id = body_part_id,
            };
        }
    }

    return closest_collision;
}
} // namespace
} // namespace Soldank

export namespace Soldank::BulletCollision
{
std::optional<BulletCollisionResult> FindMapCollision(const Bullet& bullet, const Map& map)
{
    const glm::vec2 start_point = bullet.particle.old_position;
    const glm::vec2 end_point = bullet.particle.position;
    const auto travel = end_point - start_point;
    const auto cross = [](glm::vec2 left, glm::vec2 right) {
        return left.x * right.y - left.y * right.x;
    };
    std::optional<BulletCollisionResult> closest;
    for (unsigned int polygon_id = 0; polygon_id < map.GetPolygons().size(); ++polygon_id) {
        const auto& polygon = map.GetPolygons().at(polygon_id);
        if (!Map::BulletCollidesWithPolygon(polygon.polygon_type,
                                            static_cast<std::uint8_t>(bullet.team))) {
            continue;
        }
        float fraction = Map::PointInPoly(start_point, polygon) ? 0.0F : 2.0F;
        for (unsigned int edge = 0; edge < 3; ++edge) {
            const auto& first = polygon.vertices.at(edge);
            const auto& second = polygon.vertices.at((edge + 1) % 3);
            const glm::vec2 edge_start{ first.x, first.y };
            const glm::vec2 edge_vector{ second.x - first.x, second.y - first.y };
            const float denominator = cross(travel, edge_vector);
            if (std::abs(denominator) <= 0.000001F) {
                continue;
            }
            const auto delta = edge_start - start_point;
            const float along_travel = cross(delta, edge_vector) / denominator;
            const float along_edge = cross(delta, travel) / denominator;
            if (along_travel >= 0.0F && along_travel <= 1.0F && along_edge >= 0.0F &&
                along_edge <= 1.0F) {
                fraction = std::min(fraction, along_travel);
            }
        }
        if (fraction > 1.0F) {
            continue;
        }
        const auto position = start_point + travel * fraction;
        const auto sector = map.GetSectorIndex(position);
        if (sector.x < 0 || sector.y < 0 || sector.x >= map.GetSectorsPerAxis() ||
            sector.y >= map.GetSectorsPerAxis()) {
            continue;
        }
        const float distance = glm::length(position - start_point);
        if (closest && distance >= closest->distance) {
            continue;
        }
        closest = BulletCollisionResult{
            .kind = BulletCollisionKind::MapPolygon,
            .position = position,
            .distance = distance,
            .surface_normal = GetMapImpactNormal(polygon, position, travel),
            .polygon_id = polygon_id,
        };
    }
    return closest;
}

std::optional<BulletCollisionResult> FindSoldierCollision(const Bullet& bullet,
                                                          const StateManager& state_manager,
                                                          float last_hit_distance)
{
    if (bullet.style == BulletType::ClusterGrenade) {
        return std::nullopt;
    }
    constexpr int ARROW_RESIST = 280;
    if (bullet.style == BulletType::Arrow && bullet.timeout <= ARROW_RESIST) {
        return std::nullopt;
    }

    std::optional<BulletCollisionResult> closest_collision;
    state_manager.ForEachSoldier([&](const Soldier& candidate) {
        if ((bullet.style == BulletType::Fist || bullet.style == BulletType::Blade) &&
            candidate.id == bullet.owner_id) {
            return;
        }

        if (!candidate.CanReceiveCombatImpact() || !candidate.skeleton ||
            std::ranges::contains(bullet.penetrated_soldiers, candidate.id)) {
            return;
        }
        const bool self = candidate.id == bullet.owner_id;
        const int grace_ticks = bullet.style == BulletType::FragGrenade
                                  ? 50
                                  : (bullet.style == BulletType::Flame ? 0 : 20);
        if (self && (bullet.style == BulletType::Flame ||
                     bullet.timeout >= bullet.timeout_real - grace_ticks)) {
            return;
        }
        const bool same_team = bullet.team != TeamType::None && bullet.team == candidate.team;
        const auto mask = WeaponParametersFactory::GetParameters(bullet.weapon, false).no_collision;
        if (!AllowsWeaponCollision(mask, self, same_team, false)) {
            return;
        }

        const auto collision =
          FindSoldierCollisionPoint(candidate, bullet, state_manager.GetGameTick());
        if (!collision.has_value() ||
            (closest_collision.has_value() && collision->distance >= closest_collision->distance)) {
            return;
        }

        closest_collision = collision;
    });

    if (closest_collision.has_value() && last_hit_distance > -1.0F &&
        closest_collision->distance > last_hit_distance) {
        return std::nullopt;
    }

    return closest_collision;
}

std::optional<BulletCollisionResult> FindColliderCollision(const Bullet& bullet, const Map& map)
{
    if (bullet.style == BulletType::FragGrenade && bullet.timeout >= bullet.timeout_real - 2) {
        return std::nullopt;
    }
    constexpr float COLLIDER_RADIUS_SCALE = 1.7F;
    const glm::vec2 start_point = bullet.particle.old_position;
    const glm::vec2 end_point = bullet.particle.position;
    std::optional<BulletCollisionResult> closest_collision;

    for (unsigned int collider_id = 0; collider_id < map.GetColliders().size(); ++collider_id) {
        const PMSCollider& collider = map.GetColliders().at(collider_id);
        if (collider.active == 0) {
            continue;
        }

        const auto hit_position =
          Calc::LineCircleCollision(start_point,
                                    end_point,
                                    { collider.x, collider.y },
                                    collider.radius / COLLIDER_RADIUS_SCALE);
        if (!hit_position.has_value()) {
            continue;
        }

        const float distance = Calc::Vec2Length(*hit_position - start_point);
        if (closest_collision.has_value() && distance >= closest_collision->distance) {
            continue;
        }

        closest_collision = BulletCollisionResult{
            .kind = BulletCollisionKind::MapCollider,
            .position = *hit_position,
            .distance = distance,
            .collider_id = collider_id,
        };
    }

    return closest_collision;
}

std::optional<BulletCollisionResult> FindThingCollision(const Bullet& bullet,
                                                        const StateManager& state_manager)
{
    constexpr float THING_COLLISION_RADIUS = 10.0F;
    constexpr std::int16_t ITEM_COLLISION_GRACE_TICKS = 1;

    if (bullet.style == BulletType::FragGrenade || bullet.style == BulletType::M2Bullet ||
        bullet.timeout >= bullet.timeout_real - ITEM_COLLISION_GRACE_TICKS) {
        return std::nullopt;
    }

    const glm::vec2 start_point = bullet.particle.old_position;
    const glm::vec2 end_point = bullet.particle.position;
    std::optional<BulletCollisionResult> closest_collision;
    state_manager.ForEachItem([&](const Item& item) {
        if (!item.collide_with_bullets ||
            (item.holding_soldier_id != 0 && item.holding_soldier_id == bullet.owner_id) ||
            item.style == ItemType::M2 || !item.skeleton) {
            return;
        }

        for (unsigned int particle_id = 1; particle_id <= 2; ++particle_id) {
            const auto hit_position = Calc::LineCircleCollision(
              start_point, end_point, item.skeleton->GetPos(particle_id), THING_COLLISION_RADIUS);
            if (!hit_position.has_value()) {
                continue;
            }

            const float distance = Calc::Vec2Length(*hit_position - start_point);
            if (closest_collision.has_value() && distance >= closest_collision->distance) {
                continue;
            }

            closest_collision = BulletCollisionResult{
                .kind = BulletCollisionKind::Item,
                .position = *hit_position,
                .distance = distance,
                .item_id = item.id,
                .item_particle_id = particle_id,
            };
        }
    });

    return closest_collision;
}
} // namespace Soldank::BulletCollision
