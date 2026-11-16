module;

#include <cstdint>
#include <vector>

export module Shared.Core.Physics.Polygons.PolygonContacts;

import Extern.Glm;
import Shared.Core.Map.Map;
import Shared.Core.Map.PMSEnums;
import Shared.Core.Map.PMSStructs;
import Shared.Core.Map.Polygons.PolygonTraits;
import Shared.Core.Map.Polygons.PolygonBackgroundState;
import Shared.Core.Map.Polygons.PolygonCollisionPolicy;
import Shared.Core.Entities.Soldier;
import Shared.Core.Entities.Item;

export namespace Soldank::PolygonContacts
{
void Observe(PolygonBackgroundState& state, bool& touched, const PMSPolygon& polygon)
{
    if (polygon.polygon_type == PMSPolygonType::BackgroundTransition ||
        (polygon.polygon_type == PMSPolygonType::Background &&
         state.phase == PolygonBackgroundPhase::Transition)) {
        state.phase = PolygonBackgroundPhase::Transition;
        state.polygon = static_cast<int>(polygon.id);
        touched = true;
    }
}
void Begin(const Map& map, PolygonBackgroundState& state, bool& touched, glm::vec2 center)
{
    touched = false;
    const auto fingerprint = map.GetPolygonFingerprint();
    if (state.map_fingerprint != fingerprint) {
        state = {};
    }
    state.map_fingerprint = fingerprint;
    if (!IsValidBackgroundState(state, map.GetPolygons())) {
        state = {};
        state.map_fingerprint = fingerprint;
    }
    if (state.phase == PolygonBackgroundPhase::Transition) {
        // Resolve against actual map IDs, never an index into a background-only list.
        if (state.polygon >= 0) {
            const auto& cached = map.GetPolygons().at(static_cast<unsigned int>(state.polygon));
            if (GetPolygonTraits(cached.polygon_type).background &&
                Map::PointInPoly(center, cached)) {
                Observe(state, touched, cached);
                return;
            }
        }
        for (const auto& polygon : map.GetPolygons()) {
            if (GetPolygonTraits(polygon.polygon_type).background &&
                Map::PointInPoly(center, polygon)) {
                Observe(state, touched, polygon);
                break;
            }
        }
    }
}
void Finish(PolygonBackgroundState& state, bool touched)
{
    if (!touched) {
        state.phase = PolygonBackgroundPhase::Normal;
        state.polygon = PolygonBackgroundState::NONE;
    }
}
bool SoldierContact(Soldier& soldier, const Map& map, unsigned int id, glm::vec2 /*point*/)
{
    const auto& polygon = map.GetPolygons().at(id);
    if (GetPolygonTraits(polygon.polygon_type).background) {
        Observe(soldier.polygon_background, soldier.background_touched, polygon);
    }
    return AllowsPolygon(
      polygon.polygon_type,
      { .kind = soldier.dead_meat ? PolygonQueryKind::Corpse : PolygonQueryKind::Soldier,
        .team = static_cast<std::uint8_t>(soldier.GetTeam()),
        .carrying_flag = soldier.is_holding_flags,
        .background = soldier.polygon_background.phase });
}
bool ItemContact(Item& item, const Map& map, unsigned int id, glm::vec2 /*point*/, bool flag)
{
    const auto& polygon = map.GetPolygons().at(id);
    if (GetPolygonTraits(polygon.polygon_type).background) {
        Observe(item.polygon_background, item.background_touched, polygon);
    }
    return AllowsPolygon(polygon.polygon_type,
                         { .kind = PolygonQueryKind::Item,
                           .team = item.collision_owner_team,
                           .flag_item = flag,
                           .has_owner = item.has_collision_owner,
                           .background = item.polygon_background.phase });
}
} // namespace Soldank::PolygonContacts
