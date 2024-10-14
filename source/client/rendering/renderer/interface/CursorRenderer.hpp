#ifndef __CURSORRENDERER_HPP__
#define __CURSORRENDERER_HPP__

#include "rendering/ClientState.hpp"
#include "rendering/shaders/Shader.hpp"

#include "core/math/Glm.hpp"

namespace Soldank
{
class CursorRenderer
{
public:
    CursorRenderer(ClientState& client_state);
    ~CursorRenderer();

    // it's not safe to be able to copy/move this because we would also need to take care of the
    // created OpenGL buffers and textures
    CursorRenderer(const CursorRenderer&) = delete;
    CursorRenderer& operator=(CursorRenderer other) = delete;
    CursorRenderer(CursorRenderer&&) = delete;
    CursorRenderer& operator=(CursorRenderer&& other) = delete;

    void Render(const glm::vec2& mouse_position, glm::vec2 window_dimensions);

private:
    constexpr static const float SIZE_SCALE = 0.23;

    void GenerateGLBufferVertices(glm::vec2 window_dimensions,
                                  std::vector<float>& destination_vertices) const;
    void OnWindowResized(glm::vec2 new_window_dimensions);

    Shader shader_;

    unsigned int texture_;
    int texture_width_;
    int texture_height_;
    unsigned int vbo_;
    unsigned int ebo_;
};
} // namespace Soldank

#endif
