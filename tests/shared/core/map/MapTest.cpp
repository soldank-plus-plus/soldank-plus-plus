#include "core/math/Glm.hpp"

#include <gtest/gtest.h>

#include <string>
#include <optional>
#include <array>
#include <span>

#include "core/utility/Expected.hpp"

import Shared.Core.Map.Map;
import Shared.Core.Map.PMSConstants;
import Shared.Core.Map.PMSEnums;
import Shared.Core.Map.PMSStructs;
import Shared.Core.Math.Calc;
import Shared.Core.Data.IFileReader;

constexpr const int VERSION = 1234;
constexpr const std::string_view DESCRIPTION = "test_description";
constexpr const std::string_view TEXTURE_NAME = "test_texture";
const Soldank::PMSColor BACKGROUND_TOP_COLOR{ 1, 2, 3, 4 };
const Soldank::PMSColor BACKGROUND_BOTTOM_COLOR{ 5, 6, 7, 8 };
constexpr const int JET_COUNT = 127;
constexpr const unsigned char GRENADES_COUNT = 3;
constexpr const unsigned char MEDIKITS_COUNT = 6;
constexpr const Soldank::PMSWeatherType WEATHER_TYPE = Soldank::PMSWeatherType::Rain;
constexpr const Soldank::PMSStepType STEP_TYPE = Soldank::PMSStepType::SoftGround;
constexpr const int RANDOM_ID = 12345;
const std::array<Soldank::PMSPolygon, 2> POLYGONS{
    Soldank::PMSPolygon{
      .vertices = { Soldank::PMSVertex{ .x = 1.0F,
                                        .y = 1.0F,
                                        .z = 1.0F,
                                        .rhw = 0.5F,
                                        .color = Soldank::PMSColor(3, 5, 7, 200),
                                        .texture_s = 0.2F,
                                        .texture_t = 0.3F },
                    Soldank::PMSVertex{ .x = 5.0F,
                                        .y = 1.0F,
                                        .z = 1.0F,
                                        .rhw = 0.75F,
                                        .color = Soldank::PMSColor(4, 6, 8, 254),
                                        .texture_s = 0.3F,
                                        .texture_t = 0.6F },
                    Soldank::PMSVertex{ .x = 2.5F,
                                        .y = 5.0F,
                                        .z = 1.0F,
                                        .rhw = 0.25F,
                                        .color = Soldank::PMSColor(0, 0, 0, 0),
                                        .texture_s = 0.9F,
                                        .texture_t = 0.1F } },
      .perpendiculars = { Soldank::PMSVector{ .x = 1.0F, .y = 0.5F, .z = 0.2F },
                          Soldank::PMSVector{ .x = -1.0F, .y = -0.5F, .z = -0.2F },
                          Soldank::PMSVector{ .x = 5.9F, .y = 9.9F, .z = 0.9F } },
      .polygon_type = Soldank::PMSPolygonType::Normal,
      .bounciness = 0.5F },
    Soldank::PMSPolygon{
      .vertices = { Soldank::PMSVertex{}, Soldank::PMSVertex{}, Soldank::PMSVertex{} },
      .perpendiculars = { Soldank::PMSVector{}, Soldank::PMSVector{}, Soldank::PMSVector{} },
      .polygon_type = Soldank::PMSPolygonType::NoCollide,
      .bounciness = 0.8F }
};
constexpr const int SECTORS_SIZE = 25;
constexpr const int SECTORS_COUNT = 1;
const std::array<std::array<Soldank::PMSSector, 2 * SECTORS_COUNT + 1>, 2 * SECTORS_COUNT + 1>
  SECTORS{ {
    { Soldank::PMSSector{ .polygons = { 1 } },
      Soldank::PMSSector{ .polygons = { 1, 2 } },
      Soldank::PMSSector{ .polygons = {} } },
    { Soldank::PMSSector{ .polygons = { 1 } },
      Soldank::PMSSector{ .polygons = {} },
      Soldank::PMSSector{ .polygons = { 1 } } },
    { Soldank::PMSSector{ .polygons = {} },
      Soldank::PMSSector{ .polygons = { 1 } },
      Soldank::PMSSector{ .polygons = {} } },
  } };
constexpr const int SCENERY_INSTANCES_COUNT = 1;
const std::array<Soldank::PMSScenery, SCENERY_INSTANCES_COUNT> SCENERY_INSTANCES{
    Soldank::PMSScenery{ .active = true,
                         .style = 2,
                         .width = 3,
                         .height = 4,
                         .x = 5.0F,
                         .y = 6.0F,
                         .rotation = 7.0F,
                         .scale_x = 8.0F,
                         .scale_y = 9.0F,
                         .alpha = 10,
                         .color = Soldank::PMSColor(11, 12, 13, 14),
                         .level = 15 }
};
constexpr const int SCENERY_TYPES_COUNT = 1;
const std::array<Soldank::PMSSceneryType, SCENERY_TYPES_COUNT> SCENERY_TYPES{
    Soldank::PMSSceneryType{ .name = "test_scenery.png", .timestamp = {} }
};
constexpr const int COLLIDERS_COUNT = 1;
const std::array<Soldank::PMSCollider, COLLIDERS_COUNT> COLLIDERS{
    Soldank::PMSCollider{ .active = 1, .x = 15, .y = 30, .radius = 5.0F }
};
constexpr const int SPAWN_POINTS_COUNT = 1;
const std::array<Soldank::PMSSpawnPoint, SPAWN_POINTS_COUNT> SPAWN_POINTS{ Soldank::PMSSpawnPoint{
  .active = 1,
  .x = 5,
  .y = 10,
  .type = Soldank::PMSSpawnPointType::BravoFlag } };
constexpr const int WAY_POINTS_COUNT = 1;
const std::array<Soldank::PMSWayPoint, WAY_POINTS_COUNT> WAY_POINTS{ Soldank::PMSWayPoint{
  .active = true,
  .id = 1,
  .x = 4,
  .y = 6,
  .left = true,
  .right = true,
  .up = true,
  .down = true,
  .jet = true,
  .path = 2,
  .special_action = Soldank::PMSSpecialAction::Wait10Seconds,
  .c2 = 3,
  .c3 = 6,
  .connections_count = 2,
  .connections = { 1, 2 } } };

class MapReaderExample : public Soldank::IFileReader
{
public:
    std::expected<std::string, Soldank::FileReaderError> Read(
      const std::string& file_path,
      std::ios_base::openmode /*mode*/) const override
    {
        if (file_path != "test_map") {
            return std::unexpected(Soldank::FileReaderError::FileNotFound);
        }

        std::stringstream buffer;
        WriteToBuffer(buffer, VERSION);
        WriteToBuffer(buffer, (unsigned char)DESCRIPTION.length());
        WriteToBuffer(buffer, DESCRIPTION);
        AddFillerToBuffer(buffer, Soldank::DESCRIPTION_MAX_LENGTH - DESCRIPTION.length());
        WriteToBuffer(buffer, (unsigned char)TEXTURE_NAME.length());
        WriteToBuffer(buffer, TEXTURE_NAME);
        AddFillerToBuffer(buffer, Soldank::TEXTURE_NAME_MAX_LENGTH - TEXTURE_NAME.length());

        WriteToBuffer(buffer, BACKGROUND_TOP_COLOR);
        WriteToBuffer(buffer, BACKGROUND_BOTTOM_COLOR);
        WriteToBuffer(buffer, JET_COUNT);
        WriteToBuffer(buffer, GRENADES_COUNT);
        WriteToBuffer(buffer, MEDIKITS_COUNT);
        WriteToBuffer(buffer, WEATHER_TYPE);
        WriteToBuffer(buffer, STEP_TYPE);
        WriteToBuffer(buffer, RANDOM_ID);

        WriteToBuffer(buffer, POLYGONS);

        WriteToBuffer(buffer, SECTORS_SIZE);
        WriteToBuffer(buffer, SECTORS_COUNT);
        WriteToBuffer(buffer, SECTORS);

        WriteToBuffer(buffer, (int)SCENERY_INSTANCES_COUNT);
        for (const auto& scenery : SCENERY_INSTANCES) {
            WriteToBuffer(buffer, scenery);
        }

        WriteToBuffer(buffer, SCENERY_TYPES_COUNT);
        for (const auto& scenery_type : SCENERY_TYPES) {
            WriteToBuffer(buffer, (unsigned char)scenery_type.name.length());
            WriteToBuffer(buffer, scenery_type.name);
            AddFillerToBuffer(buffer,
                              Soldank::SCENERY_NAME_MAX_LENGTH - scenery_type.name.length());
            WriteToBuffer(buffer, scenery_type.timestamp);
        }

        WriteToBuffer(buffer, COLLIDERS_COUNT);
        for (const auto& collider : COLLIDERS) {
            WriteToBuffer(buffer, collider);
        }
        WriteToBuffer(buffer, SPAWN_POINTS_COUNT);
        for (const auto& spawn_point : SPAWN_POINTS) {
            WriteToBuffer(buffer, spawn_point);
        }
        WriteToBuffer(buffer, WAY_POINTS_COUNT);
        for (const auto& way_point : WAY_POINTS) {
            WriteToBuffer(buffer, way_point);
        }

        if (buffer.bad()) {
            return std::unexpected(Soldank::FileReaderError::BufferError);
        }

        return buffer.str();
    }

private:
    template<typename T>
    static const char* Serialize(const T& t)
    {
        // NOLINTNEXTLINE: Serializing data
        return reinterpret_cast<const char*>(&t);
    }

    static void WriteToBuffer(
      std::stringstream& buffer,
      const std::array<std::array<Soldank::PMSSector, 2 * SECTORS_COUNT + 1>,
                       2 * SECTORS_COUNT + 1>& sectors)
    {
        for (const auto& sectors_row : sectors) {
            for (const auto& sector : sectors_row) {
                buffer.write(Serialize(sector.polygons.size()), sizeof(unsigned short));
                for (unsigned short poly_id : sector.polygons) {
                    buffer.write(Serialize(poly_id), sizeof(unsigned short));
                }
            }
        }
    }

    static void WriteToBuffer(std::stringstream& buffer,
                              const std::array<Soldank::PMSPolygon, 2>& polygons)
    {
        int polygons_count = (int)polygons.size();
        buffer.write(Serialize(polygons_count), sizeof(int));
        for (const Soldank::PMSPolygon& polygon : polygons) {
            for (const Soldank::PMSVertex& vertex : polygon.vertices) {
                buffer.write(Serialize(vertex), sizeof(Soldank::PMSVertex));
            }

            for (const Soldank::PMSVector& perpendicular : polygon.perpendiculars) {
                buffer.write(Serialize(perpendicular), sizeof(Soldank::PMSVector));
            }

            buffer.write(Serialize(polygon.polygon_type), sizeof(Soldank::PMSPolygonType));
        }
    }

    static void WriteToBuffer(std::stringstream& buffer, std::string_view variable_to_write)
    {
        buffer.write(variable_to_write.data(),
                     (std::streamsize)sizeof(char) * (std::streamsize)variable_to_write.size());
    }

    static void WriteToBuffer(std::stringstream& buffer, const std::string& variable_to_write)
    {
        buffer.write(variable_to_write.data(),
                     (std::streamsize)sizeof(char) * (std::streamsize)variable_to_write.size());
    }

    template<typename TVar>
    static void WriteToBuffer(std::stringstream& buffer, TVar variable_to_write)
    {
        buffer.write(Serialize(variable_to_write), sizeof(variable_to_write));
    }

    static void AddFillerToBuffer(std::stringstream& buffer, unsigned int bytes_count)
    {
        char filler = 0;
        for (unsigned int i = 0; i < bytes_count; i++) {
            buffer.write(&filler, sizeof(filler));
        }
    }
};

void ComparePMSColors(const Soldank::PMSColor& color1, const Soldank::PMSColor& color2)
{
    ASSERT_EQ(color1.alpha, color2.alpha);
    ASSERT_EQ(color1.red, color2.red);
    ASSERT_EQ(color1.green, color2.green);
    ASSERT_EQ(color1.blue, color2.blue);
}

void ComparePMSVertices(const Soldank::PMSVertex& vertex1, const Soldank::PMSVertex& vertex2)
{
    ASSERT_FLOAT_EQ(vertex1.x, vertex2.x);
    ASSERT_FLOAT_EQ(vertex1.y, vertex2.y);
    ASSERT_FLOAT_EQ(vertex1.z, vertex2.z);
    ASSERT_FLOAT_EQ(vertex1.rhw, vertex2.rhw);
    ASSERT_FLOAT_EQ(vertex1.texture_s, vertex2.texture_s);
    ASSERT_FLOAT_EQ(vertex1.texture_t, vertex2.texture_t);
    ComparePMSColors(vertex1.color, vertex2.color);
}

void ComparePMSVectors(const Soldank::PMSVector& vector1, const Soldank::PMSVector& vector2)
{
    ASSERT_FLOAT_EQ(vector1.x, vector2.x);
    ASSERT_FLOAT_EQ(vector1.y, vector2.y);
    ASSERT_FLOAT_EQ(vector1.z, vector2.z);
}

void ComparePMSPolygons(const Soldank::PMSPolygon& actual_polygon,
                        const Soldank::PMSPolygon& expected_polygon)
{
    for (unsigned int j = 0; j < 3; j++) {
        ComparePMSVertices(actual_polygon.vertices.at(j), expected_polygon.vertices.at(j));
    }
    for (unsigned int j = 0; j < 3; j++) {
        Soldank::PMSVector expected_perpendiculars = expected_polygon.perpendiculars.at(j);
        expected_perpendiculars.x =
          Soldank::Calc::Vec2Normalize(glm::vec2(expected_polygon.perpendiculars.at(j).x,
                                                 expected_polygon.perpendiculars.at(j).y))
            .x;
        expected_perpendiculars.y =
          Soldank::Calc::Vec2Normalize(glm::vec2(expected_polygon.perpendiculars.at(j).x,
                                                 expected_polygon.perpendiculars.at(j).y))
            .y;
        ComparePMSVectors(actual_polygon.perpendiculars.at(j), expected_perpendiculars);
    }
    ASSERT_EQ(actual_polygon.polygon_type, expected_polygon.polygon_type);
}

void CompareSceneryInstances(const Soldank::PMSScenery& actual_scenery,
                             const Soldank::PMSScenery& expected_scenery)
{
    ASSERT_EQ(actual_scenery.active, expected_scenery.active);
    ASSERT_EQ(actual_scenery.style, expected_scenery.style);
    ASSERT_EQ(actual_scenery.width, expected_scenery.width);
    ASSERT_EQ(actual_scenery.height, expected_scenery.height);
    ASSERT_FLOAT_EQ(actual_scenery.x, expected_scenery.x);
    ASSERT_FLOAT_EQ(actual_scenery.y, expected_scenery.y);
    ASSERT_EQ(actual_scenery.rotation, expected_scenery.rotation);
    ASSERT_EQ(actual_scenery.scale_x, expected_scenery.scale_x);
    ASSERT_EQ(actual_scenery.scale_y, expected_scenery.scale_y);
    ASSERT_EQ(actual_scenery.alpha, expected_scenery.alpha);
    ComparePMSColors(actual_scenery.color, expected_scenery.color);
    ASSERT_EQ(actual_scenery.level, expected_scenery.level);
}

void CompareSceneryTypes(const Soldank::PMSSceneryType& actual_scenery_type,
                         const Soldank::PMSSceneryType& expected_scenery_type)
{
    ASSERT_EQ(actual_scenery_type.name, expected_scenery_type.name);
    ASSERT_EQ(actual_scenery_type.timestamp.date.day, expected_scenery_type.timestamp.date.day);
    ASSERT_EQ(actual_scenery_type.timestamp.date.month, expected_scenery_type.timestamp.date.month);
    ASSERT_EQ(actual_scenery_type.timestamp.date.year, expected_scenery_type.timestamp.date.year);
    ASSERT_EQ(actual_scenery_type.timestamp.time.hour, expected_scenery_type.timestamp.time.hour);
    ASSERT_EQ(actual_scenery_type.timestamp.time.minute,
              expected_scenery_type.timestamp.time.minute);
    ASSERT_EQ(actual_scenery_type.timestamp.time.second,
              expected_scenery_type.timestamp.time.second);
}

void CompareColliders(const Soldank::PMSCollider& actual_collider,
                      const Soldank::PMSCollider& expected_collider)
{
    ASSERT_EQ(actual_collider.active, expected_collider.active);
    ASSERT_FLOAT_EQ(actual_collider.x, expected_collider.x);
    ASSERT_FLOAT_EQ(actual_collider.y, expected_collider.y);
    ASSERT_FLOAT_EQ(actual_collider.radius, expected_collider.radius);
}

void CompareSpawnPoints(const Soldank::PMSSpawnPoint& actual_spawn_point,
                        const Soldank::PMSSpawnPoint& expected_spawn_point)
{
    ASSERT_EQ(actual_spawn_point.active, expected_spawn_point.active);
    ASSERT_EQ(actual_spawn_point.x, expected_spawn_point.x);
    ASSERT_EQ(actual_spawn_point.y, expected_spawn_point.y);
    ASSERT_EQ(actual_spawn_point.type, expected_spawn_point.type);
}

void CompareWayPoints(const Soldank::PMSWayPoint& actual_way_point,
                      const Soldank::PMSWayPoint& expected_way_point)
{
    ASSERT_EQ(actual_way_point.active, expected_way_point.active);
    ASSERT_EQ(actual_way_point.id, expected_way_point.id);
    ASSERT_EQ(actual_way_point.x, expected_way_point.x);
    ASSERT_EQ(actual_way_point.y, expected_way_point.y);
    ASSERT_EQ(actual_way_point.left, expected_way_point.left);
    ASSERT_EQ(actual_way_point.right, expected_way_point.right);
    ASSERT_EQ(actual_way_point.up, expected_way_point.up);
    ASSERT_EQ(actual_way_point.down, expected_way_point.down);
    ASSERT_EQ(actual_way_point.jet, expected_way_point.jet);
    ASSERT_EQ(actual_way_point.path, expected_way_point.path);
    ASSERT_EQ(actual_way_point.special_action, expected_way_point.special_action);
    ASSERT_EQ(actual_way_point.c2, expected_way_point.c2);
    ASSERT_EQ(actual_way_point.c3, expected_way_point.c3);
    ASSERT_EQ(actual_way_point.connections_count, expected_way_point.connections_count);
    for (unsigned i = 0; i < actual_way_point.connections_count; i++) {
        ASSERT_EQ(actual_way_point.connections.at(i), expected_way_point.connections.at(i));
    }
}

void CheckLoadedMap(const Soldank::Map& map)
{
    ASSERT_EQ(map.GetVersion(), VERSION);
    ASSERT_EQ(map.GetDescription(), DESCRIPTION);
    ASSERT_EQ(map.GetTextureName(), TEXTURE_NAME);
    ComparePMSColors(map.GetBackgroundTopColor(), BACKGROUND_TOP_COLOR);
    ComparePMSColors(map.GetBackgroundBottomColor(), BACKGROUND_BOTTOM_COLOR);
    ASSERT_EQ(map.GetJetCount(), JET_COUNT);
    ASSERT_EQ(map.GetGrenadesCount(), GRENADES_COUNT);
    ASSERT_EQ(map.GetMedikitsCount(), MEDIKITS_COUNT);
    ASSERT_EQ(map.GetWeatherType(), WEATHER_TYPE);
    ASSERT_EQ(map.GetStepType(), STEP_TYPE);
    ASSERT_EQ(map.GetPolygons().size(), POLYGONS.size());
    ASSERT_EQ(map.GetPolygonsCount(), POLYGONS.size());
    for (unsigned int i = 0; i < map.GetPolygonsCount(); i++) {
        ComparePMSPolygons(map.GetPolygons().at(i), POLYGONS.at(i));
    }
    ASSERT_EQ(map.GetSectorsSize(), SECTORS_SIZE);
    ASSERT_EQ(map.GetSectorsCount(), SECTORS_COUNT);
    for (unsigned int y = 0; y < SECTORS.size(); y++) {
        const auto& sectors_row = SECTORS.at(y);
        for (unsigned int x = 0; x < sectors_row.size(); x++) {
            const auto& actual_polygons = map.GetSector(y, x).polygons;
            const auto& sector = SECTORS.at(y).at(x);
            ASSERT_EQ(actual_polygons.size(), sector.polygons.size());
            for (unsigned short i = 0; i < (unsigned short)sector.polygons.size(); i++) {
                ASSERT_EQ(actual_polygons[i], sector.polygons[i]);
            }
        }
    }
    ASSERT_EQ(map.GetSceneryInstances().size(), SCENERY_INSTANCES_COUNT);
    for (unsigned int i = 0; i < map.GetSceneryInstances().size(); i++) {
        CompareSceneryInstances(map.GetSceneryInstances().at(i), SCENERY_INSTANCES.at(i));
    }
    ASSERT_EQ(map.GetSceneryTypes().size(), SCENERY_TYPES_COUNT);
    for (unsigned int i = 0; i < map.GetSceneryTypes().size(); i++) {
        CompareSceneryTypes(map.GetSceneryTypes().at(i), SCENERY_TYPES.at(i));
    }
    ASSERT_EQ(map.GetColliders().size(), COLLIDERS_COUNT);
    for (unsigned int i = 0; i < map.GetColliders().size(); i++) {
        CompareColliders(map.GetColliders().at(i), COLLIDERS.at(i));
    }
    ASSERT_EQ(map.GetSpawnPoints().size(), SPAWN_POINTS_COUNT);
    for (unsigned int i = 0; i < map.GetSpawnPoints().size(); i++) {
        CompareSpawnPoints(map.GetSpawnPoints().at(i), SPAWN_POINTS.at(i));
    }
    ASSERT_EQ(map.GetWayPoints().size(), WAY_POINTS_COUNT);
    for (unsigned int i = 0; i < map.GetWayPoints().size(); i++) {
        CompareWayPoints(map.GetWayPoints().at(i), WAY_POINTS.at(i));
    }
}

TEST(MapTests, TestMapNotFound)
{
    MapReaderExample map_reader;
    Soldank::Map map;
    map.LoadMap("not_existing_map", map_reader);
    ASSERT_EQ(map.GetPolygons().size(), 0); // TODO: should return an error
}

TEST(MapTests, TestMapLoadedCorrectly)
{
    MapReaderExample map_reader;
    Soldank::Map map;
    map.LoadMap("test_map", map_reader);
    CheckLoadedMap(map);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
