module;

#include "rendering/shaders/ShaderSources.hpp"
#include <glad/glad.h>
#include <algorithm>
#include <array>
#include <string>
#include <vector>

export module HudSpriteRenderer;

import Extern.Glm;
import Texture;
import Shader;
import Renderer;

export namespace Soldank
{
enum class HudSprite : unsigned int
{
    Health,
    Ammo,
    Jet,
    HealthBar,
    AmmoBar,
    JetBar,
    VestBar,
    Grenade,
    ClusterGrenade,
};

// A cropped bar keeps its texture density and clips the right edge instead of stretching it.
class HudSpriteRenderer
{
public:
    HudSpriteRenderer()
        : shader_(ShaderSources::DYNAMIC_COLOR_VERTEX_SHADER_SOURCE,
                  ShaderSources::DYNAMIC_COLOR_FRAGMENT_SHADER_SOURCE)
    {
        constexpr std::array NAMES{ "health",  "ammo",     "jet",  "health-bar",  "reload-bar",
                                    "jet-bar", "vest-bar", "nade", "cluster-nade" };
        for (unsigned int index = 0; index < NAMES.size(); ++index) {
            const auto path = std::string("interface-gfx/") + NAMES.at(index) + ".png";
            if (const auto loaded = Texture::Load(path.c_str()); loaded.has_value()) {
                textures_.at(index) = *loaded;
            }
        }
        vbo_ = Renderer::CreateVBO(std::vector<float>(20, 0.0F), GL_DYNAMIC_DRAW);
        ebo_ = Renderer::CreateEBO({ 0, 1, 2, 1, 3, 2 }, GL_STATIC_DRAW);
    }

    ~HudSpriteRenderer()
    {
        Renderer::FreeVBO(vbo_);
        Renderer::FreeEBO(ebo_);
        for (const auto& texture : textures_) {
            glDeleteTextures(1, &texture.opengl_id);
        }
    }
    HudSpriteRenderer(const HudSpriteRenderer&) = delete;
    HudSpriteRenderer& operator=(const HudSpriteRenderer&) = delete;
    HudSpriteRenderer(HudSpriteRenderer&&) = delete;
    HudSpriteRenderer& operator=(HudSpriteRenderer&&) = delete;

    void Render(HudSprite sprite, glm::vec2 position, glm::vec2 viewport, float fill = 1.0F)
    {
        const auto& texture = textures_.at(static_cast<unsigned int>(sprite));
        fill = std::clamp(fill, 0.0F, 1.0F);
        if (texture.opengl_id == 0 || fill == 0.0F || viewport.x <= 0.0F || viewport.y <= 0.0F) {
            return;
        }
        const float ui_scale = std::min(viewport.x / 640.0F, viewport.y / 480.0F);
        const glm::vec2 origin((viewport.x - 640.0F * ui_scale) * 0.5F,
                               viewport.y - 480.0F * ui_scale);
        const auto p = origin + position * ui_scale;
        const float width = texture.width / 4.5F * ui_scale * fill;
        const float height = texture.height / 4.5F * ui_scale;
        const std::vector<float> vertices{ p.x,         p.y,          1.0F, 0.0F, 1.0F,
                                           p.x + width, p.y,          1.0F, fill, 1.0F,
                                           p.x,         p.y + height, 1.0F, 0.0F, 0.0F,
                                           p.x + width, p.y + height, 1.0F, fill, 0.0F };
        Renderer::ModifyVBOVertices(vbo_, vertices);
        shader_.Use();
        Renderer::SetupVertexArray(vbo_, ebo_, false, true);
        shader_.SetMatrix4("transform", glm::ortho(0.0F, viewport.x, viewport.y, 0.0F));
        shader_.SetVec4("color", glm::vec4(1.0F));
        Renderer::BindTexture(texture.opengl_id);
        Renderer::DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

private:
    Shader shader_;
    std::array<Texture::TextureData, 9> textures_{};
    unsigned int vbo_{};
    unsigned int ebo_{};
};
} // namespace Soldank
