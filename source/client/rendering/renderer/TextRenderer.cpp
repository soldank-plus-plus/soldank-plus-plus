#include "TextRenderer.hpp"

#include "rendering/shaders/ShaderSources.hpp"

#include "spdlog/spdlog.h"

#include <map>
#include <array>

namespace Soldank
{
TextRenderer::TextRenderer(const std::string& file_path, unsigned int font_height)
    : shader_(ShaderSources::FONT_VERTEX_SHADER_SOURCE, ShaderSources::FONT_FRAGMENT_SHADER_SOURCE)
{
    FT_Library ft = nullptr;
    // All functions return a value different than 0 whenever an error occurred
    if (FT_Init_FreeType(&ft) != 0) {
        spdlog::error("ERROR::FREETYPE: Could not init FreeType Library");
    }

    if (file_path.empty()) {
        spdlog::error("ERROR::FREETYPE: Failed to load file_path");
    }

    // load font as face
    FT_Face face = nullptr;
    if (FT_New_Face(ft, file_path.c_str(), 0, &face) != 0) {
        spdlog::error("ERROR::FREETYPE: Failed to load font");
    } else {
        // set size to load glyphs as
        FT_Set_Pixel_Sizes(face, 0, font_height);

        // disable byte-alignment restriction
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        // load first 128 characters of ASCII set
        for (unsigned char c = 0; c < 128; c++) {
            // Load character glyph
            if (FT_Load_Char(face, c, FT_LOAD_RENDER) != 0) {
                spdlog::error("ERROR::FREETYTPE: Failed to load Glyph");
                continue;
            }
            // generate texture
            unsigned int texture = 0;
            glActiveTexture(GL_TEXTURE0);
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(GL_TEXTURE_2D,
                         0,
                         GL_RED,
                         (GLsizei)face->glyph->bitmap.width,
                         (GLsizei)face->glyph->bitmap.rows,
                         0,
                         GL_RED,
                         GL_UNSIGNED_BYTE,
                         face->glyph->bitmap.buffer);
            // set texture options
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            // now store character for later use
            Character character = { texture,
                                    glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                                    glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                                    static_cast<unsigned int>(face->glyph->advance.x) };
            characters_.insert({ c, character });
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    // destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    vbo_ = 0;
    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
}

TextRenderer::~TextRenderer()
{
    glDeleteBuffers(1, &vbo_);
    for (auto [symbol, character] : characters_) {
        glDeleteTextures(1, &character.texture_id);
    }
}

void TextRenderer::Render(std::string text,
                          float x,
                          float y,
                          float scale,
                          glm::vec3 color,
                          glm::vec2 window_dimensions)
{
    // activate corresponding render state
    shader_.Use();
    shader_.SetVec3("textColor", color);
    glm::mat4 projection = glm::ortho(0.0F, window_dimensions.x, 0.0F, window_dimensions.y);
    shader_.SetMatrix4("projection", projection);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    // iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++) {
        Character ch = characters_[*c];

        float xpos = x + (float)ch.bearing.x * scale;
        float ypos = y - (float)(ch.size.y - ch.bearing.y) * scale;

        float w = (float)ch.size.x * scale;
        float h = (float)ch.size.y * scale;
        // update VBO for each character
        std::array<std::array<float, 4>, 6> vertices = { { { xpos, ypos + h, 0.0, 0.0 },
                                                           { xpos, ypos, 0.0, 1.0 },
                                                           { xpos + w, ypos, 1.0, 1.0 },

                                                           { xpos, ypos + h, 0.0, 0.0 },
                                                           { xpos + w, ypos, 1.0, 1.0 },
                                                           { xpos + w, ypos + h, 1.0, 0.0 } } };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.texture_id);
        glBufferSubData(GL_ARRAY_BUFFER, 0, 6ULL * 4 * sizeof(float), vertices.data());

        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (float)(ch.advance >> 6) *
             scale; // bitshift by 6 to get value in pixels (2^6 = 64 (divide
                    // amount of 1/64th pixels by 64 to get amount of pixels))
    }
}
} // namespace Soldank
