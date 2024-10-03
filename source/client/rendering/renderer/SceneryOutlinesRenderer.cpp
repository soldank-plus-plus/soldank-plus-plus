#include "rendering/renderer/SceneryOutlinesRenderer.hpp"

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
    Renderer::DrawArrays(GL_TRIANGLES, (int)scenery_id * 6, 6);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void SceneryOutlinesRenderer::GenerateGLBufferVertices(
  const std::vector<PMSScenery>& sceneries,
  std::vector<float>& destination_vertices) const
{
    for (const auto& polygon : sceneries) {
        GenerateGLBufferVerticesForScenery(polygon, destination_vertices);
    }

    for (unsigned int i = 0; i < MAX_SCENERIES_COUNT - sceneries.size(); ++i) {
        for (unsigned int j = 0; j < 6; ++j) {
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
    glm::mat4 transform_matrix(1.0F);
    transform_matrix = glm::rotate(transform_matrix, scenery.rotation, glm::vec3(0.0, 0.0, 1.0));
    transform_matrix =
      glm::scale(transform_matrix, glm::vec3(scenery.scale_x, scenery.scale_y, 0.0));

    std::array<glm::vec4, 4> vertex_positions{ glm::vec4{ 0.0F, 0.0F, 1.0F, 1.0F },
                                               glm::vec4{ scenery.width, 0.0F, 1.0F, 1.0F },
                                               glm::vec4{ 0.0F, -scenery.height, 1.0F, 1.0F },
                                               glm::vec4{
                                                 scenery.width, -scenery.height, 1.0F, 1.0F } };

    for (auto& vertex_position : vertex_positions) {
        vertex_position = transform_matrix * vertex_position;
        vertex_position.x += scenery.x;
        vertex_position.y -= scenery.y;
    }

    auto push_vertex = [&](std::vector<float>& destination_vertices,
                           const glm::vec4& vertex_position) {
        destination_vertices.push_back(vertex_position.x);
        destination_vertices.push_back(vertex_position.y);
        destination_vertices.push_back(1.0F);
        destination_vertices.push_back(color_.x); // red
        destination_vertices.push_back(color_.y); // green
        destination_vertices.push_back(color_.z); // blue
        destination_vertices.push_back(color_.w); // alpha
    };

    push_vertex(destination_vertices, vertex_positions.at(0));
    push_vertex(destination_vertices, vertex_positions.at(1));
    push_vertex(destination_vertices, vertex_positions.at(2));
    push_vertex(destination_vertices, vertex_positions.at(1));
    push_vertex(destination_vertices, vertex_positions.at(2));
    push_vertex(destination_vertices, vertex_positions.at(3));
}
} // namespace Soldank
