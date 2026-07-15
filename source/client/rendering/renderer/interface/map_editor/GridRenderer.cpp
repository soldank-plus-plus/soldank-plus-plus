module;

#include "rendering/shaders/ShaderSources.hpp"

#include <glad/glad.h>

#include <optional>
#include <vector>

export module GridRenderer;

import Extern.Glm;

import Renderer;
import Rendering.Gpu.GpuBuffer;
import Shader;

export namespace Soldank
{
class GridRenderer
{
public:
    GridRenderer();
    ~GridRenderer();

    // it's not safe to be able to copy/move this because we would also need to take care of the
    // created OpenGL buffers and textures
    GridRenderer(const GridRenderer&) = delete;
    GridRenderer& operator=(GridRenderer other) = delete;
    GridRenderer(GridRenderer&&) = delete;
    GridRenderer& operator=(GridRenderer&& other) = delete;

    void Render(glm::vec2 window_dimensions, glm::vec2 view_position, float view_zoom);

private:
    Shader shader_;

    GpuBuffer vbo_;
};
} // namespace Soldank

namespace Soldank
{
GridRenderer::GridRenderer()
    : shader_(ShaderSources::GRID_VERTEX_SHADER_SOURCE, ShaderSources::GRID_FRAGMENT_SHADER_SOURCE)
{
    float left = -1.0;
    float bottom = -1.0;
    float right = 1.0;
    float top = 1.0;

    // clang-format off
    std::vector<float> vertices{
        // position
        left,  bottom,  1.0,
        right, bottom,  1.0,
        left,  top,     1.0,
        right, bottom,  1.0,
        left,  top,     1.0,
        right, top,     1.0
    };
    // clang-format on

    vbo_ = GpuBuffer::CreateArrayBuffer(vertices, GL_STATIC_DRAW);
}

GridRenderer::~GridRenderer() {}

void GridRenderer::Render(glm::vec2 window_dimensions, glm::vec2 view_position, float view_zoom)
{
    shader_.Use();
    Renderer::SetupVertexArray(vbo_.GetId(), std::nullopt, false, false);
    shader_.SetVec2("dimensions", window_dimensions);
    shader_.SetVec2("view_position", view_position);
    shader_.SetFloat("view_zoom", view_zoom);
    Renderer::DrawArrays(GL_TRIANGLES, 0, 6);
}
}; // namespace Soldank
