module;

#include "rendering/shaders/ShaderSources.hpp"

#include <glad/glad.h>

#include <array>
#include <bitset>
#include <optional>
#include <vector>

export module PolygonVertexOutlinesRenderer;

import Extern.Glm;

import Renderer;
import Rendering.Gpu.GpuBuffer;
import Shader;
import ClientState;

import Shared.Core.Map.PMSConstants;
import Shared.Core.Map.PMSStructs;
import Shared.Core.Map.Map;

export namespace Soldank
{
class PolygonVertexOutlinesRenderer
{
public:
    PolygonVertexOutlinesRenderer(ClientState& client_state, Map& map, glm::vec4 color);
    ~PolygonVertexOutlinesRenderer();

    // it's not safe to be able to copy/move this because we would also need to take care of the
    // created OpenGL buffers and textures
    PolygonVertexOutlinesRenderer(const PolygonVertexOutlinesRenderer&) = delete;
    PolygonVertexOutlinesRenderer& operator=(PolygonVertexOutlinesRenderer other) = delete;
    PolygonVertexOutlinesRenderer(PolygonVertexOutlinesRenderer&&) = delete;
    PolygonVertexOutlinesRenderer& operator=(PolygonVertexOutlinesRenderer&& other) = delete;

    void Render(glm::mat4 transform);

private:
    void OnChangePolygonVertexSelection(const PMSPolygon& polygon,
                                        const std::bitset<3>& selected_vertices);
    void OnAddPolygon(const PMSPolygon& new_polygon);
    void OnRemovePolygon(const PMSPolygon& removed_polygon);
    void OnAddPolygons(const std::vector<PMSPolygon>& polygons_after_adding);
    void OnRemovePolygons(const std::vector<PMSPolygon>& polygons_after_removal);
    void OnModifyPolygons(const std::vector<PMSPolygon>& polygons_after_modify);

    void GenerateGLBufferVertices(const std::vector<PMSPolygon>& polygons,
                                  std::vector<float>& destination_vertices) const;
    void GenerateGLBufferVerticesForPolygon(const PMSPolygon& polygon,
                                            const std::bitset<3>& selected_vertices,
                                            std::vector<float>& destination_vertices) const;

    Shader shader_;
    glm::vec4 color_;

    GpuBuffer vbo_;
    unsigned int polygons_count_;

    std::array<std::bitset<3>, MAX_POLYGONS_COUNT> selected_polygon_vertices_;
};
}; // namespace Soldank

namespace Soldank
{
PolygonVertexOutlinesRenderer::PolygonVertexOutlinesRenderer(ClientState& client_state,
                                                             Map& map,
                                                             glm::vec4 color)
    : shader_(ShaderSources::NO_TEXTURE_VERTEX_SHADER_SOURCE,
              ShaderSources::NO_TEXTURE_FRAGMENT_SHADER_SOURCE)
    , color_(color)
    , polygons_count_(map.GetPolygonsCount())
{
    std::vector<float> vertices;

    GenerateGLBufferVertices(map.GetPolygons(), vertices);

    vbo_ = GpuBuffer::CreateArrayBuffer(vertices, GL_DYNAMIC_DRAW);

    client_state.map_editor_state.event_polygon_selected.AddObserver(
      [this](const PMSPolygon& polygon, const std::bitset<3>& selected_vertices) {
          OnChangePolygonVertexSelection(polygon, selected_vertices);
      });
    map.GetMapChangeEvents().added_new_polygon.AddObserver(
      [this](const PMSPolygon& new_polygon) { OnAddPolygon(new_polygon); });
    map.GetMapChangeEvents().removed_polygon.AddObserver(
      [this](const PMSPolygon& removed_polygon,
             const std::vector<PMSPolygon>& /*polygons_after_removal*/) {
          OnRemovePolygon(removed_polygon);
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
    map.GetMapChangeEvents().modified_polygons.AddObserver(
      [this](const std::vector<PMSPolygon>& polygons_after_modify) {
          OnModifyPolygons(polygons_after_modify);
      });
}

PolygonVertexOutlinesRenderer::~PolygonVertexOutlinesRenderer() {}

void PolygonVertexOutlinesRenderer::Render(glm::mat4 transform)
{
    shader_.Use();
    Renderer::SetupVertexArray(vbo_.GetId(), std::nullopt, true, false);
    shader_.SetMatrix4("transform", transform);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // TODO: Move to Renderer if needed
    Renderer::DrawArrays(GL_TRIANGLES, 0, (int)polygons_count_ * 3);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void PolygonVertexOutlinesRenderer::OnChangePolygonVertexSelection(
  const PMSPolygon& polygon,
  const std::bitset<3>& selected_vertices)
{
    std::vector<float> vertices;
    GenerateGLBufferVerticesForPolygon(polygon, selected_vertices, vertices);

    int offset = polygon.id * 7 * sizeof(GLfloat) * 3;
    vbo_.UpdateVertices(vertices, offset);

    selected_polygon_vertices_.at(polygon.id) = selected_vertices;
}

void PolygonVertexOutlinesRenderer::OnAddPolygon(const PMSPolygon& new_polygon)
{
    std::vector<float> vertices;
    GenerateGLBufferVerticesForPolygon(new_polygon, { 0b000 }, vertices);
    if (!vertices.empty()) {
        int offset = new_polygon.id * 7 * sizeof(GLfloat) * 3;
        vbo_.UpdateVertices(vertices, offset);
    }

    selected_polygon_vertices_.at(new_polygon.id) = { 0b000 };

    ++polygons_count_;
}

void PolygonVertexOutlinesRenderer::OnRemovePolygon(const PMSPolygon& removed_polygon)
{
    std::vector<float> vertices;
    GenerateGLBufferVerticesForPolygon(removed_polygon, { 0b000 }, vertices);

    if (!vertices.empty()) {
        int offset = removed_polygon.id * 7 * sizeof(GLfloat) * 3;
        vbo_.UpdateVertices(vertices, offset);
    }

    selected_polygon_vertices_.at(removed_polygon.id) = { 0b000 };

    --polygons_count_;
}

void PolygonVertexOutlinesRenderer::OnAddPolygons(
  const std::vector<PMSPolygon>& polygons_after_adding)
{
    std::vector<float> vertices;
    GenerateGLBufferVertices(polygons_after_adding, vertices);
    vbo_.UpdateVertices(vertices);
    for (unsigned int i = polygons_count_; i < polygons_after_adding.size(); ++i) {
        selected_polygon_vertices_.at(i) = { 0b000 };
    }
    polygons_count_ = polygons_after_adding.size();
}

void PolygonVertexOutlinesRenderer::OnRemovePolygons(
  const std::vector<PMSPolygon>& polygons_after_removal)
{
    std::vector<float> vertices;
    GenerateGLBufferVertices(polygons_after_removal, vertices);
    vbo_.UpdateVertices(vertices);
    polygons_count_ = polygons_after_removal.size();
}

void PolygonVertexOutlinesRenderer::OnModifyPolygons(
  const std::vector<PMSPolygon>& polygons_after_modify)
{
    std::vector<float> vertices;
    GenerateGLBufferVertices(polygons_after_modify, vertices);
    vbo_.UpdateVertices(vertices);
    polygons_count_ = polygons_after_modify.size();
}

void PolygonVertexOutlinesRenderer::GenerateGLBufferVertices(
  const std::vector<PMSPolygon>& polygons,
  std::vector<float>& destination_vertices) const
{
    for (const auto& polygon : polygons) {
        GenerateGLBufferVerticesForPolygon(
          polygon, selected_polygon_vertices_.at(polygon.id), destination_vertices);
    }

    for (unsigned int i = 0; i < MAX_POLYGONS_COUNT - polygons.size(); ++i) {
        for (unsigned int j = 0; j < 3; ++j) {
            for (unsigned int k = 0; k < 7; ++k) {
                destination_vertices.push_back(0.0F);
            }
        }
    }
}

void PolygonVertexOutlinesRenderer::GenerateGLBufferVerticesForPolygon(
  const PMSPolygon& polygon,
  const std::bitset<3>& selected_vertices,
  std::vector<float>& destination_vertices) const
{
    for (unsigned int i = 0; i < polygon.vertices.size(); ++i) {
        destination_vertices.push_back(polygon.vertices.at(i).x);
        destination_vertices.push_back(-polygon.vertices.at(i).y);
        destination_vertices.push_back(polygon.vertices.at(i).z);
        destination_vertices.push_back(color_.x);                               // red
        destination_vertices.push_back(color_.y);                               // green
        destination_vertices.push_back(color_.z);                               // blue
        destination_vertices.push_back(selected_vertices[i] ? color_.w : 0.0F); // alpha
    }
}
}; // namespace Soldank
