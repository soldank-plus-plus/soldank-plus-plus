#ifndef __TEXTURE_HPP__
#define __TEXTURE_HPP__

#include <expected>
#include <utility>
#include <vector>
#include <memory>

namespace Soldank::Texture
{
struct TextureData
{
    unsigned int opengl_id;
    int width;
    int height;
};

class TextureGIFData
{
public:
    TextureGIFData(int width, int height);
    ~TextureGIFData();

    // it's not safe to be able to copy/move this because we would also need to take care of the
    // created OpenGL buffers and textures
    TextureGIFData(const TextureGIFData&) = delete;
    TextureGIFData& operator=(TextureGIFData other) = delete;
    TextureGIFData(TextureGIFData&&) = delete;
    TextureGIFData& operator=(TextureGIFData&& other) = delete;

    void AddFrame(unsigned int opengl_id, int delay);
    std::pair<unsigned int, int> GetFrame(unsigned int frame_id) const
    {
        return { opengl_ids_.at(frame_id), delays_.at(frame_id) };
    }

    unsigned int Size() const { return opengl_ids_.size(); }

    int GetWidth() const { return width_; };
    int GetHeight() const { return height_; };

private:
    std::vector<unsigned int> opengl_ids_;
    std::vector<int> delays_;
    int width_;
    int height_;
};

enum class LoadError
{
    TextureNotFound = 0
};

std::expected<TextureData, LoadError> Load(const char* texture_path);
std::expected<std::shared_ptr<TextureGIFData>, LoadError> LoadGIF(const char* texture_path);

void Delete(unsigned int texture_id);
} // namespace Soldank::Texture

#endif
