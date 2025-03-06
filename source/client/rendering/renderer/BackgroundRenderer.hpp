#ifndef __BACKGROUNDRENDERER_HPP__
#define __BACKGROUNDRENDERER_HPP__

#include "rendering/shaders/Shader.hpp"

#include "core/map/Map.hpp"
#include "core/math/Glm.hpp"

#include <vector>
#include <span>

namespace Soldank
{
class BackgroundRenderer
{
public:
    BackgroundRenderer(Map& map);
    ~BackgroundRenderer();

    // it's not safe to be able to copy/move this because we would also need to take care of the
    // created OpenGL buffers and textures
    BackgroundRenderer(const BackgroundRenderer&) = delete;
    BackgroundRenderer& operator=(BackgroundRenderer other) = delete;
    BackgroundRenderer(BackgroundRenderer&&) = delete;
    BackgroundRenderer& operator=(BackgroundRenderer&& other) = delete;

    void Render(glm::mat4 transform);

private:
    void OnChangeBackgroundColor(const PMSColor& top_color,
                                 const PMSColor& bottom_color,
                                 std::span<const float, 4> boundaries);
    static void GenerateGLBufferVertices(PMSColor background_top_color,
                                         PMSColor background_bottom_color,
                                         std::span<const float, 4> boundaries,
                                         std::vector<float>& destination_vertices);

    Shader shader_;

    unsigned int vbo_;
};
} // namespace Soldank

#endif
