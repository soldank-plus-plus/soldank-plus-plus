#include "Texture.hpp"

#include "rendering/data/Texture.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stb_image.h>

#include <cstddef>
#include <span>
#include <expected>
#include <fstream>
#include <sstream>

namespace Soldank::Texture
{
TextureGIFData::TextureGIFData(int width, int height)
    : width_(width)
    , height_(height)
{
}

TextureGIFData::~TextureGIFData()
{
    for (unsigned int texture_id : opengl_ids_) {
        glDeleteTextures(1, &texture_id);
    }
}

void TextureGIFData::AddFrame(unsigned int opengl_id, int delay)
{
    opengl_ids_.push_back(opengl_id);
    delays_.push_back(delay);
}

std::expected<TextureData, LoadError> Load(const char* texture_path)
{
    unsigned int texture_id = 0;
    int texture_width = 0;
    int texture_height = 0;

    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    int texture_nr_channels = 0;
    stbi_set_flip_vertically_on_load(1);
    unsigned char* data = stbi_load(
      texture_path, &texture_width, &texture_height, &texture_nr_channels, STBI_rgb_alpha);
    if (data != nullptr) {
        std::span pixels{ data, static_cast<size_t>(texture_height * texture_width * 4) };

        // Changing fully green pixels to be transparent
        for (int y = 0; y < texture_height; y++) {
            for (int x = 0; x < texture_width; x++) {
                if (pixels[(x + y * texture_width) * 4 + 0] == 0 &&
                    pixels[(x + y * texture_width) * 4 + 1] == 255 &&
                    pixels[(x + y * texture_width) * 4 + 2] == 0) {
                    pixels[(x + y * texture_width) * 4 + 3] = 0;
                }
            }
        }

        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGBA,
                     texture_width,
                     texture_height,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     pixels.data());
    } else {
        return std::unexpected(LoadError::TextureNotFound);
    }
    stbi_image_free(data);

    return TextureData{ .opengl_id = texture_id, .width = texture_width, .height = texture_height };
}

std::expected<std::shared_ptr<TextureGIFData>, LoadError> LoadGIF(const char* texture_path)
{
    std::basic_ifstream<unsigned char> file(texture_path,
                                            std::ios_base::in | std::ios_base::binary);
    if (!file.is_open()) {
        return std::unexpected(LoadError::TextureNotFound);
    }

    std::basic_stringstream<unsigned char> buffer;
    buffer << file.rdbuf();

    int* delays_data = nullptr;
    int texture_width = 0;
    int texture_height = 0;
    int frame_count = 0;
    int channels = 0;
    unsigned char* data = stbi_load_gif_from_memory(buffer.str().c_str(),
                                                    (int)buffer.str().size(),
                                                    &delays_data,
                                                    &texture_width,
                                                    &texture_height,
                                                    &frame_count,
                                                    &channels,
                                                    4);
    if (data == nullptr) {
        return std::unexpected(LoadError::TextureNotFound);
    }

    std::shared_ptr<TextureGIFData> texture_gif_data =
      std::make_shared<TextureGIFData>(texture_width, texture_height);

    std::span delays = std::span{ delays_data, static_cast<size_t>(frame_count) };

    int frame_size = texture_height * texture_width * 4;
    int data_size = frame_size * frame_count;
    std::span all_pixels{ data, static_cast<size_t>(data_size) };
    for (int frame_id = 0; frame_id < frame_count; ++frame_id) {
        int offset = (texture_height * texture_width * 4) * frame_id;
        std::span pixels = all_pixels.subspan(offset, frame_size);

        // Changing fully green pixels to be transparent
        for (int y = 0; y < texture_height; y++) {
            for (int x = 0; x < texture_width; x++) {
                if (pixels[(x + y * texture_width) * 4 + 0] == 0 &&
                    pixels[(x + y * texture_width) * 4 + 1] == 255 &&
                    pixels[(x + y * texture_width) * 4 + 2] == 0) {
                    pixels[(x + y * texture_width) * 4 + 3] = 0;
                }
            }
        }
        unsigned int texture_id = 0;

        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_2D, texture_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGBA,
                     texture_width,
                     texture_height,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     pixels.data());

        texture_gif_data->AddFrame(texture_id, delays[frame_id]);
    }

    stbi_image_free(delays_data);
    stbi_image_free(data);

    return texture_gif_data;
}

void Delete(unsigned int texture_id)
{
    glDeleteTextures(1, &texture_id);
}
} // namespace Soldank::Texture
