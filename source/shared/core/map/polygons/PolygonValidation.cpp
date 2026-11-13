module;

#include <cmath>
#include <stdexcept>

export module Shared.Core.Map.Polygons.PolygonValidation;

import Shared.Core.Map.PMSStructs;
import Shared.Core.Map.Polygons.PolygonTraits;

export namespace Soldank
{
void ValidatePolygon(const PMSPolygon& polygon)
{
    if (!IsValidPolygonType(polygon.polygon_type) || !std::isfinite(polygon.bounciness) ||
        polygon.bounciness < 0.0F) {
        throw std::invalid_argument("Invalid PMS polygon type or bounce strength");
    }
    for (const auto& vertex : polygon.vertices) {
        if (!std::isfinite(vertex.x) || !std::isfinite(vertex.y) || !std::isfinite(vertex.z) ||
            !std::isfinite(vertex.rhw) || !std::isfinite(vertex.texture_s) ||
            !std::isfinite(vertex.texture_t)) {
            throw std::invalid_argument("Non-finite PMS polygon vertex");
        }
    }
    for (const auto& normal : polygon.perpendiculars) {
        if (!std::isfinite(normal.x) || !std::isfinite(normal.y) || !std::isfinite(normal.z)) {
            throw std::invalid_argument("Non-finite PMS polygon normal");
        }
    }
    // Finite degenerate triangles remain editable but have no interior collision.
}
} // namespace Soldank
