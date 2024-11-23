#include "RectangleRenderer.hpp"

#include "rendering/data/Texture.hpp"
#include "rendering/renderer/Renderer.hpp"
#include "rendering/shaders/ShaderSources.hpp"

#include "core/math/Calc.hpp"

#include <filesystem>
#include <cmath>

namespace Soldank
{
RectangleRenderer::RectangleRenderer()
    : shader_(ShaderSources::DYNAMIC_COLOR_NO_TEXTURE_VERTEX_SHADER_SOURCE,
              ShaderSources::DYNAMIC_COLOR_NO_TEXTURE_FRAGMENT_SHADER_SOURCE)
{
    float bottom_red = 255.0 / 255.0;
    float bottom_green = 0.0 / 255.0;
    float bottom_blue = 0.0 / 255.0;

    float top_red = 255.0 / 255.0;
    float top_green = 0.0 / 255.0;
    float top_blue = 0.0 / 255.0;

    float left = -2.5;
    float bottom = -2.5;
    float right = 2.5;
    float top = 2.5;

    std::vector<float> vertices{ // position         // color
                                 left,  bottom, 1.0, bottom_red, bottom_green, bottom_blue, 1.0,
                                 right, bottom, 1.0, bottom_red, bottom_green, bottom_blue, 1.0,
                                 left,  top,    1.0, top_red,    top_green,    top_blue,    1.0,

                                 right, bottom, 1.0, bottom_red, bottom_green, bottom_blue, 1.0,
                                 left,  top,    1.0, top_red,    top_green,    top_blue,    1.0,
                                 right, top,    1.0, top_red,    top_green,    top_blue,    1.0
    };

    vbo_ = Renderer::CreateVBO(vertices, GL_STATIC_DRAW);
}

RectangleRenderer::~RectangleRenderer()
{
    Renderer::FreeVBO(vbo_);
}

void RectangleRenderer::Render(const glm::mat4& transform,
                               const glm::vec2& position,
                               const glm::vec4& color)
{
    shader_.Use();
    Renderer::SetupVertexArray(vbo_, std::nullopt, true, false);

    glm::mat4 current_transform =
      glm::translate(transform, glm::vec3(position.x, -position.y, 0.0));
    shader_.SetMatrix4("transform", current_transform);
    shader_.SetVec4("color", color);
    Renderer::DrawArrays(GL_TRIANGLES, 0, 6);
}
} // namespace Soldank
