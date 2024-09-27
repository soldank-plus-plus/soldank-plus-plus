#ifndef __MAP_HPP__
#define __MAP_HPP__

#include "core/map/PMSConstants.hpp"
#include "core/map/PMSEnums.hpp"
#include "core/map/PMSStructs.hpp"

#include "core/math/Glm.hpp"
#include "core/data/IFileReader.hpp"
#include "core/data/FileReader.hpp"

#include <optional>
#include <utility>
#include <vector>
#include <sstream>
#include <cstring>
#include <span>
#include <array>
#include <memory>
#include <filesystem>

namespace Soldank
{
struct MapData
{
    std::array<float, 4> boundaries_xy;
    float polygons_min_x;
    float polygons_max_x;
    float polygons_min_y;
    float polygons_max_y;
    float width;
    float height;
    float center_x;
    float center_y;

    int version;

    std::string name;
    std::string description;
    std::string texture_name;

    PMSColor background_top_color;
    PMSColor background_bottom_color;

    int jet_count;
    unsigned char grenades_count;
    unsigned char medikits_count;
    PMSWeatherType weather_type;
    PMSStepType step_type;
    int random_id;

    std::vector<PMSPolygon> polygons;

    int sectors_size;
    int sectors_count;

    std::vector<std::vector<PMSSector>> sectors_poly;
    std::vector<PMSScenery> scenery_instances;
    std::vector<PMSSceneryType> scenery_types;
    std::vector<PMSCollider> colliders;
    std::vector<PMSSpawnPoint> spawn_points;
    std::vector<PMSWayPoint> way_points;
};

class Map
{
public:
    Map() = default;
    Map(MapData map_data)
        : map_data_(std::move(map_data)){};

    void CreateEmptyMap();
    void LoadMap(const std::filesystem::path& map_path,
                 const IFileReader& file_reader = FileReader());

    static bool PointInPoly(glm::vec2 p, PMSPolygon poly);
    bool PointInPolyEdges(float x, float y, int i) const;
    glm::vec2 ClosestPerpendicular(int j, glm::vec2 pos, float* d, int* n) const;
    bool CollisionTest(glm::vec2 pos, glm::vec2& perp_vec, bool is_flag = false) const;
    bool RayCast(glm::vec2 a,
                 glm::vec2 b,
                 float& distance,
                 float max_dist,
                 bool player = false,
                 bool flag = false,
                 bool bullet = true,
                 bool check_collider = false,
                 std::uint8_t team_id = 0);

    int GetVersion() const { return map_data_.version; }

    std::string GetName() const { return map_data_.name; }

    std::string GetDescription() const { return map_data_.description; }

    PMSColor GetBackgroundTopColor() const { return map_data_.background_top_color; }

    PMSColor GetBackgroundBottomColor() const { return map_data_.background_bottom_color; }

    std::span<float, 4> GetBoundaries() { return map_data_.boundaries_xy; }

    const std::vector<PMSPolygon>& GetPolygons() const { return map_data_.polygons; }

    unsigned int GetPolygonsCount() const { return map_data_.polygons.size(); }

    const std::vector<PMSScenery>& GetSceneryInstances() const
    {
        return map_data_.scenery_instances;
    }

    const std::vector<PMSSceneryType>& GetSceneryTypes() const { return map_data_.scenery_types; }

    const std::vector<PMSSpawnPoint>& GetSpawnPoints() const { return map_data_.spawn_points; }

    const std::vector<PMSCollider>& GetColliders() const { return map_data_.colliders; }

    const std::vector<PMSWayPoint>& GetWayPoints() const { return map_data_.way_points; }

    const std::string& GetTextureName() const { return map_data_.texture_name; }

    int GetJetCount() const { return map_data_.jet_count; }

    unsigned char GetGrenadesCount() const { return map_data_.grenades_count; }

    unsigned char GetMedikitsCount() const { return map_data_.medikits_count; }

    PMSWeatherType GetWeatherType() const { return map_data_.weather_type; }

    PMSStepType GetStepType() const { return map_data_.step_type; }

    int GetSectorsSize() const { return map_data_.sectors_size; }

    int GetSectorsCount() const { return map_data_.sectors_count; }

    const PMSSector& GetSector(unsigned int x, unsigned int y) const
    {
        return map_data_.sectors_poly[x][y];
    }

    std::optional<PMSSpawnPoint> FindFirstSpawnPoint(PMSSpawnPointType spawn_point_type) const;

    enum MapBoundary
    {
        TopBoundary = 0,
        BottomBoundary,
        LeftBoundary,
        RightBoundary
    };

private:
    MapData map_data_;

    template<typename DataType>
    static void ReadFromBuffer(std::stringstream& buffer, DataType& variable_to_save_data)
    {
        buffer.read((char*)&variable_to_save_data, sizeof(DataType));
    }

    static void ReadStringFromBuffer(std::stringstream& buffer,
                                     std::string& string_to_save_data,
                                     unsigned int max_string_size)
    {
        unsigned char string_size = 0;
        ReadFromBuffer(buffer, string_size);
        // We need an array with dynamic size here
        // NOLINTNEXTLINE(modernize-avoid-c-arrays,cppcoreguidelines-avoid-c-arrays)
        auto bytes = std::make_unique<char[]>(string_size);
        buffer.read(bytes.get(), string_size);
        string_to_save_data.assign(bytes.get(), string_size);
        // We need an array with dynamic size here
        // NOLINTNEXTLINE(modernize-avoid-c-arrays,cppcoreguidelines-avoid-c-arrays)
        auto filler = std::make_unique<char[]>(max_string_size - string_size);
        buffer.read(filler.get(), max_string_size - string_size);
    }

    void ReadPolygonsFromBuffer(std::stringstream& buffer);
    void ReadSectorsFromBuffer(std::stringstream& buffer);
    void ReadSceneryInstancesFromBuffer(std::stringstream& buffer);
    void ReadSceneryTypesFromBuffer(std::stringstream& buffer);
    void ReadCollidersFromBuffer(std::stringstream& buffer);
    void ReadSpawnPointsFromBuffer(std::stringstream& buffer);
    void ReadWayPointsFromBuffer(std::stringstream& buffer);

    void UpdateBoundaries();
};
} // namespace Soldank

#endif
