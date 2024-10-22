#include "rendering/renderer/SceneryOutlinesRenderer.hpp"

#include "core/map/Map.hpp"
#include "core/map/PMSConstants.hpp"
#include "rendering/renderer/Renderer.hpp"
#include "rendering/shaders/ShaderSources.hpp"

namespace Soldank
{
SceneryOutlinesRenderer::SceneryOutlinesRenderer(Map& map, glm::vec4 color)
    : shader_(ShaderSources::NO_TEXTURE_VERTEX_SHADER_SOURCE,
              ShaderSources::NO_TEXTURE_FRAGMENT_SHADER_SOURCE)
    , color_(color)
{
    std::vector<float> vertices;

    GenerateGLBufferVertices(map.GetSceneryInstances(), vertices);

    vbo_ = Renderer::CreateVBO(vertices, GL_DYNAMIC_DRAW);

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
}

SceneryOutlinesRenderer::~SceneryOutlinesRenderer()
{
    Renderer::FreeVBO(vbo_);
}

void SceneryOutlinesRenderer::Render(glm::mat4 transform, unsigned int scenery_id)
{
    shader_.Use();
    Renderer::SetupVertexArray(vbo_, std::nullopt, true, false);
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
    Renderer::ModifyVBOVertices(vbo_, vertices, offset);
}

void SceneryOutlinesRenderer::OnRemoveScenery(
  const PMSScenery& removed_scenery,
  unsigned int removed_scenery_id,
  const std::vector<PMSScenery>& sceneries_after_removal)
{
    std::vector<float> vertices;
    GenerateGLBufferVertices(sceneries_after_removal, vertices);

    if (!vertices.empty()) {
        Renderer::ModifyVBOVertices(vbo_, vertices);
    }
}

void SceneryOutlinesRenderer::OnAddSceneries(const std::vector<PMSScenery>& sceneries_after_adding)
{
    std::vector<float> vertices;
    GenerateGLBufferVertices(sceneries_after_adding, vertices);
    Renderer::ModifyVBOVertices(vbo_, vertices);
}

void SceneryOutlinesRenderer::OnRemoveSceneries(
  const std::vector<PMSScenery>& sceneries_after_removal)
{
    std::vector<float> vertices;
    GenerateGLBufferVertices(sceneries_after_removal, vertices);
    Renderer::ModifyVBOVertices(vbo_, vertices);
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
