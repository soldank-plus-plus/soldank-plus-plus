#include "rendering/renderer/interface/map_editor/SpawnPointRenderer.hpp"

#include "rendering/data/Texture.hpp"
#include "rendering/renderer/Renderer.hpp"
#include "rendering/shaders/ShaderSources.hpp"

#include "spdlog/spdlog.h"
#include <filesystem>
#include <utility>
#include <vector>

namespace Soldank
{
SpawnPointRenderer::SpawnPointRenderer()
    : shader_(ShaderSources::VERTEX_SHADER_SOURCE, ShaderSources::FRAGMENT_SHADER_SOURCE)
    , vbo_(0)
    , textures_{}
{
    LoadTexture(PMSSpawnPointType::General, "neutral-team.png");
    LoadTexture(PMSSpawnPointType::Alpha, "alpha-team.png");
    LoadTexture(PMSSpawnPointType::Bravo, "bravo-team.png");
    LoadTexture(PMSSpawnPointType::Charlie, "charlie-team.png");
    LoadTexture(PMSSpawnPointType::Delta, "delta-team.png");
    LoadTexture(PMSSpawnPointType::AlphaFlag, "alpha-flag.png");
    LoadTexture(PMSSpawnPointType::BravoFlag, "bravo-flag.png");
    LoadTexture(PMSSpawnPointType::Grenades, "grenade-kit.png");
    LoadTexture(PMSSpawnPointType::Medkits, "medical-kit.png");
    LoadTexture(PMSSpawnPointType::Clusters, "cluster-kit.png");
    LoadTexture(PMSSpawnPointType::Vest, "vest-kit.png");
    LoadTexture(PMSSpawnPointType::Flamer, "flamer-kit.png");
    LoadTexture(PMSSpawnPointType::Berserker, "berserker-kit.png");
    LoadTexture(PMSSpawnPointType::Predator, "predator-kit.png");
    LoadTexture(PMSSpawnPointType::YellowFlag, "pointmatch-flag.png");
    LoadTexture(PMSSpawnPointType::RamboBow, "bow.png");
    LoadTexture(PMSSpawnPointType::StatGun, "stationary-gun.png");

    float half_width = 16.0F;
    float half_height = 16.0F;

    std::vector<float> vertices{
        // position                      // color                // texture
        -half_width, half_height,  1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 0.0F, 1.0F,
        half_width,  half_height,  1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F,
        -half_width, -half_height, 0.0F, 1.0F, 1.0F, 1.0F, 1.0F, 0.0F, 0.0F,
        half_width,  -half_height, 0.0F, 1.0F, 1.0F, 1.0F, 1.0F, 1.0F, 0.0F,
    };

    std::vector<unsigned int> indices{ 0, 1, 2, 1, 3, 2 };

    vbo_ = Renderer::CreateVBO(vertices, GL_STATIC_DRAW);
    ebo_ = Renderer::CreateEBO(indices, GL_STATIC_DRAW);
}

SpawnPointRenderer::~SpawnPointRenderer()
{
    Renderer::FreeVBO(vbo_);
    Renderer::FreeEBO(ebo_);
    for (auto texture : textures_) {
        if (texture != 0) {
            Texture::Delete(texture);
        }
    }
}

void SpawnPointRenderer::Render(glm::mat4 transform, PMSSpawnPoint spawn_point, float scale)
{
    shader_.Use();
    Renderer::SetupVertexArray(vbo_, ebo_);

    glm::mat4 current_scenery_transform = transform;

    current_scenery_transform =
      glm::translate(current_scenery_transform, glm::vec3(spawn_point.x, -spawn_point.y, 0.0));
    current_scenery_transform =
      glm::scale(current_scenery_transform, glm::vec3(0.5 * scale, 0.5 * scale, 0.0));

    shader_.SetMatrix4("transform", current_scenery_transform);

    Renderer::BindTexture(textures_.at(std::to_underlying(spawn_point.type)));
    Renderer::DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, std::nullopt);
}

void SpawnPointRenderer::LoadTexture(PMSSpawnPointType spawn_point_type,
                                     const std::string& file_name)
{
    std::filesystem::path file_path = "interface-gfx/spawnpoints/" + file_name;
    auto texture_or_error = Texture::Load(file_path.string().c_str());
    if (texture_or_error.has_value()) {
        textures_.at(std::to_underlying(spawn_point_type)) = texture_or_error.value().opengl_id;
    } else {
        spdlog::critical("Texture file not found {}", file_path.string());
        textures_.at(std::to_underlying(spawn_point_type)) = 0;
    }
}
} // namespace Soldank
