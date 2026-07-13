#include "core/math/Glm.hpp"

#include <gtest/gtest.h>

#include <algorithm>

import Shared.Core.Map.PMSEnums;
import Shared.Core.Map.PMSStructs;
import Shared.Core.Map.Map;
import Testing.Framework.Shared.MapBuilder;

void CheckPolygonSectorReferences(const Soldank::Map& map, unsigned short polygon_id)
{
    bool found_polygon = false;
    for (unsigned int x = 0; x < 51; ++x) {
        for (unsigned int y = 0; y < 51; ++y) {
            const Soldank::PMSSector& sector = map.GetSector(x, y);
            for (const auto referenced_polygon_id : sector.polygons) {
                EXPECT_GE(referenced_polygon_id, 1);
                EXPECT_LE(referenced_polygon_id, map.GetPolygonsCount());
            }
            if (std::ranges::contains(sector.polygons, polygon_id)) {
                found_polygon = true;
            }
        }
    }
    EXPECT_TRUE(found_polygon);
}

TEST(MapTests, TestMapLoadedCorrectly)
{
    auto map =
      SoldankTesting::MapBuilder::Empty()
        ->AddPolygon(
          { -112.0F, 0.0F }, { 0.0F, -20.0F }, { 16.0F, -128.0F }, Soldank::PMSPolygonType::Normal)
        ->Build();
    ASSERT_EQ(map->GetDescription(), "Test map");
    ASSERT_EQ(map->GetPolygons().size(), 1);
    ASSERT_FLOAT_EQ(map->GetPolygons().at(0).vertices.at(0).x, -64.0F);
    ASSERT_FLOAT_EQ(map->GetPolygons().at(0).vertices.at(0).y, 64.0F);
    ASSERT_FLOAT_EQ(map->GetPolygons().at(0).vertices.at(1).x, 64.0F);
    ASSERT_FLOAT_EQ(map->GetPolygons().at(0).vertices.at(1).y, -64.0F);
    ASSERT_FLOAT_EQ(map->GetPolygons().at(0).vertices.at(2).x, 48.0F);
    ASSERT_FLOAT_EQ(map->GetPolygons().at(0).vertices.at(2).y, 44.0F);
    CheckPolygonSectorReferences(*map, 1);
    ASSERT_EQ(map->GetPolygons().at(0).polygon_type, Soldank::PMSPolygonType::Normal);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
