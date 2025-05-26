#include "shared_lib_testing/MapBuilder.hpp"
#include "core/map/PMSEnums.hpp"
#include "core/map/PMSStructs.hpp"

#include <gtest/gtest.h>

#include <vector>
#include <algorithm>

void CheckIfPolygonInSectors(const Soldank::Map& map,
                             unsigned short polygon_id,
                             const std::vector<glm::uvec2>& sectors)
{
    for (unsigned int x = 0; x < 51; x++) {
        for (unsigned int y = 0; y < 51; y++) {
            const Soldank::PMSSector& sector = map.GetSector(x, y);
            if (std::ranges::contains(sectors, glm::uvec2{ x, y })) {
                ASSERT_TRUE(std::ranges::contains(sector.polygons, polygon_id));
            } else {
                ASSERT_FALSE(std::ranges::contains(sector.polygons, polygon_id));
            }
        }
    }
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
    CheckIfPolygonInSectors(
      *map, 1, { { 14, 35 }, { 14, 36 }, { 15, 34 }, { 15, 35 }, { 15, 36 }, { 16, 33 }, { 16, 34 },
                 { 16, 35 }, { 16, 36 }, { 17, 32 }, { 17, 33 }, { 17, 34 }, { 17, 35 }, { 18, 31 },
                 { 18, 32 }, { 18, 33 }, { 18, 34 }, { 18, 35 }, { 19, 30 }, { 19, 31 }, { 19, 32 },
                 { 19, 33 }, { 19, 34 }, { 19, 35 }, { 20, 29 }, { 20, 30 }, { 20, 31 }, { 20, 32 },
                 { 20, 33 }, { 20, 34 }, { 20, 35 }, { 21, 28 }, { 21, 29 }, { 21, 30 }, { 21, 31 },
                 { 21, 32 }, { 21, 33 }, { 21, 34 }, { 21, 35 }, { 22, 27 }, { 22, 28 }, { 22, 29 },
                 { 22, 30 }, { 22, 31 }, { 22, 32 }, { 22, 33 }, { 22, 34 }, { 22, 35 }, { 23, 26 },
                 { 23, 27 }, { 23, 28 }, { 23, 29 }, { 23, 30 }, { 23, 31 }, { 23, 32 }, { 23, 33 },
                 { 23, 34 }, { 24, 25 }, { 24, 26 }, { 24, 27 }, { 24, 28 }, { 24, 29 }, { 24, 30 },
                 { 24, 31 }, { 24, 32 }, { 24, 33 }, { 24, 34 }, { 25, 24 }, { 25, 25 }, { 25, 26 },
                 { 25, 27 }, { 25, 28 }, { 25, 29 }, { 25, 30 }, { 25, 31 }, { 25, 32 }, { 25, 33 },
                 { 25, 34 }, { 26, 23 }, { 26, 24 }, { 26, 25 }, { 26, 26 }, { 26, 27 }, { 26, 28 },
                 { 26, 29 }, { 26, 30 }, { 26, 31 }, { 26, 32 }, { 26, 33 }, { 26, 34 }, { 27, 22 },
                 { 27, 23 }, { 27, 24 }, { 27, 25 }, { 27, 26 }, { 27, 27 }, { 27, 28 }, { 27, 29 },
                 { 27, 30 }, { 27, 31 }, { 27, 32 }, { 27, 33 }, { 27, 34 }, { 28, 21 }, { 28, 22 },
                 { 28, 23 }, { 28, 24 }, { 28, 25 }, { 28, 26 }, { 28, 27 }, { 28, 28 }, { 28, 29 },
                 { 28, 30 }, { 28, 31 }, { 28, 32 }, { 28, 33 }, { 28, 34 }, { 29, 20 }, { 29, 21 },
                 { 29, 22 }, { 29, 23 }, { 29, 24 }, { 29, 25 }, { 29, 26 }, { 29, 27 }, { 29, 28 },
                 { 29, 29 }, { 29, 30 }, { 29, 31 }, { 29, 32 }, { 29, 33 }, { 30, 19 }, { 30, 20 },
                 { 30, 21 }, { 30, 22 }, { 30, 23 }, { 30, 24 }, { 30, 25 }, { 30, 26 }, { 30, 27 },
                 { 30, 28 }, { 30, 29 }, { 30, 30 }, { 30, 31 }, { 30, 32 }, { 30, 33 }, { 31, 18 },
                 { 31, 19 }, { 31, 20 }, { 31, 21 }, { 31, 22 }, { 31, 23 }, { 31, 24 }, { 31, 25 },
                 { 31, 26 }, { 31, 27 }, { 31, 28 }, { 31, 29 }, { 31, 30 }, { 31, 31 }, { 31, 32 },
                 { 31, 33 }, { 32, 17 }, { 32, 18 }, { 32, 19 }, { 32, 20 }, { 32, 21 }, { 32, 22 },
                 { 32, 23 }, { 32, 24 }, { 32, 25 }, { 32, 26 }, { 32, 27 }, { 32, 28 }, { 32, 29 },
                 { 32, 30 }, { 32, 31 }, { 32, 32 }, { 32, 33 }, { 33, 16 }, { 33, 17 }, { 33, 18 },
                 { 33, 19 }, { 33, 20 }, { 33, 21 }, { 33, 22 }, { 33, 23 }, { 33, 24 }, { 33, 25 },
                 { 33, 26 }, { 33, 27 }, { 33, 28 }, { 33, 29 }, { 33, 30 }, { 33, 31 }, { 33, 32 },
                 { 33, 33 }, { 34, 15 }, { 34, 16 }, { 34, 17 }, { 34, 18 }, { 34, 19 }, { 34, 20 },
                 { 34, 21 }, { 34, 22 }, { 34, 23 }, { 34, 24 }, { 34, 25 }, { 34, 26 }, { 34, 27 },
                 { 34, 28 }, { 34, 29 }, { 34, 30 }, { 35, 14 }, { 35, 15 }, { 35, 16 }, { 35, 17 },
                 { 35, 18 }, { 35, 19 }, { 35, 20 }, { 35, 21 }, { 35, 22 }, { 35, 23 }, { 36, 14 },
                 { 36, 15 }, { 36, 16 }, { 36, 17 } });
    ASSERT_EQ(map->GetPolygons().at(0).polygon_type, Soldank::PMSPolygonType::Normal);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

