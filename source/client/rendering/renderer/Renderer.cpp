module;

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <memory>
#include <vector>
#include <optional>
#include <array>
#include <cstdio>

export module Renderer;

export namespace Soldank::Renderer
{
unsigned int CreateVBO(const std::vector<float>& vertices, GLenum usage);
void ModifyVBOVertices(unsigned int vbo, const std::vector<float>& vertices, int offset = 0);
unsigned int CreateEBO(const std::vector<unsigned int>& indices, GLenum usage);

void FreeVBO(unsigned int vbo);
void FreeEBO(unsigned int ebo);

void SetupVertexArray(unsigned int vbo,
                      std::optional<unsigned int> ebo,
                      bool has_color = true,
                      bool has_texture = true);

void BindTexture(unsigned int texture);

void DrawArrays(GLenum mode, GLint first, GLsizei count);
void DrawElements(GLenum mode,
                  GLsizei count,
                  GLenum type,
                  std::optional<unsigned int> indices_offset);
} // namespace Soldank::Renderer

namespace Soldank::Renderer
{
namespace
{
unsigned int current_vbo = 0;
unsigned int current_ebo = 0;

#ifdef __EMSCRIPTEN__
void LogMissingBufferOnce(const char* draw_call, const char* buffer_name)
{
    static bool logged = false;
    if (!logged) {
        logged = true;
        // Avoid drawing with client-side arrays in WebGL; they are invalid and can abort in JS.
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
        std::fprintf(
          stderr, "WebGL renderer state error: %s called without %s\n", draw_call, buffer_name);
    }
}
#endif
} // namespace

unsigned int CreateVBO(const std::vector<float>& vertices, GLenum usage)
{
    unsigned int vbo = 0;
    glGenBuffers(1, &vbo);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 (long long)vertices.size() * (long long)sizeof(float),
                 vertices.data(),
                 usage);

    return vbo;
}

void ModifyVBOVertices(unsigned int vbo, const std::vector<float>& vertices, int offset)
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER,
                    offset,
                    (long long)vertices.size() * (long long)sizeof(float),
                    vertices.data());
}

unsigned int CreateEBO(const std::vector<unsigned int>& indices, GLenum usage)
{
    unsigned int ebo = 0;
    glGenBuffers(1, &ebo);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 (long long)indices.size() * (long long)sizeof(unsigned int),
                 indices.data(),
                 usage);

    return ebo;
}

void FreeVBO(unsigned int vbo)
{
    glDeleteBuffers(1, &vbo);
}

void FreeEBO(unsigned int ebo)
{
    glDeleteBuffers(1, &ebo);
}

void SetupVertexArray(unsigned int vbo,
                      std::optional<unsigned int> ebo,
                      bool has_color,
                      bool has_texture)
{
    current_vbo = vbo;
    current_ebo = ebo.value_or(0);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    if (ebo) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *ebo);
    }

    GLsizei stride = 3;
    if (has_color) {
        stride += 4;
    }
    if (has_texture) {
        stride += 2;
    }
    stride *= sizeof(float);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glVertexAttrib4f(1, 1.0F, 1.0F, 1.0F, 1.0F);
    glVertexAttrib2f(2, 0.0F, 0.0F);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
    glEnableVertexAttribArray(0);

    int offset = 3;
    if (has_color) {
        // color attribute
        // NOLINTNEXTLINE(performance-no-int-to-ptr,cppcoreguidelines-pro-type-cstyle-cast)
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(offset * sizeof(float)));
        glEnableVertexAttribArray(1);
        offset += 4;
    }
    if (has_texture) {
        // texture coord attribute
        // NOLINTNEXTLINE(performance-no-int-to-ptr,cppcoreguidelines-pro-type-cstyle-cast)
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(offset * sizeof(float)));
        glEnableVertexAttribArray(2);
    }
}

void BindTexture(unsigned int texture)
{
    glBindTexture(GL_TEXTURE_2D, texture);
}

void DrawArrays(GLenum mode, GLint first, GLsizei count)
{
#ifdef __EMSCRIPTEN__
    if (current_vbo == 0) {
        LogMissingBufferOnce("DrawArrays", "ARRAY_BUFFER");
        return;
    }
#endif
    glBindBuffer(GL_ARRAY_BUFFER, current_vbo);
    glDrawArrays(mode, first, count);
}

void DrawElements(GLenum mode,
                  GLsizei count,
                  GLenum type,
                  std::optional<unsigned int> indices_offset)
{
#ifdef __EMSCRIPTEN__
    if (current_vbo == 0) {
        LogMissingBufferOnce("DrawElements", "ARRAY_BUFFER");
        return;
    }
    if (current_ebo == 0) {
        LogMissingBufferOnce("DrawElements", "ELEMENT_ARRAY_BUFFER");
        return;
    }
#endif
    glBindBuffer(GL_ARRAY_BUFFER, current_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, current_ebo);

    if (indices_offset) {
        unsigned int offset = *indices_offset;
        // NOLINTNEXTLINE(performance-no-int-to-ptr,cppcoreguidelines-pro-type-cstyle-cast)
        glDrawElements(mode, count, type, (const GLvoid*)(uintptr_t)(offset));
    } else {
        glDrawElements(mode, count, type, nullptr);
    }
}
} // namespace Soldank::Renderer
