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
    for (const auto& scenery_type : map.GetSceneryTypes()) {
        AddNewTexture(scenery_type.name);
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
    map.GetMapChangeEvents().added_sceneries.AddObserver(
      [this](const std::vector<PMSScenery>& sceneries_after_adding) {
          OnAddSceneries(sceneries_after_adding);
      });
    map.GetMapChangeEvents().removed_sceneries.AddObserver(
      [this](const std::vector<PMSScenery>& sceneries_after_removal) {
          OnRemoveSceneries(sceneries_after_removal);
      });
    map.GetMapChangeEvents().removed_scenery_types.AddObserver(
      [this](const std::vector<std::pair<unsigned short, PMSSceneryType>>& removed_scenery_types) {
          OnRemoveSceneryTypes(removed_scenery_types);
      });
    map.GetMapChangeEvents().modified_sceneries.AddObserver(
      [this](const std::vector<PMSScenery>& sceneries_after_modify) {
          OnModifySceneries(sceneries_after_modify);
      });
}

SceneriesRenderer::~SceneriesRenderer()
{
    Renderer::FreeVBO(vbo_);
    Renderer::FreeVBO(ebo_);
    for (const auto& texture : textures_) {
        if (std::holds_alternative<unsigned int>(texture)) {
            Texture::Delete(std::get<unsigned int>(texture));
        }
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

        if (std::holds_alternative<unsigned int>(textures_[scenery_instances[i].style - 1])) {
            Renderer::BindTexture(std::get<0>(textures_[scenery_instances[i].style - 1]));
        } else {
            auto& gif_texture = std::get<1>(textures_[scenery_instances[i].style - 1]);
            gif_texture.Update();
            Renderer::BindTexture(gif_texture.GetTextureId());
        }

        Renderer::DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (i * 6ULL * sizeof(GLuint)));
    }
}

void SceneriesRenderer::AddNewTexture(const std::filesystem::path& texture_file_name)
{
    std::filesystem::path texture_path = "scenery-gfx/";
    texture_path += texture_file_name;

    if (texture_path.extension() == ".gif") {
        auto texture_or_error = Texture::LoadGIF(texture_path.string().c_str());

        if (texture_or_error.has_value()) {
            GIFTexture gif_texture(texture_or_error.value());
            textures_.emplace_back(std::move(gif_texture));
        } else {
            spdlog::critical("Texture file not found {}", texture_path.string());
            textures_.emplace_back(0U);
        }
    } else {
        if (!std::filesystem::exists(texture_path)) {
            texture_path.replace_extension(".png");
        }
        auto texture_or_error = Texture::Load(texture_path.string().c_str());
        if (texture_or_error.has_value()) {
            textures_.emplace_back(texture_or_error.value().opengl_id);
        } else {
            spdlog::critical("Texture file not found {}", texture_path.string());
            textures_.emplace_back(0U);
        }
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
    AddNewTexture(new_scenery_type.name);
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
    if (std::holds_alternative<unsigned int>(textures_.at(removed_scenery_type_id))) {
        Texture::Delete(std::get<unsigned int>(textures_.at(removed_scenery_type_id)));
    }
    textures_.erase(textures_.begin() + removed_scenery_type_id);
}

void SceneriesRenderer::OnRemoveSceneryTypes(
  const std::vector<std::pair<unsigned short, PMSSceneryType>>& removed_scenery_types)
{
    std::vector<unsigned int> indexes_to_remove;
    for (const auto& removed_scenery_type : removed_scenery_types) {
        indexes_to_remove.push_back(removed_scenery_type.first - 1);
        if (std::holds_alternative<unsigned int>(textures_.at(removed_scenery_type.first - 1))) {
            Texture::Delete(std::get<unsigned int>(textures_.at(removed_scenery_type.first - 1)));
        }
    }

    std::sort(indexes_to_remove.begin(), indexes_to_remove.end());
    std::vector<std::variant<unsigned int, GIFTexture>> new_textures;

    unsigned int removal_id = 0;
    for (unsigned int i = 0; i < textures_.size(); ++i) {
        if (removal_id < indexes_to_remove.size() && indexes_to_remove.at(removal_id) == i) {

            ++removal_id;
            continue;
        }

        new_textures.push_back(textures_.at(i));
    }

    textures_ = new_textures;
}

void SceneriesRenderer::OnAddSceneries(const std::vector<PMSScenery>& sceneries_after_adding)
{
    std::vector<float> vertices;
    GenerateGLBufferVertices(sceneries_after_adding, vertices);
    Renderer::ModifyVBOVertices(vbo_, vertices);
}

void SceneriesRenderer::OnRemoveSceneries(const std::vector<PMSScenery>& sceneries_after_removal)
{
    std::vector<float> vertices;
    GenerateGLBufferVertices(sceneries_after_removal, vertices);
    Renderer::ModifyVBOVertices(vbo_, vertices);
}

void SceneriesRenderer::OnModifySceneries(const std::vector<PMSScenery>& sceneries_after_modify)
{
    std::vector<float> vertices;
    GenerateGLBufferVertices(sceneries_after_modify, vertices);
    Renderer::ModifyVBOVertices(vbo_, vertices);
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

SceneriesRenderer::GIFTexture::GIFTexture(const std::shared_ptr<Texture::TextureGIFData>& gif_data)
    : gif_data_(gif_data)
    , current_frame_id_(0)
    , last_switch_time_(std::chrono::system_clock::now())
{
}

void SceneriesRenderer::GIFTexture::Update()
{
    auto time_now = std::chrono::system_clock::now();
    std::chrono::duration<double> delta_time = time_now - last_switch_time_;
    unsigned int next_frame_id = (current_frame_id_ + 1) % gif_data_->Size();
    double delay = gif_data_->GetFrame(current_frame_id_).second;
    delay /= 1000.0;
    if (delta_time.count() >= delay) {
        current_frame_id_ = next_frame_id;
        last_switch_time_ = time_now;
    }
}

unsigned int SceneriesRenderer::GIFTexture::GetTextureId() const
{
    return gif_data_->GetFrame(current_frame_id_).first;
}
} // namespace Soldank
