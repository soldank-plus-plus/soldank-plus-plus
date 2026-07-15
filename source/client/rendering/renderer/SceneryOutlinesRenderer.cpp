module;

#include "rendering/shaders/ShaderSources.hpp"

#include <glad/glad.h>

#include <optional>
#include <vector>

export module SceneryOutlinesRenderer;

import Extern.Glm;

import Renderer;
import Rendering.Gpu.GpuBuffer;
import Shader;

import Shared.Core.Map.Map;
import Shared.Core.Map.PMSConstants;
import Shared.Core.Map.PMSStructs;

export namespace Soldank
{
class SceneryOutlinesRenderer
{
public:
    SceneryOutlinesRenderer(Map& map, glm::vec4 color);
    ~SceneryOutlinesRenderer();

    // it's not safe to be able to copy/move this because we would also need to take care of the
    // created OpenGL buffers and textures
    SceneryOutlinesRenderer(const SceneryOutlinesRenderer&) = delete;
    SceneryOutlinesRenderer& operator=(SceneryOutlinesRenderer other) = delete;
    SceneryOutlinesRenderer(SceneryOutlinesRenderer&&) = delete;
    SceneryOutlinesRenderer& operator=(SceneryOutlinesRenderer&& other) = delete;

    void Render(glm::mat4 transform, unsigned int scenery_id);

private:
    void OnAddScenery(const PMSScenery& new_scenery, unsigned int new_scenery_id);
    void OnRemoveScenery(const PMSScenery& removed_scenery,
                         unsigned int removed_scenery_id,
                         const std::vector<PMSScenery>& sceneries_after_removal);
    void OnAddSceneries(const std::vector<PMSScenery>& sceneries_after_adding);
    void OnRemoveSceneries(const std::vector<PMSScenery>& sceneries_after_removal);
    void OnModifySceneries(const std::vector<PMSScenery>& sceneries_after_modify);

    void GenerateGLBufferVertices(const std::vector<PMSScenery>& sceneries,
                                  std::vector<float>& destination_vertices) const;
    void GenerateGLBufferVerticesForScenery(const PMSScenery& scenery,
                                            std::vector<float>& destination_vertices) const;

    Shader shader_;
    glm::vec4 color_;

    GpuBuffer vbo_;
};
} // namespace Soldank

namespace Soldank
{
SceneryOutlinesRenderer::SceneryOutlinesRenderer(Map& map, glm::vec4 color)
    : shader_(ShaderSources::NO_TEXTURE_VERTEX_SHADER_SOURCE,
              ShaderSources::NO_TEXTURE_FRAGMENT_SHADER_SOURCE)
    , color_(color)
{
    std::vector<float> vertices;

    GenerateGLBufferVertices(map.GetSceneryInstances(), vertices);

    vbo_ = GpuBuffer::CreateArrayBuffer(vertices, GL_DYNAMIC_DRAW);

    map.GetMapChangeEvents().added_new_scenery.AddObserver(
      [this](const PMSScenery& new_scenery, unsigned int new_scenery_id) {
          OnAddScenery(new_scenery, new_scenery_id);
      });
    map.GetMapChangeEvents().removed_scenery.AddObserver(
      [this](const PMSScenery& removed_scenery,
             unsigned int removed_scenery_id,
             const std::vector<PMSScenery>& sceneries_after_removal) {
          OnRemoveScenery(removed_scenery, removed_scenery_id, sceneries_after_removal);
      });
    map.GetMapChangeEvents().added_sceneries.AddObserver(
      [this](const std::vector<PMSScenery>& sceneries_after_adding) {
          OnAddSceneries(sceneries_after_adding);
      });
    map.GetMapChangeEvents().removed_sceneries.AddObserver(
      [this](const std::vector<PMSScenery>& sceneries_after_removal) {
          OnRemoveSceneries(sceneries_after_removal);
      });
    map.GetMapChangeEvents().modified_sceneries.AddObserver(
      [this](const std::vector<PMSScenery>& sceneries_after_modify) {
          OnModifySceneries(sceneries_after_modify);
      });
}

SceneryOutlinesRenderer::~SceneryOutlinesRenderer() {}

void SceneryOutlinesRenderer::Render(glm::mat4 transform, unsigned int scenery_id)
{
    shader_.Use();
    Renderer::SetupVertexArray(vbo_.GetId(), std::nullopt, true, false);
    shader_.SetMatrix4("transform", transform);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // TODO: Move to Renderer if needed
    Renderer::DrawArrays(GL_LINE_LOOP, (int)scenery_id * 4, 4);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void SceneryOutlinesRenderer::OnAddScenery(const PMSScenery& new_scenery,
                                           unsigned int new_scenery_id)
{
    std::vector<float> vertices;
    GenerateGLBufferVerticesForScenery(new_scenery, vertices);
    int offset = new_scenery_id * 4 * 7 * sizeof(GLfloat);
    vbo_.UpdateVertices(vertices, offset);
}

void SceneryOutlinesRenderer::OnRemoveScenery(
  const PMSScenery& removed_scenery,
  unsigned int removed_scenery_id,
  const std::vector<PMSScenery>& sceneries_after_removal)
{
    std::vector<float> vertices;
    GenerateGLBufferVertices(sceneries_after_removal, vertices);

    if (!vertices.empty()) {
        vbo_.UpdateVertices(vertices);
    }
}

void SceneryOutlinesRenderer::OnAddSceneries(const std::vector<PMSScenery>& sceneries_after_adding)
{
    std::vector<float> vertices;
    GenerateGLBufferVertices(sceneries_after_adding, vertices);
    vbo_.UpdateVertices(vertices);
}

void SceneryOutlinesRenderer::OnRemoveSceneries(
  const std::vector<PMSScenery>& sceneries_after_removal)
{
    std::vector<float> vertices;
    GenerateGLBufferVertices(sceneries_after_removal, vertices);
    vbo_.UpdateVertices(vertices);
}

void SceneryOutlinesRenderer::OnModifySceneries(
  const std::vector<PMSScenery>& sceneries_after_modify)
{
    std::vector<float> vertices;
    GenerateGLBufferVertices(sceneries_after_modify, vertices);
    vbo_.UpdateVertices(vertices);
}

void SceneryOutlinesRenderer::GenerateGLBufferVertices(
  const std::vector<PMSScenery>& sceneries,
  std::vector<float>& destination_vertices) const
{
    for (const auto& polygon : sceneries) {
        GenerateGLBufferVerticesForScenery(polygon, destination_vertices);
    }

    for (unsigned int i = 0; i < MAX_SCENERIES_COUNT - sceneries.size(); ++i) {
        for (unsigned int j = 0; j < 4; ++j) {
            for (unsigned int k = 0; k < 7; ++k) {
                destination_vertices.push_back(0.0F);
            }
        }
    }
}

void SceneryOutlinesRenderer::GenerateGLBufferVerticesForScenery(
  const PMSScenery& scenery,
  std::vector<float>& destination_vertices) const
{
    auto vertex_positions = Map::GetSceneryVertexPositions(scenery);

    auto push_vertex = [&](std::vector<float>& destination_vertices,
                           const glm::vec2& vertex_position) {
        destination_vertices.push_back(vertex_position.x);
        destination_vertices.push_back(-vertex_position.y);
        destination_vertices.push_back(1.0F);
        destination_vertices.push_back(color_.x); // red
        destination_vertices.push_back(color_.y); // green
        destination_vertices.push_back(color_.z); // blue
        destination_vertices.push_back(color_.w); // alpha
    };

    push_vertex(destination_vertices, vertex_positions.at(0));
    push_vertex(destination_vertices, vertex_positions.at(1));
    push_vertex(destination_vertices, vertex_positions.at(2));
    push_vertex(destination_vertices, vertex_positions.at(3));
}
} // namespace Soldank
