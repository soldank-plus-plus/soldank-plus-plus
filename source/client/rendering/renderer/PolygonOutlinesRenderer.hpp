#ifndef __POLYGONOUTLINESRENDERER_HPP__
#define __POLYGONOUTLINESRENDERER_HPP__

#include "rendering/shaders/Shader.hpp"

#include "core/map/Map.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>

namespace Soldank
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

    void GenerateGLBufferVertices(const std::vector<PMSPolygon>& polygons,
                                  std::vector<float>& destination_vertices) const;
    void GenerateGLBufferVerticesForPolygon(const PMSPolygon& polygon,
                                            std::vector<float>& destination_vertices) const;

    Shader shader_;
    glm::vec4 color_;

    unsigned int vbo_;
};
} // namespace Soldank

#endif
