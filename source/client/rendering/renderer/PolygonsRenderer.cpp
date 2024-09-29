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
    std::filesystem::path texture_path = "textures/" + texture_name;
    if (!std::filesystem::exists(texture_path)) {
        texture_path.replace_extension(".png");
    }

    auto texture_or_error = Texture::Load(texture_path.string().c_str());

    if (texture_or_error.has_value()) {
        texture_ = texture_or_error.value().opengl_id;
        map.SetTextureDimensions({ texture_or_error->width, texture_or_error->height });
    } else {
        texture_ = 0;
        spdlog::critical("Texture file not found {}", texture_name);
    }

    std::vector<float> vertices;

    for (const auto& polygon : map.GetPolygons()) {
        for (const auto& vertex : polygon.vertices) {
            vertices.push_back(vertex.x);
            vertices.push_back(-vertex.y);
            vertices.push_back(vertex.z);
            vertices.push_back((float)vertex.color.red / 255.0F);
            vertices.push_back((float)vertex.color.green / 255.0F);
            vertices.push_back((float)vertex.color.blue / 255.0F);
            vertices.push_back((float)vertex.color.alpha / 255.0F);
            vertices.push_back(vertex.texture_s);
            vertices.push_back(vertex.texture_t);
        }
    }

    for (unsigned int i = 0; i < MAX_POLYGONS_COUNT - map.GetPolygonsCount(); ++i) {
        for (unsigned int j = 0; j < 3; ++j) {
            for (unsigned int k = 0; k < 9; ++k) {
                vertices.push_back(0.0F);
            }
        }
    }

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
    for (const auto& vertex : polygon.vertices) {
        vertices.push_back(vertex.x);
        vertices.push_back(-vertex.y);
        vertices.push_back(vertex.z);
        vertices.push_back((float)vertex.color.red / 255.0F);
        vertices.push_back((float)vertex.color.green / 255.0F);
        vertices.push_back((float)vertex.color.blue / 255.0F);
        vertices.push_back((float)vertex.color.alpha / 255.0F);
        vertices.push_back(vertex.texture_s);
        vertices.push_back(vertex.texture_t);
    }
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
    for (const auto& vertex : polygon.vertices) {
        vertices.push_back(vertex.x);
        vertices.push_back(-vertex.y);
        vertices.push_back(vertex.z);
        vertices.push_back((float)vertex.color.red / 255.0F);
        vertices.push_back((float)vertex.color.green / 255.0F);
        vertices.push_back((float)vertex.color.blue / 255.0F);
        vertices.push_back((float)vertex.color.alpha / 255.0F);
        vertices.push_back(vertex.texture_s);
        vertices.push_back(vertex.texture_t);
    }
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
    for (const auto& vertex : new_polygon.vertices) {
        vertices.push_back(vertex.x);
        vertices.push_back(-vertex.y);
        vertices.push_back(vertex.z);
        vertices.push_back((float)vertex.color.red / 255.0F);
        vertices.push_back((float)vertex.color.green / 255.0F);
        vertices.push_back((float)vertex.color.blue / 255.0F);
        vertices.push_back((float)vertex.color.alpha / 255.0F);
        vertices.push_back(vertex.texture_s);
        vertices.push_back(vertex.texture_t);
    }

    int offset = new_polygon.id * 9 * sizeof(GLfloat) * 3;
    Renderer::ModifyVBOVertices(vbo_, vertices, offset);
    ++polygons_count_;
}
} // namespace Soldank