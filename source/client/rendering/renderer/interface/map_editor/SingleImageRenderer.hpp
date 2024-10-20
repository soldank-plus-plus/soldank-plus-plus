#ifndef __SINGLE_IMAGE_RENDERER_HPP__
#define __SINGLE_IMAGE_RENDERER_HPP__

#include "rendering/shaders/Shader.hpp"

#include "core/math/Glm.hpp"

#include <vector>

namespace Soldank
{
class SingleImageRenderer
{
public:
    SingleImageRenderer();
    ~SingleImageRenderer();

    // it's not safe to be able to copy/move this because we would also need to take care of the
    // created OpenGL buffers and textures
    SingleImageRenderer(const SingleImageRenderer&) = delete;
    SingleImageRenderer& operator=(SingleImageRenderer other) = delete;
    SingleImageRenderer(SingleImageRenderer&&) = delete;
    SingleImageRenderer& operator=(SingleImageRenderer&& other) = delete;

    void Render(const glm::mat4& transform,
                const glm::vec2& position,
                const glm::vec4& color,
                const glm::vec2& scale,
                float rotation);

    void SetTexture(const std::string& file_name);

private:
    Shader shader_;

    unsigned int vbo_;
    unsigned int ebo_;
    unsigned int texture_;
};
} // namespace Soldank

#endif
