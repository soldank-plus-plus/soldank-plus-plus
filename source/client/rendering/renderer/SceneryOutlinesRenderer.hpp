#ifndef __SCENERY_OUTLINES_RENDERER_HPP__
#define __SCENERY_OUTLINES_RENDERER_HPP__

#include "core/map/PMSStructs.hpp"
#include "rendering/shaders/Shader.hpp"

#include "core/map/Map.hpp"

#include "core/math/Glm.hpp"

#include <vector>

namespace Soldank
{
class SceneryOutlinesRenderer
{
public:
    SceneryOutlinesRenderer(Map& map, glm::vec4 color);
    ~SceneryOutlinesRenderer();

    // it's not safe to be able to copy/move this because we would also need to take care of the
    // created OpenGL buffers and textures
    SceneryOutlinesRenderer(const SceneryOutlinesRenderer&) = delete;
    SceneryOutlinesRenderer& operator=(SceneryOutlinesRenderer other) = delete;
    SceneryOutlinesRenderer(SceneryOutlinesRenderer&&) = delete;
    SceneryOutlinesRenderer& operator=(SceneryOutlinesRenderer&& other) = delete;

    void Render(glm::mat4 transform, unsigned int scenery_id);

private:
    void OnAddScenery(const PMSScenery& new_scenery, unsigned int new_scenery_id);
    void OnRemoveScenery(const PMSScenery& removed_scenery,
                         unsigned int removed_scenery_id,
                         const std::vector<PMSScenery>& sceneries_after_removal);
    void OnAddSceneries(const std::vector<PMSScenery>& sceneries_after_adding);
    void OnRemoveSceneries(const std::vector<PMSScenery>& sceneries_after_removal);

    void GenerateGLBufferVertices(const std::vector<PMSScenery>& sceneries,
                                  std::vector<float>& destination_vertices) const;
    void GenerateGLBufferVerticesForScenery(const PMSScenery& scenery,
                                            std::vector<float>& destination_vertices) const;

    Shader shader_;
    glm::vec4 color_;

    unsigned int vbo_;
};
} // namespace Soldank

#endif
