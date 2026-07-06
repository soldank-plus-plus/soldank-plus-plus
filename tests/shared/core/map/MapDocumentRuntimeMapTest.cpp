#include "gtest/gtest.h"

#include "core/math/Glm.hpp"

import Shared.Core.Map.MapDocument;
import Shared.Core.Map.PMSEnums;
import Shared.Core.Map.PMSStructs;
import Shared.Core.Map.RuntimeMap;

namespace
{
Soldank::PMSPolygon CreateTestPolygon()
{
    Soldank::PMSPolygon polygon{};
    polygon.vertices.at(0).x = 100.0F;
    polygon.vertices.at(0).y = 100.0F;
    polygon.vertices.at(1).x = 200.0F;
    polygon.vertices.at(1).y = 100.0F;
    polygon.vertices.at(2).x = 100.0F;
    polygon.vertices.at(2).y = 200.0F;
    polygon.polygon_type = Soldank::PMSPolygonType::Normal;
    return polygon;
}
} // namespace

TEST(MapDocumentRuntimeMapTest, BuildRuntimeMapDoesNotMutateDocument)
{
    Soldank::MapDocument document;
    document.CreateEmptyMap();
    document.GetMutableMap().AddNewPolygon(CreateTestPolygon());

    const auto document_vertex_before = document.GetMap().GetPolygons().at(0).vertices.at(0);

    Soldank::RuntimeMap runtime_map = Soldank::RuntimeMap::BuildFromDocument(document);

    const auto document_vertex_after = document.GetMap().GetPolygons().at(0).vertices.at(0);
    EXPECT_FLOAT_EQ(document_vertex_after.x, document_vertex_before.x);
    EXPECT_FLOAT_EQ(document_vertex_after.y, document_vertex_before.y);

    const auto runtime_vertex = runtime_map.GetMap().GetPolygons().at(0).vertices.at(0);
    EXPECT_NE(runtime_vertex.x, document_vertex_before.x);
    EXPECT_NE(runtime_vertex.y, document_vertex_before.y);
    EXPECT_EQ(runtime_map.GetMap().GetSectorsCount(), 25);
    EXPECT_NE(runtime_map.GetDocumentToRuntimeOffset(), glm::vec2(0.0F, 0.0F));
}
