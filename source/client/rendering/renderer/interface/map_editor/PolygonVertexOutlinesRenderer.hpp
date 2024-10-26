#ifndef __POLYGON_VERTEX_OUTLINES_RENDERER_HPP__
#define __POLYGON_VERTEX_OUTLINES_RENDERER_HPP__

#include "core/map/PMSConstants.hpp"
#include "rendering/shaders/Shader.hpp"
#include "rendering/ClientState.hpp"

#include "core/map/Map.hpp"
#include "core/math/Glm.hpp"

#include <array>
#include <bitset>
#include <vector>

namespace Soldank
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

    void GenerateGLBufferVertices(const std::vector<PMSPolygon>& polygons,
                                  std::vector<float>& destination_vertices) const;
    void GenerateGLBufferVerticesForPolygon(const PMSPolygon& polygon,
                                            const std::bitset<3>& selected_vertices,
                                            std::vector<float>& destination_vertices) const;

    Shader shader_;
    glm::vec4 color_;

    unsigned int vbo_;
    unsigned int polygons_count_;

    std::array<std::bitset<3>, MAX_POLYGONS_COUNT> selected_polygon_vertices_;
};
}; // namespace Soldank

#endif
