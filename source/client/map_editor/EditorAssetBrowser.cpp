module;

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

export module MapEditor.EditorAssetBrowser;

export namespace Soldank
{
class EditorAssetBrowser
{
public:
    static std::vector<std::string> LoadTextureNames() { return LoadImageNames("textures"); }

    static std::vector<std::string> LoadSceneryNames() { return LoadImageNames("scenery-gfx"); }

private:
    static bool IsSupportedImageExtension(const std::filesystem::path& extension)
    {
        return extension == ".bmp" || extension == ".jpg" || extension == ".jpeg" ||
               extension == ".gif" || extension == ".png";
    }

    static std::vector<std::string> LoadImageNames(const std::filesystem::path& directory_path)
    {
        std::vector<std::string> image_names;
        for (std::filesystem::directory_iterator entry_iterator(directory_path);
             entry_iterator != std::filesystem::directory_iterator();
             ++entry_iterator) {
            const auto& entry = *entry_iterator;
            if (entry.is_directory() || !entry.path().has_extension()) {
                continue;
            }

            if (IsSupportedImageExtension(entry.path().extension())) {
                image_names.push_back(entry.path().filename().string());
            }
        }
        std::ranges::sort(image_names);
        return image_names;
    }
};
} // namespace Soldank
