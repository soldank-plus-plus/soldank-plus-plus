module;

#include <cstdint>

export module Shared.Core.Physics.Polygons.PolygonContact;

import Extern.Glm;
import Shared.Core.Map.PMSEnums;

export namespace Soldank
{
enum class PolygonProbe : std::uint8_t
{
    Body,
    Radius,
    Vertex,
    Landing,
    Bounce,
};
enum class DeathSeverity : std::uint8_t
{
    Normal,
    Head,
    Brutal,
};
struct PolygonContact
{
    unsigned int polygon{};
    PMSPolygonType type{};
    glm::vec2 position{};
    glm::vec2 velocity{};
    PolygonProbe probe{};
};
enum class PolygonEventKind : std::uint8_t
{
    Hurt,
    Heal,
    Lava,
    Bounce,
    Landing,
    Death,
};
struct PolygonEffectEvent
{
    PolygonEventKind kind{};
    std::uint8_t soldier{};
    unsigned int tick{};
    std::uint32_t life{};
    unsigned int polygon{};
    unsigned int ordinal{};
    glm::vec2 position{};
    float amount{};
    DeathSeverity severity{};
    std::uint64_t map_fingerprint{};
    bool operator==(const PolygonEffectEvent&) const = default;
};
} // namespace Soldank
