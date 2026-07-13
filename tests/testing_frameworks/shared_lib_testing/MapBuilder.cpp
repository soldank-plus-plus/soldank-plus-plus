module;

#include "core/math/Glm.hpp"

#include <memory>

export module Testing.Framework.Shared.MapBuilder;

import Shared.Core.Map.Map;
import Shared.Core.Map.PMSEnums;
import Shared.Core.Map.PMSStructs;

export namespace SoldankTesting
{
class MapBuilder
{
public:
    static std::unique_ptr<MapBuilder> Empty()
    {
        auto builder = std::unique_ptr<MapBuilder>(new MapBuilder());
        builder->map_->CreateEmptyMap();
        builder->map_->SetDescription("Test map");
        return builder;
    }

    MapBuilder* AddPolygon(glm::vec2 vertex1,
                           glm::vec2 vertex2,
                           glm::vec2 vertex3,
                           Soldank::PMSPolygonType polygon_type)
    {
        Soldank::PMSPolygon polygon{};
        polygon.vertices.at(0).x = vertex1.x;
        polygon.vertices.at(0).y = vertex1.y;
        polygon.vertices.at(1).x = vertex2.x;
        polygon.vertices.at(1).y = vertex2.y;
        polygon.vertices.at(2).x = vertex3.x;
        polygon.vertices.at(2).y = vertex3.y;
        polygon.polygon_type = polygon_type;
        polygon.bounciness = 1.0F;
        map_->AddNewPolygon(polygon);
        return this;
    }

    std::unique_ptr<Soldank::Map> Build()
    {
        map_->NormalizeCoordinatesForRuntime();
        map_->GenerateSectors();
        return std::move(map_);
    }

private:
    MapBuilder()
        : map_(std::make_unique<Soldank::Map>())
    {
    }

    std::unique_ptr<Soldank::Map> map_;
};
} // namespace SoldankTesting
