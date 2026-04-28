module;

#include "core/math/Glm.hpp"

#include "rendering/shaders/ShaderSources.hpp"

#include <glad/glad.h>

#include <filesystem>
#include <cmath>
#include <vector>

export module RectangleRenderer;

import Texture;
import Renderer;
import Shader;

import Shared.Core.Math.Calc;

export namespace Soldank
{
class RectangleRenderer
{
public:
    RectangleRenderer();
    ~RectangleRenderer();

    // it's not safe to be able to copy/move this because we would also need to take care of the
    // created OpenGL buffers and textures
    RectangleRenderer(const RectangleRenderer&) = delete;
    RectangleRenderer& operator=(RectangleRenderer other) = delete;
    RectangleRenderer(RectangleRenderer&&) = delete;
    RectangleRenderer& operator=(RectangleRenderer&& other) = delete;

    void Render(const glm::mat4& transform, const glm::vec2& position, const glm::vec4& color);

private:
    Shader shader_;

    unsigned int vbo_;
};
} // namespace Soldank

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
