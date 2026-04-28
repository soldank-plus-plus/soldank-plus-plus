module;

#include "core/math/Glm.hpp"

#include "rendering/shaders/ShaderSources.hpp"

#include <glad/glad.h>

#include <optional>
#include <vector>

export module PolygonOutlinesRenderer;

import Renderer;
import Shader;

import Shared.Core.Map.Map;
import Shared.Core.Map.PMSConstants;
import Shared.Core.Map.PMSStructs;

export namespace Soldank
{
class PolygonOutlinesRenderer
{
public:
    PolygonOutlinesRenderer(Map& map, glm::vec4 color);
    ~PolygonOutlinesRenderer();

    // it's not safe to be able to copy/move this because we would also need to take care of the
    // created OpenGL buffers and textures
    PolygonOutlinesRenderer(const PolygonOutlinesRenderer&) = delete;
    PolygonOutlinesRenderer& operator=(PolygonOutlinesRenderer other) = delete;
    PolygonOutlinesRenderer(PolygonOutlinesRenderer&&) = delete;
    PolygonOutlinesRenderer& operator=(PolygonOutlinesRenderer&& other) = delete;

    void Render(glm::mat4 transform, unsigned int polygon_id);

private:
    void OnAddPolygon(const PMSPolygon& new_polygon);
    void OnRemovePolygon(const std::vector<PMSPolygon>& polygons_after_removal);
    void OnAddPolygons(const std::vector<PMSPolygon>& polygons_after_adding);
    void OnRemovePolygons(const std::vector<PMSPolygon>& polygons_after_removal);

    void GenerateGLBufferVertices(const std::vector<PMSPolygon>& polygons,
                                  std::vector<float>& destination_vertices) const;
    void GenerateGLBufferVerticesForPolygon(const PMSPolygon& polygon,
                                            std::vector<float>& destination_vertices) const;

    Shader shader_;
    glm::vec4 color_;

    unsigned int vbo_;
};
} // namespace Soldank

namespace Soldank
{
PolygonOutlinesRenderer::PolygonOutlinesRenderer(Map& map, glm::vec4 color)
    : shader_(ShaderSources::NO_TEXTURE_VERTEX_SHADER_SOURCE,
              ShaderSources::NO_TEXTURE_FRAGMENT_SHADER_SOURCE)
    , color_(color)
{
    std::vector<float> vertices;

    GenerateGLBufferVertices(map.GetPolygons(), vertices);

    vbo_ = Renderer::CreateVBO(vertices, GL_DYNAMIC_DRAW);

    map.GetMapChangeEvents().added_new_polygon.AddObserver(
      [this](const PMSPolygon& new_polygon) { OnAddPolygon(new_polygon); });
    map.GetMapChangeEvents().removed_polygon.AddObserver(
      [this](const PMSPolygon& /*removed_polygon*/,
             const std::vector<PMSPolygon>& polygons_after_removal) {
          OnRemovePolygon(polygons_after_removal);
      });
    map.GetMapChangeEvents().added_new_polygons.AddObserver(
      [this](const std::vector<PMSPolygon>& /*created_polygons*/,
             const std::vector<PMSPolygon>& polygons_after_adding) {
          OnAddPolygons(polygons_after_adding);
      });
    map.GetMapChangeEvents().removed_polygons.AddObserver(
      [this](const std::vector<PMSPolygon>& /*removed_polygons*/,
             const std::vector<PMSPolygon>& polygons_after_removal) {
          OnRemovePolygons(polygons_after_removal);
      });
}

PolygonOutlinesRenderer::~PolygonOutlinesRenderer()
{
    Renderer::FreeVBO(vbo_);
}

void PolygonOutlinesRenderer::Render(glm::mat4 transform, unsigned int polygon_id)
{
    shader_.Use();
    Renderer::SetupVertexArray(vbo_, std::nullopt, true, false);
    shader_.SetMatrix4("transform", transform);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // TODO: Move to Renderer if needed
    Renderer::DrawArrays(GL_TRIANGLES, (int)polygon_id * 3, 3);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void PolygonOutlinesRenderer::OnAddPolygon(const PMSPolygon& new_polygon)
{
    std::vector<float> vertices;
    GenerateGLBufferVerticesForPolygon(new_polygon, vertices);

    int offset = new_polygon.id * 7 * sizeof(GLfloat) * 3;
    Renderer::ModifyVBOVertices(vbo_, vertices, offset);
}

void PolygonOutlinesRenderer::OnRemovePolygon(const std::vector<PMSPolygon>& polygons_after_removal)
{
    std::vector<float> vertices;
    GenerateGLBufferVertices(polygons_after_removal, vertices);

    if (!vertices.empty()) {
        Renderer::ModifyVBOVertices(vbo_, vertices, 0);
    }
}

void PolygonOutlinesRenderer::OnAddPolygons(const std::vector<PMSPolygon>& polygons_after_adding)
{
    std::vector<float> vertices;
    GenerateGLBufferVertices(polygons_after_adding, vertices);

    if (!vertices.empty()) {
        Renderer::ModifyVBOVertices(vbo_, vertices, 0);
    }
}

void PolygonOutlinesRenderer::OnRemovePolygons(
  const std::vector<PMSPolygon>& polygons_after_removal)
{
    std::vector<float> vertices;
    GenerateGLBufferVertices(polygons_after_removal, vertices);

    if (!vertices.empty()) {
        Renderer::ModifyVBOVertices(vbo_, vertices, 0);
    }
}

void PolygonOutlinesRenderer::GenerateGLBufferVertices(
  const std::vector<PMSPolygon>& polygons,
  std::vector<float>& destination_vertices) const
{
    for (const auto& polygon : polygons) {
        GenerateGLBufferVerticesForPolygon(polygon, destination_vertices);
    }

    for (unsigned int i = 0; i < MAX_POLYGONS_COUNT - polygons.size(); ++i) {
        for (unsigned int j = 0; j < 3; ++j) {
            for (unsigned int k = 0; k < 7; ++k) {
                destination_vertices.push_back(0.0F);
            }
        }
    }
}

void PolygonOutlinesRenderer::GenerateGLBufferVerticesForPolygon(
  const PMSPolygon& polygon,
  std::vector<float>& destination_vertices) const
{
    for (const auto& vertex : polygon.vertices) {
        destination_vertices.push_back(vertex.x);
        destination_vertices.push_back(-vertex.y);
        destination_vertices.push_back(vertex.z);
        destination_vertices.push_back(color_.x); // red
        destination_vertices.push_back(color_.y); // green
        destination_vertices.push_back(color_.z); // blue
        destination_vertices.push_back(color_.w); // alpha
    }
}
} // namespace Soldank
