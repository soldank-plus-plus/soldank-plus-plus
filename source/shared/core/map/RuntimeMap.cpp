module;

#include "core/math/Glm.hpp"

#include <optional>

export module Shared.Core.Map.RuntimeMap;

import Shared.Core.Map.Map;
import Shared.Core.Map.MapDocument;

export namespace Soldank
{
class RuntimeMap
{
public:
    RuntimeMap() = default;

    static RuntimeMap BuildFromDocument(const MapDocument& document)
    {
        RuntimeMap runtime_map;
        runtime_map.map_ = document.GetMap();

        std::optional<glm::vec2> old_first_polygon_vertex;
        if (!runtime_map.map_.GetPolygons().empty()) {
            const auto vertex = runtime_map.map_.GetPolygons().at(0).vertices.at(0);
            old_first_polygon_vertex = glm::vec2{ vertex.x, vertex.y };
        }

        runtime_map.map_.GenerateSectors();

        if (old_first_polygon_vertex.has_value() && !runtime_map.map_.GetPolygons().empty()) {
            const auto vertex = runtime_map.map_.GetPolygons().at(0).vertices.at(0);
            runtime_map.document_to_runtime_offset_ =
              glm::vec2{ vertex.x, vertex.y } - *old_first_polygon_vertex;
        }

        return runtime_map;
    }

    const Map& GetMap() const { return map_; }

    glm::vec2 GetDocumentToRuntimeOffset() const { return document_to_runtime_offset_; }

private:
    Map map_;
    glm::vec2 document_to_runtime_offset_{ 0.0F, 0.0F };
};
} // namespace Soldank
