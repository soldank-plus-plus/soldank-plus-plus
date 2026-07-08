module;

#include "rendering/shaders/ShaderSources.hpp"

#include <glad/glad.h>

#include <cmath>
#include <optional>
#include <vector>

export module LineRenderer;

import Extern.Glm;

import Renderer;
import Rendering.Gpu.GpuBuffer;
import Shader;

export namespace Soldank
{
class LineRenderer
{
public:
    LineRenderer();
    ~LineRenderer();

    // it's not safe to be able to copy/move this because we would also need to take care of the
    // created OpenGL buffers and textures
    LineRenderer(const LineRenderer&) = delete;
    LineRenderer& operator=(LineRenderer other) = delete;
    LineRenderer(LineRenderer&&) = delete;
    LineRenderer& operator=(LineRenderer&& other) = delete;

    void Render(glm::mat4 transform, glm::vec2 p1, glm::vec2 p2, glm::vec4 color, float thickness);

private:
    Shader shader_;

    GpuBuffer vbo_;
};
} // namespace Soldank

namespace Soldank
{
LineRenderer::LineRenderer()
    : shader_(ShaderSources::DYNAMIC_COLOR_NO_TEXTURE_VERTEX_SHADER_SOURCE,
              ShaderSources::DYNAMIC_COLOR_NO_TEXTURE_FRAGMENT_SHADER_SOURCE)
{
    std::vector<float> vertices;

    for (unsigned int i = 0; i < 4; i++) {
        vertices.push_back(0.0F);
        vertices.push_back(0.0F);
        vertices.push_back(0.0F);
    }

    vbo_ = GpuBuffer::CreateArrayBuffer(vertices, GL_DYNAMIC_DRAW);
}

LineRenderer::~LineRenderer() = default;

void LineRenderer::Render(glm::mat4 transform,
                          glm::vec2 p1,
                          glm::vec2 p2,
                          glm::vec4 color,
                          float thickness)
{
    p1.y = -p1.y;
    p2.y = -p2.y;
    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;
    float length = std::sqrt(dx * dx + dy * dy);

    // Normalize direction vector
    dx /= length;
    dy /= length;

    // Perpendicular vector for thickness
    float px = -dy * thickness * 0.5F;
    float py = dx * thickness * 0.5F;

    // Vertex positions for a thick line (rectangle)
    // clang-format off
    std::vector<float> vertices{
        p1.x + px, p1.y + py, 1.0F,  // First vertex (offset from start point)
        p1.x - px, p1.y - py, 1.0F,  // Second vertex (opposite side of the start point)
        p2.x + px, p2.y + py, 1.0F,  // Third vertex (offset from end point)
        p2.x - px, p2.y - py, 1.0F   // Fourth vertex (opposite side of the end point)
    };
    // clang-format on

    shader_.Use();
    Renderer::SetupVertexArray(vbo_.GetId(), std::nullopt, false, false);
    shader_.SetMatrix4("transform", transform);
    shader_.SetVec4("color", color);

    vbo_.UpdateVertices(vertices);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
} // namespace Soldank
