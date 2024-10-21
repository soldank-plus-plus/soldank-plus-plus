#include "rendering/renderer/interface/map_editor/SingleImageRenderer.hpp"

#include "rendering/data/Texture.hpp"
#include "rendering/renderer/Renderer.hpp"
#include "rendering/shaders/ShaderSources.hpp"

#include "spdlog/spdlog.h"
#include <filesystem>
#include <utility>
#include <vector>

namespace Soldank
{
SingleImageRenderer::SingleImageRenderer()
    : shader_(ShaderSources::DYNAMIC_COLOR_VERTEX_SHADER_SOURCE,
              ShaderSources::DYNAMIC_COLOR_FRAGMENT_SHADER_SOURCE)
    , vbo_(0)
    , texture_(0)
    , texture_width_(0)
    , texture_height_(0)
{
    float half_width = 16.0F;
    float half_height = 16.0F;

    std::vector<float> vertices{
        // position                      // color                // texture
        -half_width, half_height,  1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 0.0F, 1.0F,
        half_width,  half_height,  1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F,
        -half_width, -half_height, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 0.0F, 0.0F,
        half_width,  -half_height, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 0.0F,
    };

    std::vector<unsigned int> indices{ 0, 1, 2, 1, 3, 2 };

    vbo_ = Renderer::CreateVBO(vertices, GL_DYNAMIC_DRAW);
    ebo_ = Renderer::CreateEBO(indices, GL_STATIC_DRAW);
}

SingleImageRenderer::~SingleImageRenderer()
{
    Renderer::FreeVBO(vbo_);
    Renderer::FreeEBO(ebo_);
    Texture::Delete(texture_);
}

void SingleImageRenderer::Render(const glm::mat4& transform,
                                 const glm::vec2& position,
                                 const glm::vec4& color,
                                 const glm::vec2& scale,
                                 float rotation)
{
    if (texture_ == 0) {
        return;
    }

    shader_.Use();
    Renderer::SetupVertexArray(vbo_, ebo_, false, true);

    glm::mat4 current_scenery_transform = transform;

    current_scenery_transform =
      glm::translate(current_scenery_transform, glm::vec3(position.x, -position.y, 0.0));
    current_scenery_transform =
      glm::rotate(current_scenery_transform, rotation, glm::vec3(0.0, 0.0, 1.0));
    current_scenery_transform =
      glm::scale(current_scenery_transform, glm::vec3(scale.x, scale.y, 0.0));

    shader_.SetMatrix4("transform", current_scenery_transform);
    shader_.SetVec4("color", color);

    Renderer::BindTexture(texture_);
    Renderer::DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, std::nullopt);
}

void SingleImageRenderer::SetTexture(const std::string& file_name)
{
    if (texture_ != 0) {
        Texture::Delete(texture_);
    }

    std::filesystem::path file_path = "scenery-gfx/" + file_name;
    auto texture_or_error = Texture::Load(file_path.string().c_str());
    if (texture_or_error.has_value()) {
        texture_ = texture_or_error.value().opengl_id;

        texture_width_ = texture_or_error.value().width;
        texture_height_ = texture_or_error.value().height;

        auto width = (float)texture_width_;
        auto height = (float)texture_height_;

        // clang-format off
        std::vector<float> vertices{
            // position             // texture
            0.0F,  0.0F,   1.0F,    0.0F, 1.0F,
            width, 0.0F,   1.0F,    1.0F, 1.0F,
            0.0F,  -height, 1.0F,    0.0F, 0.0F,
            width, -height, 1.0F,    1.0F, 0.0F,
        };
        // clang-format on

        Renderer::ModifyVBOVertices(vbo_, vertices);
    } else {
        spdlog::critical("Texture file not found {}", file_path.string());
        texture_ = 0;
        texture_width_ = 0;
        texture_height_ = 0;
    }
}
} // namespace Soldank
