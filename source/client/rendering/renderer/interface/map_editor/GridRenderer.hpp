#ifndef __GRID_RENDERER_HPP__
#define __GRID_RENDERER_HPP__

#include "rendering/shaders/Shader.hpp"

namespace Soldank
{
class GridRenderer
{
public:
    GridRenderer();
    ~GridRenderer();

    // it's not safe to be able to copy/move this because we would also need to take care of the
    // created OpenGL buffers and textures
    GridRenderer(const GridRenderer&) = delete;
    GridRenderer& operator=(GridRenderer other) = delete;
    GridRenderer(GridRenderer&&) = delete;
    GridRenderer& operator=(GridRenderer&& other) = delete;

    void Render(glm::vec2 window_dimensions, glm::vec2 view_position, float view_zoom);

private:
    Shader shader_;

    unsigned int vbo_;
};
} // namespace Soldank

#endif
