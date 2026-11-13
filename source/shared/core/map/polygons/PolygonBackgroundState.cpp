module;

#include <cstdint>
#include <span>

export module Shared.Core.Map.Polygons.PolygonBackgroundState;

import Shared.Core.Map.PMSEnums;
import Shared.Core.Map.PMSStructs;
import Shared.Core.Map.Polygons.PolygonTraits;

export namespace Soldank
{
enum class PolygonBackgroundPhase : std::uint8_t
{
    Normal,
    Transition,
};
struct PolygonBackgroundState
{
    static constexpr std::int32_t UNKNOWN = -2;
    static constexpr std::int32_t NONE = -1;
    std::uint64_t map_fingerprint{};
    PolygonBackgroundPhase phase{ PolygonBackgroundPhase::Transition };
    std::int32_t polygon{ UNKNOWN };
    bool operator==(const PolygonBackgroundState&) const = default;
};
class PolygonBackgroundContact
{
public:
    explicit PolygonBackgroundContact(PolygonBackgroundState& state)
        : state_(state)
    {
    }
    bool Test(PMSPolygonType type, unsigned int polygon)
    {
        if (type == PMSPolygonType::BackgroundTransition ||
            (type == PMSPolygonType::Background &&
             state_.phase == PolygonBackgroundPhase::Transition)) {
            touched_ = true;
            state_.phase = PolygonBackgroundPhase::Transition;
            state_.polygon = static_cast<std::int32_t>(polygon);
            return true;
        }
        return false;
    }
    void Finish()
    {
        if (!touched_) {
            state_.phase = PolygonBackgroundPhase::Normal;
            state_.polygon = PolygonBackgroundState::NONE;
        }
    }

private:
    PolygonBackgroundState& state_;
    bool touched_{};
};
bool IsValidBackgroundState(const PolygonBackgroundState& state, unsigned int polygon_count)
{
    return state.phase <= PolygonBackgroundPhase::Transition &&
           state.polygon >= PolygonBackgroundState::UNKNOWN &&
           (state.polygon < 0 || static_cast<unsigned int>(state.polygon) < polygon_count) &&
           (state.phase != PolygonBackgroundPhase::Normal ||
            state.polygon == PolygonBackgroundState::NONE);
}
bool IsValidBackgroundState(const PolygonBackgroundState& state,
                            std::span<const PMSPolygon> polygons)
{
    return IsValidBackgroundState(state, static_cast<unsigned int>(polygons.size())) &&
           (state.polygon < 0 ||
            GetPolygonTraits(polygons[static_cast<unsigned int>(state.polygon)].polygon_type)
              .background);
}
} // namespace Soldank
