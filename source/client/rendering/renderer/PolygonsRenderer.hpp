#ifndef __POLYGONSRENDERER_HPP__
#define __POLYGONSRENDERER_HPP__

#include "rendering/shaders/Shader.hpp"

#include "core/map/Map.hpp"
#include "core/map/PMSStructs.hpp"

#include "core/math/Glm.hpp"

#include <vector>

namespace Soldank
{
class PolygonsRenderer
{
public:
    PolygonsRenderer(Map& map, const std::string& texture_name);
    ~PolygonsRenderer();

    // it's not safe to be able to copy/move this because we would also need to take care of the
    // created OpenGL buffers and textures
    PolygonsRenderer(const PolygonsRenderer&) = delete;
    PolygonsRenderer& operator=(PolygonsRenderer other) = delete;
    PolygonsRenderer(PolygonsRenderer&&) = delete;
    PolygonsRenderer& operator=(PolygonsRenderer&& other) = delete;

    void Render(glm::mat4 transform);
    void RenderSinglePolygonFirstEdge(glm::mat4 transform, const PMSPolygon& polygon) const;
    void RenderSinglePolygon(glm::mat4 transform, const PMSPolygon& polygon) const;

    glm::vec2 GetTextureDimensions() const { return texture_dimensions_; }
    unsigned int GetTextureOpenGLID() const { return texture_; };

private:
    void OnAddPolygon(const PMSPolygon& new_polygon);
    void OnRemovePolygon(const std::vector<PMSPolygon>& polygons_after_removal);
    void OnAddPolygons(const std::vector<PMSPolygon>& polygons_after_adding);
    void OnRemovePolygons(const std::vector<PMSPolygon>& polygons_after_removal);
    void OnModifyPolygons(const std::vector<PMSPolygon>& polygons_after_modify);

    static void GenerateGLBufferVertices(const std::vector<PMSPolygon>& polygons,
                                         std::vector<float>& destination_vertices);
    static void GenerateGLBufferVerticesForPolygon(const PMSPolygon& polygon,
                                                   std::vector<float>& destination_vertices);
    void LoadTexture(const std::string& texture_name);

    Shader shader_;
    unsigned int polygons_count_;

    unsigned int texture_;
    unsigned int vbo_;
    unsigned int single_polygon_vbo_;

    glm::vec2 texture_dimensions_;
};
} // namespace Soldank

#endif
