#ifndef __TEXT_RENDERER_HPP__
#define __TEXT_RENDERER_HPP__

#include "rendering/shaders/Shader.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <vector>
#include <map>

namespace Soldank
{
struct Character
{
    unsigned int texture_id; // ID handle of the glyph texture
    glm::ivec2 size;         // Size of glyph
    glm::ivec2 bearing;      // Offset from baseline to left/top of glyph
    unsigned int advance;    // Offset to advance to next glyph
};

class TextRenderer
{
public:
    TextRenderer(const std::string& file_path, unsigned int font_height);
    ~TextRenderer();

    // it's not safe to be able to copy/move this because we would also need to take care of the
    // created OpenGL buffers and textures
    TextRenderer(const TextRenderer&) = delete;
    TextRenderer& operator=(TextRenderer other) = delete;
    TextRenderer(TextRenderer&&) = delete;
    TextRenderer& operator=(TextRenderer&& other) = delete;

    void Render(std::string text,
                float x,
                float y,
                float scale,
                glm::vec3 color,
                glm::vec2 window_dimensions);

private:
    Shader shader_;
    std::map<char, Character> characters_;

    unsigned int vbo_;
};
} // namespace Soldank

#endif
