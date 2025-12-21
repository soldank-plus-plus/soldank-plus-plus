module;

#include "rendering/shaders/ShaderSources.hpp"

#include "core/math/Glm.hpp"

#include <glad/glad.h>

#include <optional>

export module CircleRenderer;

import Renderer;
import Shader;

export namespace Soldank
{
class CircleRenderer
{
public:
    CircleRenderer();
    ~CircleRenderer();

    // it's not safe to be able to copy/move this because we would also need to take care of the
    // created OpenGL buffers and textures
    CircleRenderer(const CircleRenderer&) = delete;
    CircleRenderer& operator=(CircleRenderer other) = delete;
    CircleRenderer(CircleRenderer&&) = delete;
    CircleRenderer& operator=(CircleRenderer&& other) = delete;

    void Render(glm::mat4 transform,
                glm::vec2 position,
                glm::vec4 color,
                float outer_radius,
                float inner_radius = 0.0F);

private:
    Shader shader_;

    unsigned int vbo_;
};
} // namespace Soldank

namespace Soldank
{
CircleRenderer::CircleRenderer()
    : shader_(ShaderSources::CIRCLE_VERTEX_SHADER_SOURCE,
              ShaderSources::CIRCLE_FRAGMENT_SHADER_SOURCE)
{
    std::vector<float> vertices(18, 0.0F);

    vbo_ = Renderer::CreateVBO(vertices, GL_DYNAMIC_DRAW);
}

CircleRenderer::~CircleRenderer()
{
    Renderer::FreeVBO(vbo_);
}

void CircleRenderer::Render(glm::mat4 transform,
                            glm::vec2 position,
                            glm::vec4 color,
                            float outer_radius,
                            float inner_radius)
{
    float left = -outer_radius;
    float bottom = -outer_radius;
    float right = outer_radius;
    float top = outer_radius;

    shader_.Use();
    Renderer::SetupVertexArray(vbo_, std::nullopt, false, false);

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

    glBufferSubData(
      GL_ARRAY_BUFFER, 0, (long long)vertices.size() * (long long)sizeof(float), vertices.data());

    transform = glm::translate(transform, glm::vec3(position.x, -position.y, 0.0));
    shader_.SetMatrix4("transform", transform);
    shader_.SetFloat("outerRadius", outer_radius);
    shader_.SetFloat("innerRadius", inner_radius);
    shader_.SetVec4("color", color);
    Renderer::DrawArrays(GL_TRIANGLES, 0, 6);
}
} // namespace Soldank
