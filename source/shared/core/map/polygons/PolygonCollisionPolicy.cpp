module;

#include <cstdint>

export module Shared.Core.Map.Polygons.PolygonCollisionPolicy;

import Shared.Core.Map.PMSEnums;
import Shared.Core.Map.Polygons.PolygonTraits;
import Shared.Core.Map.Polygons.PolygonBackgroundState;

export namespace Soldank
{
enum class PolygonQueryKind : std::uint8_t
{
    Soldier,
    Corpse,
    Projectile,
    Item,
    Ray,
    Point,
    Cosmetic,
};
struct PolygonQuery
{
    PolygonQueryKind kind{};
    std::uint8_t team{};
    bool carrying_flag{};
    bool flag_item{};
    bool has_owner{};
    PolygonBackgroundPhase background{ PolygonBackgroundPhase::Normal };
    bool player{};
    bool flag{};
    bool bullet{};
};
namespace PolygonPolicyDetail
{
struct TeamRule
{
    unsigned int id;
    bool team_polygon;
    bool team_bullets;
    bool matching_team;
};
bool Ray(PMSPolygonType type, const PolygonQuery& query, const TeamRule& rule)
{
    if (rule.team_polygon) {
        return rule.matching_team && (rule.team_bullets ? query.bullet : query.player);
    }
    switch (type) {
        case PMSPolygonType::OnlyBulletsCollide:
            return query.bullet;
        case PMSPolygonType::OnlyPlayersCollide:
            return query.player;
        case PMSPolygonType::FlaggerCollides:
            return query.player && query.flag;
        case PMSPolygonType::NonFlaggerCollides:
            return query.player && !query.flag;
        case PMSPolygonType::FlagCollides:
            return query.player && query.flag && query.bullet;
        case PMSPolygonType::Background:
        case PMSPolygonType::BackgroundTransition:
            return false;
        default:
            return true;
    }
}
bool Item(const PolygonQuery& query, const TeamRule& rule)
{
    if (rule.id == 1 || rule.id == 2 || rule.id == 21 || rule.id == 22) {
        return false;
    }
    if (query.flag_item && rule.team_polygon) {
        return false;
    }
    if (query.has_owner && (rule.team_polygon || rule.id == 23)) {
        return rule.team_polygon && !rule.team_bullets && rule.matching_team;
    }
    return true;
}
bool Cosmetic(const PolygonQuery& query, const TeamRule& rule)
{
    if (!query.has_owner || rule.id == 1 || rule.id == 2 || rule.id == 24 ||
        (rule.id == 18 && !query.carrying_flag)) {
        return false;
    }
    // Sparks use owner-team rules but do not use soldier carrier gates for IDs 21/22.
    return rule.team_polygon ? (!rule.team_bullets && rule.matching_team) : rule.id != 23;
}
bool Soldier(const PolygonQuery& query, const TeamRule& rule)
{
    if (rule.team_polygon) {
        return !rule.team_bullets && rule.matching_team;
    }
    switch (rule.id) {
        case 1:
        case 23:
            return false;
        case 21:
            return query.carrying_flag;
        case 22:
            return !query.carrying_flag;
        default:
            return true;
    }
}
} // namespace PolygonPolicyDetail
bool AllowsPolygon(PMSPolygonType type, const PolygonQuery& query)
{
    if (!IsValidPolygonType(type) || type == PMSPolygonType::NoCollide) {
        return false;
    }
    const auto id = static_cast<unsigned int>(type);
    const bool team_polygon = id >= 10 && id <= 17;
    const bool team_bullets = team_polygon && id % 2 == 0;
    const PolygonPolicyDetail::TeamRule rule{ .id = id,
                                              .team_polygon = team_polygon,
                                              .team_bullets = team_bullets,
                                              .matching_team =
                                                team_polygon && query.team == ((id - 10) / 2) + 1 };
    switch (query.kind) {
        case PolygonQueryKind::Point:
            return id != 1 && id != 2 && id != 24 && id != 25 && (!team_polygon || team_bullets) &&
                   ((id < 21 || id > 23) || query.flag);
        case PolygonQueryKind::Ray:
            return PolygonPolicyDetail::Ray(type, query, rule);
        case PolygonQueryKind::Projectile:
            // The Bravo typo in upstream TeamCollides is intentionally corrected.
            return team_polygon ? (team_bullets && rule.matching_team)
                                : (id != 2 && (id < 21 || id > 25));
        default:
            break;
    }
    if (id == 25 || (id == 24 && query.background == PolygonBackgroundPhase::Transition)) {
        return false;
    }
    switch (query.kind) {
        case PolygonQueryKind::Item:
            return PolygonPolicyDetail::Item(query, rule);
        case PolygonQueryKind::Cosmetic:
            return PolygonPolicyDetail::Cosmetic(query, rule);
        default:
            return PolygonPolicyDetail::Soldier(query, rule);
    }
}
} // namespace Soldank
