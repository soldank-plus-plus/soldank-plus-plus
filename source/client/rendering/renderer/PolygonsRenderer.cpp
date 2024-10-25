#include "PolygonsRenderer.hpp"

#include "rendering/data/Texture.hpp"
#include "rendering/renderer/Renderer.hpp"
#include "rendering/shaders/ShaderSources.hpp"

#include "spdlog/spdlog.h"

#include <filesystem>

namespace Soldank
{
PolygonsRenderer::PolygonsRenderer(Map& map, const std::string& texture_name)
    : shader_(ShaderSources::VERTEX_SHADER_SOURCE, ShaderSources::FRAGMENT_SHADER_SOURCE)
    , polygons_count_(map.GetPolygonsCount())
{
    LoadTexture(map.GetTextureName());

    std::vector<float> vertices;

    GenerateGLBufferVertices(map.GetPolygons(), vertices);

    vbo_ = Renderer::CreateVBO(vertices, GL_DYNAMIC_DRAW);

    vertices.clear();
    for (unsigned int i = 0; i < 3; ++i) {
        for (unsigned int j = 0; j < 9; ++j) {
            vertices.push_back(0.0F);
        }
    }
    single_polygon_vbo_ = Renderer::CreateVBO(vertices, GL_DYNAMIC_DRAW);

    map.GetMapChangeEvents().added_new_polygon.AddObserver(
      [this](const PMSPolygon& new_polygon) { OnAddPolygon(new_polygon); });
    map.GetMapChangeEvents().removed_polygon.AddObserver(
      [this](const PMSPolygon& /*removed_polygon*/,
             const std::vector<PMSPolygon>& polygons_after_removal) {
          OnRemovePolygon(polygons_after_removal);
      });
    map.GetMapChangeEvents().changed_texture_name.AddObserver(
      [this](const std::string& texture_name) {
          Texture::Delete(texture_);
          LoadTexture(texture_name);
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

PolygonsRenderer::~PolygonsRenderer()
{
    Renderer::FreeVBO(vbo_);
    Renderer::FreeVBO(single_polygon_vbo_);
    Texture::Delete(texture_);
}

void PolygonsRenderer::Render(glm::mat4 transform)
{
    Renderer::SetupVertexArray(vbo_, std::nullopt);
    shader_.Use();
    Renderer::BindTexture(texture_);
    shader_.SetMatrix4("transform", transform);
    Renderer::DrawArrays(GL_TRIANGLES, 0, (int)polygons_count_ * 3);
}

void PolygonsRenderer::RenderSinglePolygonFirstEdge(glm::mat4 transform,
                                                    const PMSPolygon& polygon) const
{
    std::vector<float> vertices;
    GenerateGLBufferVerticesForPolygon(polygon, vertices);
    Renderer::ModifyVBOVertices(single_polygon_vbo_, vertices);

    Renderer::SetupVertexArray(single_polygon_vbo_, std::nullopt);
    shader_.Use();
    Renderer::BindTexture(texture_);
    shader_.SetMatrix4("transform", transform);
    Renderer::DrawArrays(GL_LINE_STRIP, 0, 2);
}

void PolygonsRenderer::RenderSinglePolygon(glm::mat4 transform, const PMSPolygon& polygon) const
{
    std::vector<float> vertices;
    GenerateGLBufferVerticesForPolygon(polygon, vertices);
    Renderer::ModifyVBOVertices(single_polygon_vbo_, vertices);

    Renderer::SetupVertexArray(single_polygon_vbo_, std::nullopt);
    shader_.Use();
    Renderer::BindTexture(texture_);
    shader_.SetMatrix4("transform", transform);
    Renderer::DrawArrays(GL_TRIANGLES, 0, 3);
}

void PolygonsRenderer::OnAddPolygon(const PMSPolygon& new_polygon)
{
    std::vector<float> vertices;
    GenerateGLBufferVerticesForPolygon(new_polygon, vertices);

    int offset = new_polygon.id * 9 * sizeof(GLfloat) * 3;
    Renderer::ModifyVBOVertices(vbo_, vertices, offset);
    ++polygons_count_;
}

void PolygonsRenderer::OnRemovePolygon(const std::vector<PMSPolygon>& polygons_after_removal)
{
    std::vector<float> vertices;
    GenerateGLBufferVertices(polygons_after_removal, vertices);
    polygons_count_ = vertices.size();

    if (!vertices.empty()) {
        Renderer::ModifyVBOVertices(vbo_, vertices, 0);
    }
}

void PolygonsRenderer::OnAddPolygons(const std::vector<PMSPolygon>& polygons_after_adding)
{
    std::vector<float> vertices;
    GenerateGLBufferVertices(polygons_after_adding, vertices);
    Renderer::ModifyVBOVertices(vbo_, vertices);
    polygons_count_ = polygons_after_adding.size();
}

void PolygonsRenderer::OnRemovePolygons(const std::vector<PMSPolygon>& polygons_after_removal)
{
    std::vector<float> vertices;
    GenerateGLBufferVertices(polygons_after_removal, vertices);
    Renderer::ModifyVBOVertices(vbo_, vertices);
    polygons_count_ = polygons_after_removal.size();
}

void PolygonsRenderer::OnModifyPolygons(const std::vector<PMSPolygon>& polygons_after_modify)
{
    std::vector<float> vertices;
    GenerateGLBufferVertices(polygons_after_modify, vertices);
    Renderer::ModifyVBOVertices(vbo_, vertices);
    polygons_count_ = polygons_after_modify.size();
}

void PolygonsRenderer::GenerateGLBufferVertices(const std::vector<PMSPolygon>& polygons,
                                                std::vector<float>& destination_vertices)
{
    for (const auto& polygon : polygons) {
        GenerateGLBufferVerticesForPolygon(polygon, destination_vertices);
    }

    for (unsigned int i = 0; i < MAX_POLYGONS_COUNT - polygons.size(); ++i) {
        for (unsigned int j = 0; j < 3; ++j) {
            for (unsigned int k = 0; k < 9; ++k) {
                destination_vertices.push_back(0.0F);
            }
        }
    }
}

void PolygonsRenderer::GenerateGLBufferVerticesForPolygon(const PMSPolygon& polygon,
                                                          std::vector<float>& destination_vertices)
{
    for (const auto& vertex : polygon.vertices) {
        destination_vertices.push_back(vertex.x);
        destination_vertices.push_back(-vertex.y);
        destination_vertices.push_back(vertex.z);
        destination_vertices.push_back((float)vertex.color.red / 255.0F);
        destination_vertices.push_back((float)vertex.color.green / 255.0F);
        destination_vertices.push_back((float)vertex.color.blue / 255.0F);
        destination_vertices.push_back((float)vertex.color.alpha / 255.0F);
        destination_vertices.push_back(vertex.texture_s);
        destination_vertices.push_back(vertex.texture_t);
    }
}

void PolygonsRenderer::LoadTexture(const std::string& texture_name)
{
    std::filesystem::path texture_path = "textures/" + texture_name;
    if (!std::filesystem::exists(texture_path)) {
        texture_path.replace_extension(".png");
    }

    auto texture_or_error = Texture::Load(texture_path.string().c_str());

    if (texture_or_error.has_value()) {
        texture_ = texture_or_error.value().opengl_id;
        texture_dimensions_ = { texture_or_error->width, texture_or_error->height };
    } else {
        texture_ = 0;
        texture_dimensions_ = { 0, 0 };
        spdlog::critical("Texture file not found {}", texture_name);
    }
}
} // namespace Soldank