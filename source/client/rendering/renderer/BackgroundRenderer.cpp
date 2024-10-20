#include "BackgroundRenderer.hpp"

#include "rendering/renderer/Renderer.hpp"
#include "rendering/shaders/ShaderSources.hpp"

#include <filesystem>

namespace Soldank
{
BackgroundRenderer::BackgroundRenderer(Map& map)
    : shader_(ShaderSources::NO_TEXTURE_VERTEX_SHADER_SOURCE,
              ShaderSources::NO_TEXTURE_FRAGMENT_SHADER_SOURCE)
{
    PMSColor background_bottom_color = map.GetBackgroundBottomColor();
    PMSColor background_top_color = map.GetBackgroundTopColor();
    std::span<float, 4> boundaries = map.GetBoundaries();
    std::vector<float> vertices;

    GenerateGLBufferVertices(background_top_color, background_bottom_color, boundaries, vertices);

    vbo_ = Renderer::CreateVBO(vertices, GL_DYNAMIC_DRAW);

    map.GetMapChangeEvents().changed_background_color.AddObserver(
      [this](
        const PMSColor& top_color, const PMSColor& bottom_color, std::span<float, 4> boundaries) {
          OnChangeBackgroundColor(top_color, bottom_color, boundaries);
      });
}

BackgroundRenderer::~BackgroundRenderer()
{
    Renderer::FreeVBO(vbo_);
}

void BackgroundRenderer::Render(glm::mat4 transform)
{
    shader_.Use();
    Renderer::SetupVertexArray(vbo_, std::nullopt, true, false);
    shader_.SetMatrix4("transform", transform);
    Renderer::DrawArrays(GL_TRIANGLES, 0, 6);
}

void BackgroundRenderer::OnChangeBackgroundColor(const PMSColor& top_color,
                                                 const PMSColor& bottom_color,
                                                 std::span<float, 4> boundaries)
{
    std::vector<float> vertices;
    GenerateGLBufferVertices(top_color, bottom_color, boundaries, vertices);
    Renderer::ModifyVBOVertices(vbo_, vertices);
}

void BackgroundRenderer::GenerateGLBufferVertices(PMSColor background_top_color,
                                                  PMSColor background_bottom_color,
                                                  std::span<float, 4> boundaries,
                                                  std::vector<float>& destination_vertices)
{
    float bottom_red = (float)background_bottom_color.red / 255.0F;
    float bottom_green = (float)background_bottom_color.green / 255.0F;
    float bottom_blue = (float)background_bottom_color.blue / 255.0F;

    float top_red = (float)background_top_color.red / 255.0F;
    float top_green = (float)background_top_color.green / 255.0F;
    float top_blue = (float)background_top_color.blue / 255.0F;

    float left = boundaries[2] * 2.0F;
    float bottom = -boundaries[1] * 2.0F;
    float right = boundaries[3] * 2.0F;
    float top = -boundaries[0] * 2.0F;

    // clang-format off
    destination_vertices = { 
        // position             // color
        left,  bottom, 1.0,     bottom_red, bottom_green, bottom_blue, 1.0,
        right, bottom, 1.0,     bottom_red, bottom_green, bottom_blue, 1.0,
        left,  top,    1.0,     top_red,    top_green,    top_blue,    1.0,

        right, bottom, 1.0,     bottom_red, bottom_green, bottom_blue, 1.0,
        left,  top,    1.0,     top_red,    top_green,    top_blue,    1.0,
        right, top,    1.0,     top_red,    top_green,    top_blue,    1.0
    };
    // clang-format on
}
} // namespace Soldank
