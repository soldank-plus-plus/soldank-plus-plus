module;

#include <vector>
#include <span>

export module Shared.Core.Map.Polygons.PolygonRenderBatch;

import Shared.Core.Map.PMSStructs;
import Shared.Core.Map.Polygons.PolygonTraits;

export namespace Soldank
{
struct PolygonRenderBatch
{
    std::vector<PMSPolygon> polygons;
    unsigned int background_count{};
};
PolygonRenderBatch BuildPolygonRenderBatch(std::span<const PMSPolygon> polygons)
{
    PolygonRenderBatch result;
    result.polygons.reserve(polygons.size());
    for (const auto& polygon : polygons) {
        if (GetPolygonTraits(polygon.polygon_type).background) {
            result.polygons.push_back(polygon);
        }
    }
    result.background_count = static_cast<unsigned int>(result.polygons.size());
    for (const auto& polygon : polygons) {
        if (!GetPolygonTraits(polygon.polygon_type).background) {
            result.polygons.push_back(polygon);
        }
    }
    return result;
}
} // namespace Soldank
