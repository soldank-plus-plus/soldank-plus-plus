module;

#include <array>
#include <cstdint>

export module Shared.Core.Map.Polygons.PolygonTraits;

import Shared.Core.Map.PMSEnums;

export namespace Soldank
{
enum class PolygonSurface : std::uint8_t
{
    Ordinary,
    Ice,
    Bouncy,
};
enum class PolygonEffect : std::uint8_t
{
    None,
    Deadly,
    BloodyDeadly,
    Hurts,
    Regenerates,
    Lava,
    Explosive,
    HurtFlaggers,
};
struct PolygonTraits
{
    PolygonSurface surface{};
    PolygonEffect effect{};
    bool background{};
};
constexpr bool IsValidPolygonType(PMSPolygonType type)
{
    return static_cast<unsigned int>(type) <= 25U;
}
constexpr PolygonTraits GetPolygonTraits(PMSPolygonType type)
{
    PolygonTraits traits{};
    switch (type) {
        case PMSPolygonType::Ice:
            traits.surface = PolygonSurface::Ice;
            break;
        case PMSPolygonType::Bouncy:
            traits.surface = PolygonSurface::Bouncy;
            break;
        case PMSPolygonType::Deadly:
            traits.effect = PolygonEffect::Deadly;
            break;
        case PMSPolygonType::BloodyDeadly:
            traits.effect = PolygonEffect::BloodyDeadly;
            break;
        case PMSPolygonType::Hurts:
            traits.effect = PolygonEffect::Hurts;
            break;
        case PMSPolygonType::Regenerates:
            traits.effect = PolygonEffect::Regenerates;
            break;
        case PMSPolygonType::Lava:
            traits.effect = PolygonEffect::Lava;
            break;
        case PMSPolygonType::Explosive:
            traits.effect = PolygonEffect::Explosive;
            break;
        case PMSPolygonType::HurtFlaggers:
            traits.effect = PolygonEffect::HurtFlaggers;
            break;
        case PMSPolygonType::Background:
        case PMSPolygonType::BackgroundTransition:
            traits.background = true;
            break;
        default:
            break;
    }
    return traits;
}
constexpr auto POLYGON_TRAITS = [] {
    std::array<PolygonTraits, 26> traits{};
    for (unsigned int id = 0; id < traits.size(); ++id) {
        traits.at(id) = GetPolygonTraits(static_cast<PMSPolygonType>(id));
    }
    return traits;
}();
} // namespace Soldank
