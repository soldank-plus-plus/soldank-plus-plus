#ifndef __SCENERIESRENDERER_HPP__
#define __SCENERIESRENDERER_HPP__

#include "rendering/shaders/Shader.hpp"

#include "core/map/Map.hpp"
#include "core/map/PMSStructs.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>

namespace Soldank
{
class SceneriesRenderer
{
public:
    SceneriesRenderer(Map& map);
    ~SceneriesRenderer();

    // it's not safe to be able to copy/move this because we would also need to take care of the
    // created OpenGL buffers and textures
    SceneriesRenderer(const SceneriesRenderer&) = delete;
    SceneriesRenderer& operator=(SceneriesRenderer other) = delete;
    SceneriesRenderer(SceneriesRenderer&&) = delete;
    SceneriesRenderer& operator=(SceneriesRenderer&& other) = delete;

    void Render(glm::mat4 transform,
                int target_level,
                const std::vector<PMSScenery>& scenery_instances);

private:
    void OnAddScenery(const PMSScenery& new_scenery, unsigned int new_scenery_id);
    void OnAddSceneryType(const PMSSceneryType& new_scenery_type);
    void OnRemoveScenery(const PMSScenery& removed_scenery,
                         unsigned int removed_scenery_id,
                         const std::vector<PMSScenery>& sceneries_after_removal);
    void OnRemoveSceneryType(const PMSSceneryType& removed_scenery_type,
                             unsigned short removed_scenery_type_id,
                             const std::vector<PMSSceneryType>& scenery_types_after_removal);
    void OnRemoveSceneryTypes(
      const std::vector<std::pair<unsigned short, PMSSceneryType>>& removed_scenery_types);
    void OnAddSceneries(const std::vector<PMSScenery>& sceneries_after_adding);
    void OnRemoveSceneries(const std::vector<PMSScenery>& sceneries_after_removal);

    static void GenerateGLBufferVertices(const std::vector<PMSScenery>& sceneries,
                                         std::vector<float>& destination_vertices);
    static void GenerateGLBufferVerticesForScenery(const PMSScenery& scenery,
                                                   std::vector<float>& destination_vertices);

    Shader shader_;

    std::vector<unsigned int> textures_;
    unsigned int vbo_;
    unsigned int ebo_;
};
} // namespace Soldank

#endif
