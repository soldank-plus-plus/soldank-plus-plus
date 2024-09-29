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

private:
    void OnAddPolygon(const PMSPolygon& new_polygon);

    Shader shader_;
    unsigned int polygons_count_;

    unsigned int texture_;
    unsigned int vbo_;
    unsigned int single_polygon_vbo_;
};
} // namespace Soldank

#endif
