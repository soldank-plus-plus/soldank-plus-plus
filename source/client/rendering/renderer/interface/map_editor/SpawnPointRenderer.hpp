#ifndef __SPAWN_POINT_RENDERER_HPP__
#define __SPAWN_POINT_RENDERER_HPP__

#include "rendering/shaders/Shader.hpp"

#include "core/map/PMSStructs.hpp"
#include "core/map/PMSEnums.hpp"
#include "core/math/Glm.hpp"

#include <array>

namespace Soldank
{
class SpawnPointRenderer
{
public:
    SpawnPointRenderer();
    ~SpawnPointRenderer();

    // it's not safe to be able to copy/move this because we would also need to take care of the
    // created OpenGL buffers and textures
    SpawnPointRenderer(const SpawnPointRenderer&) = delete;
    SpawnPointRenderer& operator=(SpawnPointRenderer other) = delete;
    SpawnPointRenderer(SpawnPointRenderer&&) = delete;
    SpawnPointRenderer& operator=(SpawnPointRenderer&& other) = delete;

    void Render(glm::mat4 transform, PMSSpawnPoint spawn_point, float scale = 1.0F);

private:
    void LoadTexture(PMSSpawnPointType spawn_point_type, const std::string& file_name);

    Shader shader_;

    unsigned int vbo_;
    unsigned int ebo_;
    std::array<unsigned int, 17> textures_;
};
} // namespace Soldank

#endif
