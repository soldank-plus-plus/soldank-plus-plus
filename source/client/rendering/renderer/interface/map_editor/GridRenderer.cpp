#include "rendering/renderer/interface/map_editor/GridRenderer.hpp"

#include "rendering/renderer/Renderer.hpp"
#include "rendering/shaders/ShaderSources.hpp"

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

    vbo_ = Renderer::CreateVBO(vertices, GL_STATIC_DRAW);
}

GridRenderer::~GridRenderer()
{
    Renderer::FreeVBO(vbo_);
}

void GridRenderer::Render(glm::vec2 window_dimensions, glm::vec2 view_position, float view_zoom)
{
    shader_.Use();
    Renderer::SetupVertexArray(vbo_, std::nullopt, false, false);
    shader_.SetVec2("dimensions", window_dimensions);
    shader_.SetVec2("view_position", view_position);
    shader_.SetFloat("view_zoom", view_zoom);
    Renderer::DrawArrays(GL_TRIANGLES, 0, 6);
}
}; // namespace Soldank
