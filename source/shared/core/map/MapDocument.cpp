module;

#include <filesystem>
#include <memory>
#include <utility>

export module Shared.Core.Map.MapDocument;

import Shared.Core.Data.IFileReader;
import Shared.Core.Data.FileReader;
import Shared.Core.Data.IFileWriter;
import Shared.Core.Data.FileWriter;
import Shared.Core.Map.Map;

export namespace Soldank
{
class MapDocument
{
public:
    MapDocument() = default;

    explicit MapDocument(Map map)
        : map_(std::move(map))
    {
    }

    static MapDocument FromMap(const Map& map) { return MapDocument(map); }

    void CreateEmptyMap() { map_.CreateEmptyMap(); }

    void LoadMap(const std::filesystem::path& map_path,
                 const IFileReader& file_reader = FileReader())
    {
        map_.LoadMap(map_path, file_reader);
    }

    void SaveMap(const std::filesystem::path& map_path,
                 std::shared_ptr<IFileWriter> file_writer = std::make_shared<FileWriter>()) const
    {
        map_.SaveMap(map_path, std::move(file_writer));
    }

    const Map& GetMap() const { return map_; }

    Map& GetMutableMap() { return map_; }

private:
    Map map_;
};
} // namespace Soldank
