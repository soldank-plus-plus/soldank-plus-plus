#include "core/math/Glm.hpp"

#include <gtest/gtest.h>

#include <array>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "core/utility/Expected.hpp"

import Shared.Core.Data.IFileReader;
import Shared.Core.Data.IFileWriter;
import Shared.Core.Map.Map;
import Shared.Core.Map.PMSConstants;
import Shared.Core.Map.PMSEnums;
import Shared.Core.Map.PMSStructs;

namespace
{
class BufferFileReader final : public Soldank::IFileReader
{
public:
    explicit BufferFileReader(std::string data)
        : data_(std::move(data))
    {
    }

    std::expected<std::string, Soldank::FileReaderError> Read(
      const std::string& /*file_path*/,
      std::ios_base::openmode /*mode*/) const override
    {
        return data_;
    }

private:
    std::string data_;
};

class BufferFileWriter final : public Soldank::IFileWriter
{
public:
    Soldank::FileWriterError AppendData(const char* data, std::streamsize data_size) override
    {
        data_.append(data, static_cast<std::size_t>(data_size));
        return Soldank::FileWriterError::NoError;
    }

    Soldank::FileWriterError Write(const std::filesystem::path& /*file_path*/,
                                   std::ios_base::openmode /*mode*/) const override
    {
        return Soldank::FileWriterError::NoError;
    }

    const std::string& GetData() const { return data_; }

private:
    std::string data_;
};

Soldank::PMSPolygon CreatePolygon(float x, unsigned int id = 0)
{
    Soldank::PMSPolygon polygon{};
    polygon.id = id;
    polygon.vertices.at(0).x = x;
    polygon.vertices.at(0).y = 0.0F;
    polygon.vertices.at(1).x = x + 10.0F;
    polygon.vertices.at(1).y = 0.0F;
    polygon.vertices.at(2).x = x;
    polygon.vertices.at(2).y = 10.0F;
    polygon.polygon_type = Soldank::PMSPolygonType::Normal;
    polygon.bounciness = 1.0F;
    return polygon;
}

Soldank::PMSSpawnPoint CreateSpawnPoint(int x)
{
    return Soldank::PMSSpawnPoint{
        .active = 1, .x = x, .y = x + 1, .type = Soldank::PMSSpawnPointType::General
    };
}

Soldank::PMSScenery CreateScenery(float x)
{
    Soldank::PMSScenery scenery{};
    scenery.active = true;
    scenery.width = 10;
    scenery.height = 20;
    scenery.x = x;
    scenery.scale_x = 1.0F;
    scenery.scale_y = 1.0F;
    return scenery;
}

std::shared_ptr<BufferFileWriter> SaveMapToBuffer(Soldank::Map& map)
{
    auto writer = std::make_shared<BufferFileWriter>();
    map.SaveMap("characterization-map.pms", writer);
    return writer;
}
} // namespace

TEST(MapBehaviorTest, MapDataHasDeterministicDefaults)
{
    const Soldank::MapData map_data;

    EXPECT_EQ(map_data.boundaries_xy, (std::array<float, 4>{}));
    EXPECT_FLOAT_EQ(map_data.polygons_min_x, 0.0F);
    EXPECT_FLOAT_EQ(map_data.polygons_max_x, 0.0F);
    EXPECT_FLOAT_EQ(map_data.polygons_min_y, 0.0F);
    EXPECT_FLOAT_EQ(map_data.polygons_max_y, 0.0F);
    EXPECT_FLOAT_EQ(map_data.width, 0.0F);
    EXPECT_FLOAT_EQ(map_data.height, 0.0F);
    EXPECT_FLOAT_EQ(map_data.center_x, 0.0F);
    EXPECT_FLOAT_EQ(map_data.center_y, 0.0F);
    EXPECT_EQ(map_data.version, Soldank::MAP_VERSION);
    EXPECT_EQ(map_data.name, std::nullopt);
    EXPECT_TRUE(map_data.description.empty());
    EXPECT_TRUE(map_data.texture_name.empty());
    EXPECT_EQ(map_data.jet_count, 0);
    EXPECT_EQ(map_data.grenades_count, 0);
    EXPECT_EQ(map_data.medikits_count, 0);
    EXPECT_EQ(map_data.weather_type, Soldank::PMSWeatherType::None);
    EXPECT_EQ(map_data.step_type, Soldank::PMSStepType::HardGround);
    EXPECT_EQ(map_data.random_id, 0);
    EXPECT_EQ(map_data.sectors_size, 0);
    EXPECT_EQ(map_data.sectors_count, 25);
    EXPECT_TRUE(map_data.polygons.empty());
    EXPECT_TRUE(map_data.sectors_poly.empty());
    EXPECT_TRUE(map_data.scenery_instances.empty());
    EXPECT_TRUE(map_data.scenery_types.empty());
    EXPECT_TRUE(map_data.colliders.empty());
    EXPECT_TRUE(map_data.spawn_points.empty());
    EXPECT_TRUE(map_data.way_points.empty());
}

TEST(MapBehaviorTest, CreateEmptyMapResetsExistingDataAndInitializesSerializableProperties)
{
    Soldank::MapData map_data;
    map_data.name = "old-map.pms";
    map_data.description = "old description";
    map_data.texture_name = "old-texture.png";
    map_data.polygons.push_back(CreatePolygon(100.0F));
    map_data.scenery_instances.push_back(CreateScenery(100.0F));
    map_data.scenery_types.push_back({ .name = "old-scenery.png", .timestamp = {} });
    map_data.colliders.push_back({ .active = 1, .x = 10.0F, .y = 20.0F, .radius = 5.0F });
    map_data.spawn_points.push_back(CreateSpawnPoint(100));
    map_data.way_points.push_back({});
    Soldank::Map map(std::move(map_data));

    map.CreateEmptyMap();

    EXPECT_EQ(map.GetVersion(), Soldank::MAP_VERSION);
    EXPECT_EQ(map.GetName(), std::nullopt);
    EXPECT_EQ(map.GetDescription(), "New Soldank++ map");
    EXPECT_EQ(map.GetTextureName(), "banana.png");
    EXPECT_EQ(map.GetJetCount(), 0);
    EXPECT_EQ(map.GetGrenadesCount(), 0);
    EXPECT_EQ(map.GetMedikitsCount(), 0);
    EXPECT_EQ(map.GetWeatherType(), Soldank::PMSWeatherType::None);
    EXPECT_EQ(map.GetStepType(), Soldank::PMSStepType::HardGround);
    EXPECT_TRUE(map.GetPolygons().empty());
    EXPECT_TRUE(map.GetSceneryInstances().empty());
    EXPECT_TRUE(map.GetSceneryTypes().empty());
    EXPECT_TRUE(map.GetColliders().empty());
    EXPECT_TRUE(map.GetSpawnPoints().empty());
    EXPECT_TRUE(map.GetWayPoints().empty());
    EXPECT_EQ(map.GetSectorsCount(), 25);
    EXPECT_EQ(map.GetSectorsSize(), 4);
    EXPECT_EQ(map.GetBoundaries()[Soldank::Map::TopBoundary], -Soldank::MAP_BOUNDARY);
    EXPECT_EQ(map.GetBoundaries()[Soldank::Map::BottomBoundary], Soldank::MAP_BOUNDARY);
    EXPECT_EQ(map.GetBoundaries()[Soldank::Map::LeftBoundary], -Soldank::MAP_BOUNDARY);
    EXPECT_EQ(map.GetBoundaries()[Soldank::Map::RightBoundary], Soldank::MAP_BOUNDARY);
}

TEST(MapBehaviorTest, BatchPolygonInsertionAndRemovalPreserveIndexedOrder)
{
    Soldank::Map map;
    map.CreateEmptyMap();
    map.AddNewPolygon(CreatePolygon(0.0F));
    map.AddNewPolygon(CreatePolygon(20.0F));

    map.AddPolygons({ CreatePolygon(10.0F, 1), CreatePolygon(30.0F, 3) });

    ASSERT_EQ(map.GetPolygons().size(), 4);
    for (unsigned int i = 0; i < map.GetPolygons().size(); ++i) {
        EXPECT_EQ(map.GetPolygons().at(i).id, i);
        EXPECT_FLOAT_EQ(map.GetPolygons().at(i).vertices.at(0).x, static_cast<float>(i) * 10.0F);
    }

    map.RemovePolygonsById({ 1, 3 });

    ASSERT_EQ(map.GetPolygons().size(), 2);
    EXPECT_EQ(map.GetPolygons().at(0).id, 0);
    EXPECT_FLOAT_EQ(map.GetPolygons().at(0).vertices.at(0).x, 0.0F);
    EXPECT_EQ(map.GetPolygons().at(1).id, 1);
    EXPECT_FLOAT_EQ(map.GetPolygons().at(1).vertices.at(0).x, 20.0F);
}

TEST(MapBehaviorTest, BatchSpawnPointInsertionAndRemovalPreserveIndexedOrder)
{
    Soldank::Map map;
    map.CreateEmptyMap();
    map.AddNewSpawnPoint(CreateSpawnPoint(10));
    map.AddNewSpawnPoint(CreateSpawnPoint(30));

    map.AddSpawnPoints({ { 1, CreateSpawnPoint(20) }, { 3, CreateSpawnPoint(40) } });

    ASSERT_EQ(map.GetSpawnPoints().size(), 4);
    EXPECT_EQ(map.GetSpawnPoints().at(0).x, 10);
    EXPECT_EQ(map.GetSpawnPoints().at(1).x, 20);
    EXPECT_EQ(map.GetSpawnPoints().at(2).x, 30);
    EXPECT_EQ(map.GetSpawnPoints().at(3).x, 40);

    map.RemoveSpawnPointsById({ 1, 3 });

    ASSERT_EQ(map.GetSpawnPoints().size(), 2);
    EXPECT_EQ(map.GetSpawnPoints().at(0).x, 10);
    EXPECT_EQ(map.GetSpawnPoints().at(1).x, 30);
}

TEST(MapBehaviorTest, RemovingLastSceneryOfATypeCompactsRemainingStyles)
{
    Soldank::Map map;
    map.CreateEmptyMap();
    map.AddNewScenery(CreateScenery(10.0F), "tree.png");
    map.AddNewScenery(CreateScenery(20.0F), "tree.png");
    map.AddNewScenery(CreateScenery(30.0F), "bush.png");

    ASSERT_EQ(map.GetSceneryTypes().size(), 2);
    EXPECT_EQ(map.GetSceneryInstances().at(0).style, 1);
    EXPECT_EQ(map.GetSceneryInstances().at(1).style, 1);
    EXPECT_EQ(map.GetSceneryInstances().at(2).style, 2);

    map.RemoveSceneryById(0);
    ASSERT_EQ(map.GetSceneryTypes().size(), 2);
    EXPECT_EQ(map.GetSceneryInstances().at(0).style, 1);
    EXPECT_EQ(map.GetSceneryInstances().at(1).style, 2);

    map.RemoveSceneryById(0);
    ASSERT_EQ(map.GetSceneryTypes().size(), 1);
    ASSERT_EQ(map.GetSceneryInstances().size(), 1);
    EXPECT_EQ(map.GetSceneryTypes().at(0).name, "bush.png");
    EXPECT_EQ(map.GetSceneryInstances().at(0).style, 1);
}

TEST(MapBehaviorTest, AddingPolygonEmitsTwoBoundaryEventsAndOnePolygonEvent)
{
    Soldank::Map map;
    map.CreateEmptyMap();
    unsigned int boundary_event_count = 0;
    unsigned int polygon_event_count = 0;
    map.GetMapChangeEvents().changed_background_color.AddObserver(
      [&boundary_event_count](const Soldank::PMSColor&,
                              const Soldank::PMSColor&,
                              std::span<const float, 4>) { ++boundary_event_count; });
    map.GetMapChangeEvents().added_new_polygon.AddObserver(
      [&polygon_event_count](const Soldank::PMSPolygon&) { ++polygon_event_count; });

    map.AddNewPolygon(CreatePolygon(100.0F));

    // This records the current duplicate boundary notification behavior. A later cleanup can
    // intentionally change this expectation to one event.
    EXPECT_EQ(boundary_event_count, 2);
    EXPECT_EQ(polygon_event_count, 1);
}

TEST(MapBehaviorTest, GeometryQueriesDistinguishClearlyInsideAndOutsidePoints)
{
    const auto polygon = CreatePolygon(0.0F);
    EXPECT_TRUE(Soldank::Map::PointInPoly({ 2.0F, 2.0F }, polygon));
    EXPECT_FALSE(Soldank::Map::PointInPoly({ 20.0F, 20.0F }, polygon));

    const auto scenery = CreateScenery(5.0F);
    EXPECT_TRUE(Soldank::Map::PointInScenery({ 10.0F, 10.0F }, scenery));
    EXPECT_FALSE(Soldank::Map::PointInScenery({ 20.0F, 30.0F }, scenery));

    glm::vec2 intersection{};
    EXPECT_TRUE(Soldank::Map::LineInPoly({ -5.0F, 2.0F }, { 15.0F, 2.0F }, polygon, intersection));
    EXPECT_FALSE(
      Soldank::Map::LineInPoly({ -5.0F, 20.0F }, { 15.0F, 20.0F }, polygon, intersection));
}

TEST(MapBehaviorTest, SavingDirtyMapCurrentlyRecentersEditablePolygonCoordinates)
{
    Soldank::Map map;
    map.CreateEmptyMap();
    map.AddNewPolygon(CreatePolygon(100.0F));
    const float x_before_save = map.GetPolygons().at(0).vertices.at(0).x;

    const auto writer = SaveMapToBuffer(map);

    EXPECT_FALSE(writer->GetData().empty());
    EXPECT_FLOAT_EQ(x_before_save, 100.0F);
    // Polygon min/max values are currently initialized to zero, so a map entirely in positive
    // coordinates is centered against the origin instead of its polygon-only bounds.
    EXPECT_FLOAT_EQ(map.GetPolygons().at(0).vertices.at(0).x, 45.0F);
}

TEST(MapBehaviorTest, LoadingSameMapTwiceReplacesWayPoints)
{
    Soldank::MapData map_data{};
    map_data.version = 11;
    map_data.description = "waypoint characterization";
    map_data.texture_name = "banana.png";
    map_data.sectors_count = 25;
    Soldank::PMSWayPoint way_point{};
    way_point.active = true;
    way_point.id = 42;
    map_data.way_points.push_back(way_point);
    Soldank::Map source_map(std::move(map_data));
    const auto writer = SaveMapToBuffer(source_map);
    const BufferFileReader reader(writer->GetData());
    Soldank::Map loaded_map;

    loaded_map.LoadMap("first-load.pms", reader);
    loaded_map.LoadMap("second-load.pms", reader);

    ASSERT_EQ(loaded_map.GetWayPoints().size(), 1);
    EXPECT_EQ(loaded_map.GetWayPoints().at(0).id, 42);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
