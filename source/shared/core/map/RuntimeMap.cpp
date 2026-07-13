module;

export module Shared.Core.Map.RuntimeMap;

import Extern.Glm;

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
        runtime_map.document_to_runtime_offset_ = runtime_map.map_.NormalizeCoordinatesForRuntime();
        runtime_map.map_.GenerateSectors();

        return runtime_map;
    }

    const Map& GetMap() const { return map_; }

    glm::vec2 GetDocumentToRuntimeOffset() const { return document_to_runtime_offset_; }

private:
    Map map_;
    glm::vec2 document_to_runtime_offset_{ 0.0F, 0.0F };
};
} // namespace Soldank
