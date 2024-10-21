#include "SceneriesRenderer.hpp"

#include "core/map/PMSConstants.hpp"
#include "core/map/PMSStructs.hpp"
#include "rendering/data/Texture.hpp"
#include "rendering/renderer/Renderer.hpp"
#include "rendering/shaders/ShaderSources.hpp"

#include "spdlog/spdlog.h"

#include <filesystem>

namespace Soldank
{
SceneriesRenderer::SceneriesRenderer(Map& map)
    : shader_(ShaderSources::VERTEX_SHADER_SOURCE, ShaderSources::FRAGMENT_SHADER_SOURCE)
{
    for (const auto& scnery_type : map.GetSceneryTypes()) {
        std::filesystem::path texture_path = "scenery-gfx/";
        texture_path += scnery_type.name;
        if (!std::filesystem::exists(texture_path)) {
            texture_path.replace_extension(".png");
        }

        auto texture_or_error = Texture::Load(texture_path.string().c_str());

        if (texture_or_error.has_value()) {
            textures_.push_back(texture_or_error.value().opengl_id);
        } else {
            spdlog::critical("Texture file not found {}", texture_path.string());
            textures_.push_back(0);
        }
    }

    std::vector<float> vertices;
    GenerateGLBufferVertices(map.GetSceneryInstances(), vertices);

    std::vector<unsigned int> indices;
    for (unsigned int i = 0; i < MAX_SCENERIES_COUNT; ++i) {
        indices.push_back(i * 4 + 0);
        indices.push_back(i * 4 + 1);
        indices.push_back(i * 4 + 2);
        indices.push_back(i * 4 + 1);
        indices.push_back(i * 4 + 3);
        indices.push_back(i * 4 + 2);
    }

    vbo_ = Renderer::CreateVBO(vertices, GL_DYNAMIC_DRAW);
    ebo_ = Renderer::CreateEBO(indices, GL_STATIC_DRAW);

    map.GetMapChangeEvents().added_new_scenery.AddObserver(
      [this](const PMSScenery& new_scenery, unsigned int new_scenery_id) {
          OnAddScenery(new_scenery, new_scenery_id);
      });
    map.GetMapChangeEvents().added_new_scenery_type.AddObserver(
      [this](const PMSSceneryType& new_scenery_type) { OnAddSceneryType(new_scenery_type); });
    map.GetMapChangeEvents().removed_scenery.AddObserver(
      [this](const PMSScenery& removed_scenery,
             unsigned int removed_scenery_id,
             const std::vector<PMSScenery>& sceneries_after_removal) {
          OnRemoveScenery(removed_scenery, removed_scenery_id, sceneries_after_removal);
      });
    map.GetMapChangeEvents().removed_scenery_type.AddObserver(
      [this](const PMSSceneryType& removed_scenery_type,
             unsigned short removed_scenery_type_id,
             const std::vector<PMSSceneryType>& scenery_types_after_removal) {
          OnRemoveSceneryType(
            removed_scenery_type, removed_scenery_type_id, scenery_types_after_removal);
      });
}

SceneriesRenderer::~SceneriesRenderer()
{
    Renderer::FreeVBO(vbo_);
    Renderer::FreeVBO(ebo_);
    for (const auto& texture : textures_) {
        Texture::Delete(texture);
    }
}

void SceneriesRenderer::Render(glm::mat4 transform,
                               int target_level,
                               const std::vector<PMSScenery>& scenery_instances)
{
    shader_.Use();
    Renderer::SetupVertexArray(vbo_, ebo_);

    for (unsigned int i = 0; i < scenery_instances.size(); ++i) {
        if (scenery_instances[i].level != target_level) {
            continue;
        }

        glm::mat4 current_scenery_transform = transform;

        current_scenery_transform =
          glm::translate(current_scenery_transform,
                         glm::vec3(scenery_instances[i].x, -scenery_instances[i].y, 0.0));
        current_scenery_transform = glm::rotate(
          current_scenery_transform, scenery_instances[i].rotation, glm::vec3(0.0, 0.0, 1.0));
        current_scenery_transform =
          glm::scale(current_scenery_transform,
                     glm::vec3(scenery_instances[i].scale_x, scenery_instances[i].scale_y, 0.0));

        shader_.SetMatrix4("transform", current_scenery_transform);

        Renderer::BindTexture(textures_[scenery_instances[i].style - 1]);
        Renderer::DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (i * 6ULL * sizeof(GLuint)));
    }
}

void SceneriesRenderer::OnAddScenery(const PMSScenery& new_scenery, unsigned int new_scenery_id)
{
    std::vector<float> vertices;
    GenerateGLBufferVerticesForScenery(new_scenery, vertices);
    int offset = new_scenery_id * 4 * 9 * sizeof(GLfloat);
    Renderer::ModifyVBOVertices(vbo_, vertices, offset);
}

void SceneriesRenderer::OnAddSceneryType(const PMSSceneryType& new_scenery_type)
{
    spdlog::debug("new scenery type: {}", new_scenery_type.name);
    std::filesystem::path texture_path = "scenery-gfx/";
    texture_path += new_scenery_type.name;
    if (!std::filesystem::exists(texture_path)) {
        texture_path.replace_extension(".png");
    }

    auto texture_or_error = Texture::Load(texture_path.string().c_str());

    if (texture_or_error.has_value()) {
        textures_.push_back(texture_or_error.value().opengl_id);
    } else {
        spdlog::critical("Texture file not found {}", texture_path.string());
        textures_.push_back(0);
    }
}

void SceneriesRenderer::OnRemoveScenery(const PMSScenery& removed_scenery,
                                        unsigned int removed_scenery_id,
                                        const std::vector<PMSScenery>& sceneries_after_removal)
{
    std::vector<float> vertices;
    GenerateGLBufferVertices(sceneries_after_removal, vertices);

    if (!vertices.empty()) {
        Renderer::ModifyVBOVertices(vbo_, vertices);
    }
}

void SceneriesRenderer::OnRemoveSceneryType(
  const PMSSceneryType& /*removed_scenery_type*/,
  unsigned short removed_scenery_type_id,
  const std::vector<PMSSceneryType>& /*scenery_types_after_removal*/)
{
    --removed_scenery_type_id;
    Texture::Delete(textures_.at(removed_scenery_type_id));
    textures_.erase(textures_.begin() + removed_scenery_type_id);
}

void SceneriesRenderer::GenerateGLBufferVertices(const std::vector<PMSScenery>& sceneries,
                                                 std::vector<float>& destination_vertices)
{
    for (const auto& scenery : sceneries) {
        GenerateGLBufferVerticesForScenery(scenery, destination_vertices);
    }

    for (unsigned int i = 0; i < MAX_SCENERIES_COUNT - sceneries.size(); ++i) {
        for (unsigned int j = 0; j < 4; ++j) {
            for (unsigned int k = 0; k < 9; ++k) {
                destination_vertices.push_back(0.0F);
            }
        }
    }
}

void SceneriesRenderer::GenerateGLBufferVerticesForScenery(const PMSScenery& scenery,
                                                           std::vector<float>& destination_vertices)
{
    unsigned int first_index = destination_vertices.size();

    for (unsigned int i = 0; i < 4; ++i) {
        destination_vertices.push_back(0.0);
        destination_vertices.push_back(0.0);
        destination_vertices.push_back(1.0);
        destination_vertices.push_back((float)scenery.color.red / 255.0F);
        destination_vertices.push_back((float)scenery.color.green / 255.0F);
        destination_vertices.push_back((float)scenery.color.blue / 255.0F);
        destination_vertices.push_back((float)scenery.alpha / 255.0F);
        destination_vertices.push_back(0.0);
        destination_vertices.push_back(0.0);
    }

    destination_vertices[first_index + 0] = 0.0;
    destination_vertices[first_index + 1] = (float)-scenery.height;
    destination_vertices[first_index + 7] = 0.0;
    destination_vertices[first_index + 8] = 0.0;

    destination_vertices[first_index + 9] = (float)scenery.width;
    destination_vertices[first_index + 10] = (float)-scenery.height;
    destination_vertices[first_index + 16] = 1.0;
    destination_vertices[first_index + 17] = 0.0;

    destination_vertices[first_index + 18] = 0.0;
    destination_vertices[first_index + 19] = 0.0;
    destination_vertices[first_index + 25] = 0.0;
    destination_vertices[first_index + 26] = 1.0;

    destination_vertices[first_index + 27] = (float)scenery.width;
    destination_vertices[first_index + 28] = 0.0;
    destination_vertices[first_index + 34] = 1.0;
    destination_vertices[first_index + 35] = 1.0;
}
} // namespace Soldank
