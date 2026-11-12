module;

#include <cstdint>
#include <optional>

export module Shared.Core.Physics.Bullets.BulletTypes;

import Extern.Glm;

export namespace Soldank
{
enum class BulletCollisionKind
{
    MapPolygon,
    MapCollider,
    Soldier,
    Item,
};

struct BulletCollisionResult
{
    BulletCollisionKind kind;
    glm::vec2 position;
    float distance;
    std::optional<glm::vec2> surface_normal{};
    std::optional<unsigned int> polygon_id{};
    std::optional<unsigned int> collider_id{};
    std::optional<std::uint8_t> soldier_id{};
    std::optional<int> body_part_id{};
    std::optional<std::uint8_t> item_id{};
    std::optional<unsigned int> item_particle_id{};
};
} // namespace Soldank
