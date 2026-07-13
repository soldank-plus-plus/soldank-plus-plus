#include "core/math/Glm.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <ios>
#include <memory>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <tuple>
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
class StringFileReader final : public Soldank::IFileReader
{
public:
    explicit StringFileReader(std::string data)
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

class RecordingFileWriter : public Soldank::IFileWriter
{
public:
    explicit RecordingFileWriter(
      bool fail_append = false,
      Soldank::FileWriterError write_error = Soldank::FileWriterError::NoError)
        : fail_append_(fail_append)
        , write_error_(write_error)
    {
    }

    Soldank::FileWriterError AppendData(const char* data, std::streamsize data_size) override
    {
        if (fail_append_) {
            return Soldank::FileWriterError::BufferError;
        }
        data_.append(data, static_cast<std::size_t>(data_size));
        return Soldank::FileWriterError::NoError;
    }

    Soldank::FileWriterError Write(const std::filesystem::path& /*file_path*/,
                                   std::ios_base::openmode /*mode*/) const override
    {
        write_called_ = true;
        return write_error_;
    }

    const std::string& GetData() const { return data_; }
    bool WasWriteCalled() const { return write_called_; }

private:
    bool fail_append_;
    Soldank::FileWriterError write_error_;
    std::string data_;
    mutable bool write_called_{};
};

Soldank::PMSPolygon CreatePolygon(
  float x,
  float y,
  Soldank::PMSPolygonType polygon_type = Soldank::PMSPolygonType::Normal)
{
    Soldank::PMSPolygon polygon{};
    polygon.vertices.at(0).x = x;
    polygon.vertices.at(0).y = y;
    polygon.vertices.at(0).z = 1.0F;
    polygon.vertices.at(1).x = x + 20.0F;
    polygon.vertices.at(1).y = y;
    polygon.vertices.at(1).z = 1.0F;
    polygon.vertices.at(2).x = x;
    polygon.vertices.at(2).y = y + 20.0F;
    polygon.vertices.at(2).z = 1.0F;
    polygon.polygon_type = polygon_type;
    polygon.bounciness = 1.0F;
    return polygon;
}

Soldank::PMSScenery CreateScenery(float x, float y)
{
    Soldank::PMSScenery scenery{};
    scenery.active = true;
    scenery.style = 1;
    scenery.width = 32;
    scenery.height = 48;
    scenery.x = x;
    scenery.y = y;
    scenery.rotation = 0.25F;
    scenery.scale_x = 1.5F;
    scenery.scale_y = 0.75F;
    scenery.alpha = 190;
    scenery.color = Soldank::PMSColor(10, 20, 30, 190);
    scenery.level = 2;
    return scenery;
}

Soldank::PMSSpawnPoint CreateSpawnPoint(int x, int y)
{
    return Soldank::PMSSpawnPoint{
        .active = 1, .x = x, .y = y, .type = Soldank::PMSSpawnPointType::Bravo
    };
}

std::shared_ptr<RecordingFileWriter> Serialize(const Soldank::Map& map)
{
    auto writer = std::make_shared<RecordingFileWriter>();
    map.SaveMap("robustness.pms", writer);
    return writer;
}

template<typename Value>
void WriteAt(std::string& data, std::size_t offset, const Value& value)
{
    ASSERT_LE(offset + sizeof(Value), data.size());
    std::memcpy(data.data() + offset, &value, sizeof(Value));
}

template<typename Value>
Value ReadAt(const std::string& data, std::size_t offset)
{
    EXPECT_LE(offset + sizeof(Value), data.size());
    Value value{};
    std::memcpy(&value, data.data() + offset, sizeof(Value));
    return value;
}

constexpr std::size_t PolygonCountOffset()
{
    return sizeof(int) + 1 + Soldank::DESCRIPTION_MAX_LENGTH + 1 +
           Soldank::TEXTURE_NAME_MAX_LENGTH + 2 * sizeof(Soldank::PMSColor) + sizeof(int) +
           4 * sizeof(unsigned char) + sizeof(int);
}

std::size_t SectorDataOffset(const std::string& data)
{
    const auto polygon_count = ReadAt<std::uint32_t>(data, PolygonCountOffset());
    constexpr std::size_t POLYGON_RECORD_SIZE = 3 * sizeof(Soldank::PMSVertex) +
                                                3 * sizeof(Soldank::PMSVector) +
                                                sizeof(Soldank::PMSPolygonType);
    return PolygonCountOffset() + sizeof(std::uint32_t) + polygon_count * POLYGON_RECORD_SIZE;
}

std::size_t FindFirstSectorPolygonIdOffset(const std::string& data)
{
    std::size_t offset = SectorDataOffset(data) + 2 * sizeof(int);
    for (int sector = 0; sector < Soldank::SECTORS_COUNT * Soldank::SECTORS_COUNT; ++sector) {
        const auto polygon_count = ReadAt<unsigned short>(data, offset);
        offset += sizeof(unsigned short);
        if (polygon_count > 0) {
            return offset;
        }
        offset += polygon_count * sizeof(unsigned short);
    }
    throw std::runtime_error("Serialized test map did not index its polygon");
}

std::size_t SceneryCountOffset(const std::string& data)
{
    std::size_t offset = SectorDataOffset(data) + 2 * sizeof(int);
    for (int sector = 0; sector < Soldank::SECTORS_COUNT * Soldank::SECTORS_COUNT; ++sector) {
        const auto polygon_count = ReadAt<unsigned short>(data, offset);
        offset += sizeof(unsigned short) + polygon_count * sizeof(unsigned short);
    }
    return offset;
}

std::set<std::pair<unsigned int, unsigned int>> FindPolygonSectors(const Soldank::Map& map,
                                                                   unsigned short polygon_id)
{
    std::set<std::pair<unsigned int, unsigned int>> sectors;
    const auto size = static_cast<unsigned int>(map.GetSectorsCount() * 2 + 1);
    for (unsigned int x = 0; x < size; ++x) {
        for (unsigned int y = 0; y < size; ++y) {
            if (std::ranges::contains(map.GetSector(x, y).polygons, polygon_id)) {
                sectors.emplace(x, y);
            }
        }
    }
    return sectors;
}

bool RayCastHitsPolygon(Soldank::PMSPolygonType polygon_type,
                        bool player,
                        bool flag,
                        bool bullet,
                        std::uint8_t team_id)
{
    Soldank::Map map;
    map.CreateEmptyMap();
    map.AddNewPolygon(CreatePolygon(-10.0F, -10.0F, polygon_type));
    map.NormalizeCoordinatesForRuntime();
    map.GenerateSectors();

    float distance = 0.0F;
    return map.RayCast(
      { -5.0F, -5.0F }, { 20.0F, 20.0F }, distance, 100.0F, player, flag, bullet, false, team_id);
}

Soldank::Map CreateCollisionMap(Soldank::PMSPolygonType polygon_type)
{
    Soldank::Map map;
    map.CreateEmptyMap();
    map.AddNewPolygon(CreatePolygon(-10.0F, -10.0F, polygon_type));
    map.NormalizeCoordinatesForRuntime();
    map.GenerateSectors();
    return map;
}
} // namespace

TEST(MapRobustnessTest, SaveLoadRoundTripPreservesMetadataAndEveryCollection)
{
    Soldank::MapData map_data;
    map_data.description = "round trip";
    map_data.texture_name = "texture.png";
    map_data.background_top_color = Soldank::PMSColor(1, 2, 3, 4);
    map_data.background_bottom_color = Soldank::PMSColor(5, 6, 7, 8);
    map_data.jet_count = 123;
    map_data.grenades_count = 4;
    map_data.medikits_count = 5;
    map_data.weather_type = Soldank::PMSWeatherType::Snow;
    map_data.step_type = Soldank::PMSStepType::SoftGround;
    map_data.random_id = 9876;
    map_data.polygons.push_back(CreatePolygon(-120.0F, -20.0F));
    map_data.polygons.push_back(CreatePolygon(100.0F, 0.0F, Soldank::PMSPolygonType::Bouncy));
    map_data.polygons.back().bounciness = 2.5F;
    map_data.scenery_instances.push_back(CreateScenery(25.0F, -30.0F));
    map_data.scenery_types.push_back({ .name = "tree.png", .timestamp = {} });
    map_data.colliders.push_back({ .active = 1, .x = -15.0F, .y = 30.0F, .radius = 7.0F });
    map_data.spawn_points.push_back(CreateSpawnPoint(12, -18));
    Soldank::PMSWayPoint way_point{};
    way_point.active = true;
    way_point.id = 42;
    way_point.x = -8;
    way_point.y = 9;
    way_point.left = true;
    way_point.connections_count = 1;
    way_point.connections.at(0) = 7;
    map_data.way_points.push_back(way_point);
    const Soldank::Map source(std::move(map_data));

    const auto writer = Serialize(source);
    Soldank::Map loaded;
    loaded.LoadMap("round-trip.pms", StringFileReader(writer->GetData()));

    EXPECT_EQ(loaded.GetDescription(), "round trip");
    EXPECT_EQ(loaded.GetTextureName(), "texture.png");
    EXPECT_EQ(loaded.GetJetCount(), 123);
    EXPECT_EQ(loaded.GetGrenadesCount(), 4);
    EXPECT_EQ(loaded.GetMedikitsCount(), 5);
    EXPECT_EQ(loaded.GetWeatherType(), Soldank::PMSWeatherType::Snow);
    EXPECT_EQ(loaded.GetStepType(), Soldank::PMSStepType::SoftGround);
    EXPECT_EQ(loaded.GetBackgroundTopColor().red, 1);
    EXPECT_EQ(loaded.GetBackgroundBottomColor().blue, 7);
    ASSERT_EQ(loaded.GetPolygons().size(), 2);
    EXPECT_EQ(loaded.GetPolygons().at(1).polygon_type, Soldank::PMSPolygonType::Bouncy);
    EXPECT_NEAR(loaded.GetPolygons().at(1).bounciness, 2.5F, 0.0001F);
    ASSERT_EQ(loaded.GetSceneryInstances().size(), 1);
    EXPECT_FLOAT_EQ(loaded.GetSceneryInstances().at(0).x, 25.0F);
    ASSERT_EQ(loaded.GetSceneryTypes().size(), 1);
    EXPECT_EQ(loaded.GetSceneryTypes().at(0).name, "tree.png");
    ASSERT_EQ(loaded.GetColliders().size(), 1);
    EXPECT_FLOAT_EQ(loaded.GetColliders().at(0).radius, 7.0F);
    ASSERT_EQ(loaded.GetSpawnPoints().size(), 1);
    EXPECT_EQ(loaded.GetSpawnPoints().at(0).x, 12);
    ASSERT_EQ(loaded.GetWayPoints().size(), 1);
    EXPECT_EQ(loaded.GetWayPoints().at(0).id, 42);
    EXPECT_EQ(loaded.GetWayPoints().at(0).connections.at(0), 7);
    EXPECT_EQ(Serialize(loaded)->GetData(), writer->GetData());
}

TEST(MapRobustnessTest, InvalidPolygonCountDoesNotReplaceExistingMap)
{
    Soldank::Map source;
    source.CreateEmptyMap();
    auto data = Serialize(source)->GetData();
    WriteAt(data, PolygonCountOffset(), Soldank::MAX_POLYGONS_COUNT + 1);
    Soldank::Map map;
    map.CreateEmptyMap();
    map.AddNewSpawnPoint(CreateSpawnPoint(10, 20));

    EXPECT_THROW(map.LoadMap("bad-count.pms", StringFileReader(std::move(data))),
                 std::runtime_error);
    ASSERT_EQ(map.GetSpawnPoints().size(), 1);
    EXPECT_EQ(map.GetSpawnPoints().at(0).x, 10);
}

TEST(MapRobustnessTest, InvalidSectorDimensionsAreRejected)
{
    Soldank::Map source;
    source.CreateEmptyMap();
    auto data = Serialize(source)->GetData();
    WriteAt(data, SectorDataOffset(data), 0);

    Soldank::Map loaded;
    EXPECT_THROW(loaded.LoadMap("bad-sectors.pms", StringFileReader(std::move(data))),
                 std::runtime_error);
}

TEST(MapRobustnessTest, InvalidSectorPolygonReferenceIsRejected)
{
    Soldank::Map source;
    source.CreateEmptyMap();
    source.AddNewPolygon(CreatePolygon(-10.0F, -10.0F));
    auto data = Serialize(source)->GetData();
    WriteAt(data, FindFirstSectorPolygonIdOffset(data), static_cast<unsigned short>(0));

    Soldank::Map loaded;
    EXPECT_THROW(loaded.LoadMap("bad-sector-reference.pms", StringFileReader(std::move(data))),
                 std::runtime_error);
}

TEST(MapRobustnessTest, OversizedSceneryCollectionIsRejected)
{
    Soldank::Map source;
    source.CreateEmptyMap();
    auto data = Serialize(source)->GetData();
    WriteAt(data, SceneryCountOffset(data), Soldank::MAX_SCENERIES_COUNT + 1);

    Soldank::Map loaded;
    EXPECT_THROW(loaded.LoadMap("bad-scenery-count.pms", StringFileReader(std::move(data))),
                 std::runtime_error);
}

TEST(MapRobustnessTest, AppendFailureThrowsBeforeFinalWrite)
{
    Soldank::Map map;
    map.CreateEmptyMap();
    auto writer = std::make_shared<RecordingFileWriter>(true);

    EXPECT_THROW(map.SaveMap("append-failure.pms", writer), std::runtime_error);
    EXPECT_FALSE(writer->WasWriteCalled());
}

TEST(MapRobustnessTest, FinalWriteFailureIsAttemptedWithoutThrowing)
{
    Soldank::Map map;
    map.CreateEmptyMap();
    auto writer =
      std::make_shared<RecordingFileWriter>(false, Soldank::FileWriterError::FileNotOpen);

    EXPECT_NO_THROW(map.SaveMap("write-failure.pms", writer));
    EXPECT_TRUE(writer->WasWriteCalled());
}

TEST(MapRobustnessTest, SectorIndexIsRebuiltAfterPolygonMovement)
{
    Soldank::Map map;
    map.CreateEmptyMap();
    map.AddNewPolygon(CreatePolygon(-500.0F, -10.0F));
    map.AddNewPolygon(CreatePolygon(-100.0F, -10.0F));
    map.AddNewPolygon(CreatePolygon(480.0F, -10.0F));
    map.GenerateSectors();
    const auto sectors_before = FindPolygonSectors(map, 2);

    map.MovePolygonVerticesById({ { { 1, 0 }, { 100.0F, -10.0F } },
                                  { { 1, 1 }, { 120.0F, -10.0F } },
                                  { { 1, 2 }, { 100.0F, 10.0F } } });
    map.GenerateSectors();
    const auto sectors_after = FindPolygonSectors(map, 2);

    EXPECT_FALSE(sectors_before.empty());
    EXPECT_FALSE(sectors_after.empty());
    EXPECT_NE(sectors_before, sectors_after);
}

TEST(MapRobustnessTest, RuntimeNormalizationIsIdempotent)
{
    Soldank::Map map;
    map.CreateEmptyMap();
    map.AddNewPolygon(CreatePolygon(100.0F, 100.0F));

    const auto first_offset = map.NormalizeCoordinatesForRuntime();
    const auto vertex_after_first_normalization = map.GetPolygons().at(0).vertices.at(0);
    const auto second_offset = map.NormalizeCoordinatesForRuntime();
    const auto vertex_after_second_normalization = map.GetPolygons().at(0).vertices.at(0);

    EXPECT_NE(first_offset, glm::vec2(0.0F, 0.0F));
    EXPECT_EQ(second_offset, glm::vec2(0.0F, 0.0F));
    EXPECT_FLOAT_EQ(vertex_after_second_normalization.x, vertex_after_first_normalization.x);
    EXPECT_FLOAT_EQ(vertex_after_second_normalization.y, vertex_after_first_normalization.y);
}

TEST(MapRobustnessTest, PolygonNormalizationHandlesWindingAndDegenerateEdges)
{
    Soldank::Map map;
    map.CreateEmptyMap();
    auto counter_clockwise = CreatePolygon(0.0F, 0.0F);
    std::swap(counter_clockwise.vertices.at(1), counter_clockwise.vertices.at(2));
    map.AddNewPolygon(counter_clockwise);
    auto degenerate = CreatePolygon(30.0F, 0.0F);
    degenerate.vertices.at(1) = degenerate.vertices.at(0);
    map.AddNewPolygon(degenerate);

    auto normalized_polygon = map.GetPolygons().at(0);
    EXPECT_TRUE(normalized_polygon.AreVerticesClockwise());
    for (const auto& perpendicular : map.GetPolygons().at(1).perpendiculars) {
        EXPECT_TRUE(std::isfinite(perpendicular.x));
        EXPECT_TRUE(std::isfinite(perpendicular.y));
        EXPECT_TRUE(std::isfinite(perpendicular.z));
    }
}

TEST(MapRobustnessTest, CollisionPolicyCoversEveryTeamForPlayersAndBullets)
{
    constexpr std::array PLAYER_TYPES{
        Soldank::PMSPolygonType::AlphaPlayers,
        Soldank::PMSPolygonType::BravoPlayers,
        Soldank::PMSPolygonType::CharliePlayers,
        Soldank::PMSPolygonType::DeltaPlayers,
    };
    constexpr std::array BULLET_TYPES{
        Soldank::PMSPolygonType::AlphaBullets,
        Soldank::PMSPolygonType::BravoBullets,
        Soldank::PMSPolygonType::CharlieBullets,
        Soldank::PMSPolygonType::DeltaBullets,
    };

    for (std::uint8_t team = 1; team <= 4; ++team) {
        for (std::uint8_t tested_team = 1; tested_team <= 4; ++tested_team) {
            EXPECT_EQ(
              RayCastHitsPolygon(PLAYER_TYPES.at(team - 1), true, false, false, tested_team),
              team == tested_team);
            EXPECT_EQ(
              RayCastHitsPolygon(BULLET_TYPES.at(team - 1), false, false, true, tested_team),
              team == tested_team);
        }
    }
}

TEST(MapRobustnessTest, CollisionPoliciesDocumentNonFlaggerAndOnlyPlayerDifferences)
{
    EXPECT_FALSE(
      RayCastHitsPolygon(Soldank::PMSPolygonType::NonFlaggerCollides, true, false, false, 0));
    EXPECT_TRUE(
      RayCastHitsPolygon(Soldank::PMSPolygonType::OnlyPlayersCollide, true, false, false, 0));

    auto map = CreateCollisionMap(Soldank::PMSPolygonType::OnlyPlayersCollide);
    glm::vec2 perpendicular{};
    EXPECT_FALSE(map.CollisionTest({ -5.0F, -5.0F }, perpendicular));
}

TEST(MapRobustnessTest, ColliderRayCastBranchCurrentlyDoesNotReportCollision)
{
    Soldank::MapData map_data;
    map_data.sectors_size = 4;
    map_data.sectors_count = 25;
    map_data.sectors_poly = std::vector<std::vector<Soldank::PMSSector>>(
      Soldank::SECTORS_COUNT, std::vector<Soldank::PMSSector>(Soldank::SECTORS_COUNT));
    map_data.colliders.push_back({ .active = 1, .x = 0.0F, .y = 0.0F, .radius = 5.0F });
    Soldank::Map map(std::move(map_data));
    float distance = 0.0F;

    EXPECT_FALSE(map.RayCast(
      { -10.0F, -10.0F }, { 10.0F, 10.0F }, distance, 100.0F, false, false, true, true));
}

TEST(MapRobustnessTest, BatchMutationEventsHaveStableOrderingAndPayloads)
{
    Soldank::Map map;
    map.CreateEmptyMap();
    std::vector<std::string> events;
    map.GetMapChangeEvents().changed_background_color.AddObserver(
      [&events](const Soldank::PMSColor&, const Soldank::PMSColor&, std::span<const float, 4>) {
          events.emplace_back("boundaries");
      });
    map.GetMapChangeEvents().added_new_polygons.AddObserver(
      [&events](const std::vector<Soldank::PMSPolygon> inserted,
                const std::vector<Soldank::PMSPolygon>& all) {
          EXPECT_EQ(inserted.size(), 2);
          EXPECT_EQ(all.size(), 2);
          events.emplace_back("polygons");
      });
    map.GetMapChangeEvents().added_new_scenery_type.AddObserver(
      [&events](const Soldank::PMSSceneryType&) { events.emplace_back("scenery-type"); });
    map.GetMapChangeEvents().added_sceneries.AddObserver(
      [&events](const std::vector<Soldank::PMSScenery>& sceneries) {
          EXPECT_EQ(sceneries.size(), 1);
          events.emplace_back("sceneries");
      });

    auto first_polygon = CreatePolygon(0.0F, 0.0F);
    first_polygon.id = 0;
    auto second_polygon = CreatePolygon(30.0F, 0.0F);
    second_polygon.id = 1;
    map.AddPolygons({ first_polygon, second_polygon });
    map.AddSceneries({ { 0, { CreateScenery(0.0F, 0.0F), "tree.png" } } });

    EXPECT_EQ(events,
              (std::vector<std::string>{ "boundaries", "polygons", "scenery-type", "sceneries" }));
}

TEST(MapRobustnessTest, BatchRemovalEventsHaveStableOrderingAndPayloads)
{
    Soldank::Map map;
    map.CreateEmptyMap();
    map.AddNewPolygon(CreatePolygon(0.0F, 0.0F));
    map.AddNewPolygon(CreatePolygon(30.0F, 0.0F));
    map.AddNewSpawnPoint(CreateSpawnPoint(10, 20));
    map.AddNewSpawnPoint(CreateSpawnPoint(30, 40));
    map.AddNewScenery(CreateScenery(0.0F, 0.0F), "tree.png");
    map.AddNewScenery(CreateScenery(30.0F, 0.0F), "bush.png");

    std::vector<std::string> events;
    map.GetMapChangeEvents().changed_background_color.AddObserver(
      [&events](const Soldank::PMSColor&, const Soldank::PMSColor&, std::span<const float, 4>) {
          events.emplace_back("boundaries");
      });
    map.GetMapChangeEvents().removed_polygons.AddObserver(
      [&events](const std::vector<Soldank::PMSPolygon> removed,
                const std::vector<Soldank::PMSPolygon>& remaining) {
          EXPECT_EQ(removed.size(), 1);
          EXPECT_EQ(remaining.size(), 1);
          events.emplace_back("polygons");
      });
    map.GetMapChangeEvents().removed_spawn_points.AddObserver(
      [&events](const std::vector<Soldank::PMSSpawnPoint>& remaining) {
          EXPECT_EQ(remaining.size(), 1);
          events.emplace_back("spawn-points");
      });
    map.GetMapChangeEvents().removed_scenery_types.AddObserver(
      [&events](const std::vector<std::pair<unsigned short, Soldank::PMSSceneryType>>& removed) {
          ASSERT_EQ(removed.size(), 1);
          EXPECT_EQ(removed.at(0).first, 1);
          EXPECT_EQ(removed.at(0).second.name, "tree.png");
          events.emplace_back("scenery-types");
      });
    map.GetMapChangeEvents().removed_sceneries.AddObserver(
      [&events](const std::vector<Soldank::PMSScenery>& remaining) {
          ASSERT_EQ(remaining.size(), 1);
          EXPECT_EQ(remaining.at(0).style, 1);
          events.emplace_back("sceneries");
      });

    map.RemovePolygonsById({ 0 });
    map.RemoveSpawnPointsById({ 0 });
    map.RemoveSceneriesById({ 0 });

    EXPECT_EQ(events,
              (std::vector<std::string>{
                "boundaries", "polygons", "spawn-points", "scenery-types", "sceneries" }));
}

TEST(MapRobustnessTest, MoveAndReplaceOperationsUpdateEveryEditableCollection)
{
    Soldank::Map map;
    map.CreateEmptyMap();
    map.AddNewPolygon(CreatePolygon(0.0F, 0.0F));
    map.AddNewSpawnPoint(CreateSpawnPoint(10, 20));
    map.AddNewScenery(CreateScenery(5.0F, 6.0F), "tree.png");

    auto replacement_polygon = CreatePolygon(50.0F, 60.0F);
    map.SetPolygonsById({ { 0, replacement_polygon } });
    map.MoveSpawnPointsById({ { 0, { 30, 40 } } });
    auto replacement_spawn = CreateSpawnPoint(70, 80);
    map.SetSpawnPointsById({ { 0, replacement_spawn } });
    map.MoveSceneriesById({ { 0, { 15.0F, 16.0F } } });
    auto replacement_scenery = CreateScenery(25.0F, 26.0F);
    replacement_scenery.style = 1;
    map.SetSceneriesById({ { 0, replacement_scenery } });

    EXPECT_FLOAT_EQ(map.GetPolygons().at(0).vertices.at(0).x, 50.0F);
    EXPECT_FLOAT_EQ(map.GetPolygons().at(0).vertices.at(0).y, 60.0F);
    EXPECT_EQ(map.GetSpawnPoints().at(0).x, 70);
    EXPECT_EQ(map.GetSpawnPoints().at(0).y, 80);
    EXPECT_FLOAT_EQ(map.GetSceneryInstances().at(0).x, 25.0F);
    EXPECT_FLOAT_EQ(map.GetSceneryInstances().at(0).y, 26.0F);
}

TEST(MapRobustnessTest, GeometryBoundaryCasesHaveExplicitContracts)
{
    const auto polygon = CreatePolygon(0.0F, 0.0F);
    EXPECT_FALSE(Soldank::Map::PointInPoly({ 0.0F, 0.0F }, polygon));
    EXPECT_FALSE(Soldank::Map::PointInPoly({ 5.0F, 0.0F }, polygon));

    glm::vec2 intersection{};
    EXPECT_FALSE(Soldank::Map::LineInPoly({ -5.0F, 0.0F }, { 25.0F, 0.0F }, polygon, intersection));
    EXPECT_FALSE(Soldank::Map::LineInPoly({ -5.0F, -5.0F }, { 0.0F, 0.0F }, polygon, intersection));

    const auto scenery = CreateScenery(0.0F, 0.0F);
    EXPECT_TRUE(Soldank::Map::PointInScenery({ 0.0F, 0.0F }, scenery));
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
