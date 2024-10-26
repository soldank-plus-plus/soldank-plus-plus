#ifndef __MAP_HPP__
#define __MAP_HPP__

#include "core/map/PMSConstants.hpp"
#include "core/map/PMSEnums.hpp"
#include "core/map/PMSStructs.hpp"

#include "core/math/Glm.hpp"
#include "core/data/IFileReader.hpp"
#include "core/data/FileReader.hpp"
#include "core/data/IFileWriter.hpp"
#include "core/data/FileWriter.hpp"

#include "core/utility/Observable.hpp"

#include <bitset>
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

struct MapChangeEvents
{
    Observable<const PMSPolygon&> added_new_polygon;
    Observable<const PMSPolygon&, const std::vector<PMSPolygon>&> removed_polygon;
    Observable<const std::vector<PMSPolygon>, const std::vector<PMSPolygon>&> added_new_polygons;
    Observable<const std::vector<PMSPolygon>, const std::vector<PMSPolygon>&> removed_polygons;
    Observable<const PMSSpawnPoint&, unsigned int> added_new_spawn_point;
    Observable<const PMSSpawnPoint&, unsigned int> removed_spawn_point;
    Observable<const std::vector<PMSSpawnPoint>&> removed_spawn_points;
    Observable<const std::vector<PMSSpawnPoint>&> added_spawn_points;
    Observable<const PMSColor&, const PMSColor&, std::span<float, 4>> changed_background_color;
    Observable<const std::string&> changed_texture_name;
    Observable<const PMSScenery&, unsigned int> added_new_scenery;
    Observable<const PMSScenery&, unsigned int, const std::vector<PMSScenery>&> removed_scenery;
    Observable<const PMSSceneryType&> added_new_scenery_type;
    Observable<const PMSSceneryType&, unsigned short, const std::vector<PMSSceneryType>&>
      removed_scenery_type;
    Observable<const std::vector<PMSScenery>&> added_sceneries;
    Observable<const std::vector<PMSScenery>&> removed_sceneries;
    Observable<const std::vector<std::pair<unsigned short, PMSSceneryType>>&> removed_scenery_types;
    Observable<const std::vector<PMSScenery>&> modified_sceneries;
    Observable<const std::vector<PMSPolygon>&> modified_polygons;
    Observable<const std::vector<PMSSpawnPoint>&> modified_spawn_points;
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
    void SaveMap(const std::filesystem::path& map_path,
                 std::shared_ptr<IFileWriter> file_writer = std::make_shared<FileWriter>());

    static bool PointInPoly(glm::vec2 p, PMSPolygon poly);
    bool PointInPolyEdges(float x, float y, int i) const;
    static bool PointInScenery(glm::vec2 p, const PMSScenery& scenery);
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
                 std::uint8_t team_id = 0) const;
    static bool LineInPoly(const glm::vec2& a,
                           const glm::vec2& b,
                           const PMSPolygon& polygon,
                           glm::vec2& v);

    MapChangeEvents& GetMapChangeEvents() { return map_change_events_; }

    PMSPolygon AddNewPolygon(const PMSPolygon& polygon);
    void AddPolygons(const std::vector<PMSPolygon>& polygons);
    PMSPolygon RemovePolygonById(unsigned int id);
    void RemovePolygonsById(const std::vector<unsigned int>& polygon_ids);
    void SetPolygonVerticesColorById(
      const std::vector<std::pair<std::pair<unsigned int, unsigned int>, PMSColor>>&
        polygon_vertices_with_new_color);

    unsigned int AddNewSpawnPoint(const PMSSpawnPoint& spawn_point);
    PMSSpawnPoint RemoveSpawnPointById(unsigned int id);
    void AddSpawnPoints(const std::vector<std::pair<unsigned int, PMSSpawnPoint>>& spawn_points);
    void RemoveSpawnPointsById(const std::vector<unsigned int>& spawn_point_ids);
    void MoveSpawnPointsById(
      const std::vector<std::pair<unsigned int, glm::ivec2>>& spawn_point_ids_with_new_position);

    unsigned int AddNewScenery(const PMSScenery& scenery, const std::string& file_name);
    PMSScenery RemoveSceneryById(unsigned int id);
    void AddSceneries(
      const std::vector<std::pair<unsigned int, std::pair<PMSScenery, std::string>>>& sceneries);
    void RemoveSceneriesById(const std::vector<unsigned int>& scenery_ids);
    void SetSceneriesColorById(
      const std::vector<std::pair<unsigned int, PMSColor>>& scenery_ids_with_new_color);

    static std::array<glm::vec2, 4> GetSceneryVertexPositions(const PMSScenery& scenery);

    int GetVersion() const { return map_data_.version; }

    std::string GetName() const { return map_data_.name; }

    void SetDescription(const std::string& new_description)
    {
        map_data_.description = new_description;
    }

    std::string GetDescription() const { return map_data_.description; }

    void SetBackgroundTopColor(const PMSColor& color)
    {
        map_data_.background_top_color = color;
        map_change_events_.changed_background_color.Notify(
          map_data_.background_top_color, map_data_.background_bottom_color, GetBoundaries());
    }
    PMSColor GetBackgroundTopColor() const { return map_data_.background_top_color; }

    void SetBackgroundBottomColor(const PMSColor& color)
    {
        map_data_.background_bottom_color = color;
        map_change_events_.changed_background_color.Notify(
          map_data_.background_top_color, map_data_.background_bottom_color, GetBoundaries());
    }
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

    void SetTextureName(const std::string& texture_name)
    {
        map_data_.texture_name = texture_name;
        map_change_events_.changed_texture_name.Notify(map_data_.texture_name);
    }
    const std::string& GetTextureName() const { return map_data_.texture_name; }

    void SetJetCount(int jet_count) { map_data_.jet_count = jet_count; }
    int GetJetCount() const { return map_data_.jet_count; }

    void SetGrenadesCount(unsigned char grenades_count)
    {
        map_data_.grenades_count = grenades_count;
    }
    unsigned char GetGrenadesCount() const { return map_data_.grenades_count; }

    void SetMedikitsCount(unsigned char medikits_count)
    {
        map_data_.medikits_count = medikits_count;
    }
    unsigned char GetMedikitsCount() const { return map_data_.medikits_count; }

    void SetWeatherType(PMSWeatherType weather_type) { map_data_.weather_type = weather_type; }
    PMSWeatherType GetWeatherType() const { return map_data_.weather_type; }
    std::string GetWeatherTypeText() const
    {
        switch (map_data_.weather_type) {
            case PMSWeatherType::None:
                return "None";
            case PMSWeatherType::Rain:
                return "Rain";
            case PMSWeatherType::Sandstorm:
                return "Sandstorm";
            case PMSWeatherType::Snow:
                return "Snow";
        }
    }

    void SetStepType(PMSStepType step_type) { map_data_.step_type = step_type; }
    PMSStepType GetStepType() const { return map_data_.step_type; }
    std::string GetStepTypeText() const
    {
        switch (map_data_.step_type) {
            case PMSStepType::HardGround:
                return "Hard";
            case PMSStepType::SoftGround:
                return "Soft";
            case PMSStepType::None:
                return "None";
        }
    }

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
    MapChangeEvents map_change_events_;

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

    template<typename DataType>
    static void AppendToFileWriter(std::shared_ptr<IFileWriter>& file_writer, const DataType& data)
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        file_writer->AppendData(reinterpret_cast<const char*>(&data), sizeof(data));
    }

    static void AppendStringToFileWriter(std::shared_ptr<IFileWriter>& file_writer,
                                         const std::string& data,
                                         unsigned int max_string_size)
    {
        unsigned char string_size = data.length();
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        file_writer->AppendData(reinterpret_cast<char*>(&string_size), sizeof(string_size));
        unsigned long long data_size = data.length() * sizeof(char);
        file_writer->AppendData(data.c_str(), (std::streamsize)data_size);
        std::array<char, 64> filler{};
        filler.fill(0);
        data_size = (max_string_size - data.length()) * sizeof(char);
        file_writer->AppendData(filler.data(), (std::streamsize)data_size);
    }

    void ReadPolygonsFromBuffer(std::stringstream& buffer);
    void ReadSectorsFromBuffer(std::stringstream& buffer);
    void ReadSceneryInstancesFromBuffer(std::stringstream& buffer);
    void ReadSceneryTypesFromBuffer(std::stringstream& buffer);
    void ReadCollidersFromBuffer(std::stringstream& buffer);
    void ReadSpawnPointsFromBuffer(std::stringstream& buffer);
    void ReadWayPointsFromBuffer(std::stringstream& buffer);

    void AppendPolygonsToFileWriter(std::shared_ptr<IFileWriter>& file_writer);
    void AppendSectorsToFileWriter(std::shared_ptr<IFileWriter>& file_writer);
    void AppendSceneryInstancesToFileWriter(std::shared_ptr<IFileWriter>& file_writer);
    void AppendSceneryTypesToFileWriter(std::shared_ptr<IFileWriter>& file_writer);
    void AppendCollidersToFileWriter(std::shared_ptr<IFileWriter>& file_writer);
    void AppendSpawnPointsToFileWriter(std::shared_ptr<IFileWriter>& file_writer);
    void AppendWayPointsToFileWriter(std::shared_ptr<IFileWriter>& file_writer);

    void UpdateBoundaries();
    void GenerateSectors();
    bool IsPolygonInSector(unsigned short polygon_index,
                           float sector_x,
                           float sector_y,
                           float sector_size);

    static void SetPolygonVerticesAndPerpendiculars(PMSPolygon& polygon);

    void FixPolygonIds();
};
} // namespace Soldank

#endif
