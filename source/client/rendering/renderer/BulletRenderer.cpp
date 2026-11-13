module;

#include "rendering/data/sprites/SpriteTypes.hpp"
#include "rendering/shaders/ShaderSources.hpp"

#include <glad/glad.h>

#include <map>
#include <unordered_map>
#include <numbers>
#include <cmath>
#include <vector>
#include <array>
#include <algorithm>

export module BulletRenderer;

import Extern.Glm;

import Renderer;
import Shader;
import Texture;
import SpritesManager;
import WeaponVisualEffects;

import Shared.Core.Entities.Bullet;
import Shared.Core.Math.Calc;
import Shared.Core.Types.BulletType;
import Shared.Core.Types.WeaponType;

export namespace Soldank
{
class BulletRenderer
{
public:
    BulletRenderer(const Sprites::SpriteManager& sprite_manager);
    ~BulletRenderer() = default;

    // it's not safe to be able to copy/move this because we would also need to take care of the
    // created OpenGL buffers and textures
    BulletRenderer(const BulletRenderer&) = delete;
    BulletRenderer& operator=(BulletRenderer other) = delete;
    BulletRenderer(BulletRenderer&&) = delete;
    BulletRenderer& operator=(BulletRenderer&& other) = delete;

    void Render(glm::mat4 transform, const Bullet& bullet, double frame_percent);
    void RenderEffect(const glm::mat4& transform,
                      const WeaponVisualEffect& effect,
                      float frame_percent);

private:
    struct BulletSpriteData
    {
        unsigned int vbo;
        unsigned int ebo;
        Texture::TextureData texture_data;
    };

    BulletSpriteData CreateSpriteData(Texture::TextureData texture_data, glm::vec2 pivot);
    void DrawSprite(const glm::mat4& transform,
                    const BulletSpriteData& data,
                    glm::vec2 position,
                    float rotation,
                    glm::vec2 scale,
                    glm::vec4 color);
    void LoadSpriteData(const Sprites::SpriteManager& sprite_manager,
                        Sprites::WeaponSpriteType weapon_sprite_type,
                        glm::vec2 pivot);

    Shader shader_;
    std::array<BulletSpriteData, 16> explosion_sprites_{};
    std::array<BulletSpriteData, 11> explosion_smoke_sprites_{};
    BulletSpriteData big_smoke_sprite_{};
    BulletSpriteData big_smoke2_sprite_{};
    BulletSpriteData smoke_sprite_{};
    BulletSpriteData spark_sprite_{};
    BulletSpriteData flame_sprite_{};

    std::unordered_map<Sprites::WeaponSpriteType, BulletSpriteData> weapon_sprite_type_to_gl_data_;
};
} // namespace Soldank

namespace Soldank
{

BulletRenderer::BulletRenderer(const Sprites::SpriteManager& sprite_manager)
    : shader_(ShaderSources::DYNAMIC_COLOR_VERTEX_SHADER_SOURCE,
              ShaderSources::DYNAMIC_COLOR_FRAGMENT_SHADER_SOURCE)
{
    LoadSpriteData(sprite_manager, Sprites::WeaponSpriteType::Knife, { 0.5, 0.5 });
    LoadSpriteData(sprite_manager, Sprites::WeaponSpriteType::Knife2, { 0.5, 0.5 });
    LoadSpriteData(sprite_manager, Sprites::WeaponSpriteType::Bullet, { 0.5, 0.5 });
    LoadSpriteData(sprite_manager, Sprites::WeaponSpriteType::DeaglesBullet, { 0.5, 0.5 });
    LoadSpriteData(sprite_manager, Sprites::WeaponSpriteType::Mp5Bullet, { 0.5, 0.5 });
    LoadSpriteData(sprite_manager, Sprites::WeaponSpriteType::Ak74Bullet, { 0.5, 0.5 });
    LoadSpriteData(sprite_manager, Sprites::WeaponSpriteType::SteyrBullet, { 0.5, 0.5 });
    LoadSpriteData(sprite_manager, Sprites::WeaponSpriteType::SpasBullet, { 0.5, 0.5 });
    LoadSpriteData(sprite_manager, Sprites::WeaponSpriteType::M79Bullet, { 0.5, 0.5 });
    LoadSpriteData(sprite_manager, Sprites::WeaponSpriteType::RugerBullet, { 0.5, 0.5 });
    LoadSpriteData(sprite_manager, Sprites::WeaponSpriteType::BarrettBullet, { 0.5, 0.5 });
    LoadSpriteData(sprite_manager, Sprites::WeaponSpriteType::MinimiBullet, { 0.5, 0.5 });
    LoadSpriteData(sprite_manager, Sprites::WeaponSpriteType::MinigunBullet, { 0.5, 0.5 });
    LoadSpriteData(sprite_manager, Sprites::WeaponSpriteType::ColtBullet, { 0.5, 0.5 });
    LoadSpriteData(sprite_manager, Sprites::WeaponSpriteType::Missile, { 0.5, 0.5 });
    for (const auto type : { Sprites::WeaponSpriteType::Arrow,
                             Sprites::WeaponSpriteType::FragGrenade,
                             Sprites::WeaponSpriteType::ClusterGrenade,
                             Sprites::WeaponSpriteType::Cluster,
                             Sprites::WeaponSpriteType::Shell,
                             Sprites::WeaponSpriteType::DeaglesShell,
                             Sprites::WeaponSpriteType::Mp5Shell,
                             Sprites::WeaponSpriteType::Ak74Shell,
                             Sprites::WeaponSpriteType::SteyrShell,
                             Sprites::WeaponSpriteType::SpasShell,
                             Sprites::WeaponSpriteType::RugerShell,
                             Sprites::WeaponSpriteType::M79Shell,
                             Sprites::WeaponSpriteType::BarrettShell,
                             Sprites::WeaponSpriteType::MinimiShell,
                             Sprites::WeaponSpriteType::MinigunShell,
                             Sprites::WeaponSpriteType::ColtShell }) {
        LoadSpriteData(sprite_manager, type, { 0.5F, 0.5F });
    }
    flame_sprite_ = CreateSpriteData(
      sprite_manager.GetEffectTexture(Sprites::SparkSpriteType::Plomyk), { 0.5F, 0.5F });
    smoke_sprite_ = CreateSpriteData(
      sprite_manager.GetEffectTexture(Sprites::SparkSpriteType::Lilsmoke), { 0.5F, 0.5F });
    spark_sprite_ = CreateSpriteData(
      sprite_manager.GetEffectTexture(Sprites::SparkSpriteType::Odprysk), { 0.5F, 0.5F });
    for (unsigned int frame = 0; frame < explosion_sprites_.size(); ++frame) {
        const auto type = static_cast<Sprites::SparkSpriteType>(
          static_cast<unsigned int>(Sprites::SparkSpriteType::ExplosionExplode1) + frame);
        explosion_sprites_.at(frame) =
          CreateSpriteData(sprite_manager.GetEffectTexture(type), { 0.0F, 1.0F });
    }
    for (unsigned int frame = 0; frame < explosion_smoke_sprites_.size(); ++frame) {
        const auto type = static_cast<Sprites::SparkSpriteType>(
          static_cast<unsigned int>(Sprites::SparkSpriteType::ExplosionSmoke1) + frame);
        explosion_smoke_sprites_.at(frame) =
          CreateSpriteData(sprite_manager.GetEffectTexture(type), { 0.0F, 1.0F });
    }
    big_smoke_sprite_ = CreateSpriteData(
      sprite_manager.GetEffectTexture(Sprites::SparkSpriteType::Bigsmoke), { 0.0F, 1.0F });
    big_smoke2_sprite_ = CreateSpriteData(
      sprite_manager.GetEffectTexture(Sprites::SparkSpriteType::Bigsmoke2), { 0.0F, 1.0F });
}

void BulletRenderer::LoadSpriteData(const Sprites::SpriteManager& sprite_manager,
                                    Sprites::WeaponSpriteType weapon_sprite_type,
                                    glm::vec2 pivot)
{
    weapon_sprite_type_to_gl_data_.insert_or_assign(
      weapon_sprite_type,
      CreateSpriteData(sprite_manager.GetBulletTexture(weapon_sprite_type), pivot));
}

BulletRenderer::BulletSpriteData BulletRenderer::CreateSpriteData(Texture::TextureData texture_data,
                                                                  glm::vec2 pivot)
{

    pivot.x *= (float)texture_data.width;
    pivot.y *= (float)texture_data.height;
    float w0 = 0.0F - pivot.x;
    float w1 = (float)texture_data.width - pivot.x;
    float h0 = 0.0F - pivot.y;
    float h1 = (float)texture_data.height - pivot.y;

    // clang-format off
    std::vector<float> vertices{
      // position     // texture
      w0, h0, 1.0F,   0.0F, 0.0F,
      w1, h0, 1.0F,   1.0F, 0.0F,
      w0, h1, 1.0F,   0.0F, 1.0F,
      w1, h1, 1.0F,   1.0F, 1.0F
    };
    // clang-format on

    std::vector<unsigned int> indices{ 0, 1, 2, 1, 3, 2 };

    unsigned int vbo = Renderer::CreateVBO(vertices, GL_STATIC_DRAW);
    unsigned int ebo = Renderer::CreateEBO(indices, GL_STATIC_DRAW);
    return { vbo, ebo, texture_data };
}

void BulletRenderer::Render(glm::mat4 transform, const Bullet& bullet, double frame_percent)
{
    if (!bullet.active || bullet.style == BulletType::Fist || bullet.style == BulletType::Blade) {
        return;
    }

    auto pos =
      Calc::Lerp(bullet.particle.old_position, bullet.particle.position, (float)frame_percent);

    float rot = 0.0F;
    glm::vec2 scale;
    float alpha = 255.0F;
    BulletSpriteData bullet_sprite_data{};

    switch (bullet.style) {
        case BulletType::FragGrenade:
        case BulletType::ClusterGrenade:
        case BulletType::Cluster: {
            auto type = Sprites::WeaponSpriteType::FragGrenade;
            if (bullet.style == BulletType::ClusterGrenade) {
                type = Sprites::WeaponSpriteType::ClusterGrenade;
            } else if (bullet.style == BulletType::Cluster) {
                type = Sprites::WeaponSpriteType::Cluster;
            }
            bullet_sprite_data = weapon_sprite_type_to_gl_data_.at(type);
            scale = { 1.0F, 1.0F };
            rot = static_cast<float>(bullet.timeout) * 0.15F;
            break;
        }
        case BulletType::Arrow:
        case BulletType::FlameArrow:
            bullet_sprite_data =
              weapon_sprite_type_to_gl_data_.at(Sprites::WeaponSpriteType::Arrow);
            scale = { 1.0F, 1.0F };
            rot = Calc::Vec2Angle(bullet.initial_position - bullet.particle.position);
            break;
        case BulletType::Flame:
            bullet_sprite_data = flame_sprite_;
            scale = { 0.8F, 0.8F };
            alpha = std::clamp(static_cast<float>(bullet.timeout) * 8.0F, 30.0F, 230.0F);
            rot = static_cast<float>(bullet.timeout) * 0.2F;
            break;
        case BulletType::ThrownKnife: {
            auto t = Calc::Lerp(bullet.timeout_prev, bullet.timeout, (float)frame_percent);
            rot = 0.0F;
            scale = { 1.0F, 1.0F };

            if (bullet.particle.velocity_.x >= 0.0F) {
                rot = t / std::numbers::pi;
                bullet_sprite_data =
                  weapon_sprite_type_to_gl_data_.at(Sprites::WeaponSpriteType::Knife);
            } else {
                rot = -(t / std::numbers::pi);
                // auto bullet_type = WeaponType::Knife2;
                bullet_sprite_data =
                  weapon_sprite_type_to_gl_data_.at(Sprites::WeaponSpriteType::Knife2);
            }
            break;
        }
        case BulletType::M79Grenade: {
            bullet_sprite_data =
              weapon_sprite_type_to_gl_data_.at(Sprites::WeaponSpriteType::M79Bullet);

            scale = { 1.0F, 1.0F };
            alpha = 252.0F;
            rot = Calc::Vec2Angle(-bullet.particle.velocity_);
            break;
        }
        case BulletType::LAWMissile: {
            bullet_sprite_data =
              weapon_sprite_type_to_gl_data_.at(Sprites::WeaponSpriteType::Missile);
            scale = { 1.0F, 1.0F };
            rot = Calc::Vec2Angle(-bullet.particle.velocity_);
            break;
        }
        default: {
            auto hit =
              Calc::Lerp(bullet.hit_multiply_prev, bullet.hit_multiply, (float)frame_percent);

            bullet_sprite_data =
              weapon_sprite_type_to_gl_data_.at(Sprites::WeaponSpriteType::Bullet);

            scale = { Calc::Vec2Length(bullet.particle.velocity_) / 13.0F, 1.0F };
            auto dist = Calc::Vec2Length(pos - bullet.initial_position);

            if (dist < scale.x * (float)bullet_sprite_data.texture_data.width) {
                scale.x = dist / (scale.x * (float)bullet_sprite_data.texture_data.width);
            }
            alpha = std::max(50.0F, std::min(230.0F, 255.0F * hit * (scale.x * scale.x) / 4.63F));

            rot = Calc::Vec2Angle(-bullet.particle.velocity_);
            break;
        }
    }

    if (bullet.style == BulletType::GaugeBullet) {
        bullet_sprite_data =
          weapon_sprite_type_to_gl_data_.at(Sprites::WeaponSpriteType::SpasBullet);
    }
    if (bullet.style == BulletType::Bullet) {
        if (bullet.weapon == WeaponType::DesertEagles) {
            bullet_sprite_data =
              weapon_sprite_type_to_gl_data_.at(Sprites::WeaponSpriteType::DeaglesBullet);
        }
        if (bullet.weapon == WeaponType::MP5) {
            bullet_sprite_data =
              weapon_sprite_type_to_gl_data_.at(Sprites::WeaponSpriteType::Mp5Bullet);
        }
        if (bullet.weapon == WeaponType::Ak74) {
            bullet_sprite_data =
              weapon_sprite_type_to_gl_data_.at(Sprites::WeaponSpriteType::Ak74Bullet);
        }
        if (bullet.weapon == WeaponType::SteyrAUG) {
            bullet_sprite_data =
              weapon_sprite_type_to_gl_data_.at(Sprites::WeaponSpriteType::SteyrBullet);
        }
        if (bullet.weapon == WeaponType::Ruger77) {
            bullet_sprite_data =
              weapon_sprite_type_to_gl_data_.at(Sprites::WeaponSpriteType::RugerBullet);
        }
        if (bullet.weapon == WeaponType::Barrett) {
            bullet_sprite_data =
              weapon_sprite_type_to_gl_data_.at(Sprites::WeaponSpriteType::BarrettBullet);
        }
        if (bullet.weapon == WeaponType::Minimi) {
            bullet_sprite_data =
              weapon_sprite_type_to_gl_data_.at(Sprites::WeaponSpriteType::MinimiBullet);
        }
        if (bullet.weapon == WeaponType::Minigun) {
            bullet_sprite_data =
              weapon_sprite_type_to_gl_data_.at(Sprites::WeaponSpriteType::MinigunBullet);
        }
        if (bullet.weapon == WeaponType::USSOCOM) {
            bullet_sprite_data =
              weapon_sprite_type_to_gl_data_.at(Sprites::WeaponSpriteType::ColtBullet);
        }
    }

    DrawSprite(transform,
               bullet_sprite_data,
               pos,
               rot,
               scale / 4.5F,
               glm::vec4(1.0F, 1.0F, 1.0F, alpha / 255.0F));
}

void BulletRenderer::DrawSprite(const glm::mat4& transform,
                                const BulletSpriteData& data,
                                glm::vec2 position,
                                float rotation,
                                glm::vec2 scale,
                                glm::vec4 color)
{
    if (data.texture_data.opengl_id == 0) {
        return;
    }
    shader_.Use();
    Renderer::SetupVertexArray(data.vbo, data.ebo, false, true);
    auto matrix = glm::translate(transform, glm::vec3(position.x, -position.y, 0.0F));
    matrix = glm::rotate(matrix, rotation, glm::vec3(0.0F, 0.0F, 1.0F));
    matrix = glm::scale(matrix, glm::vec3(scale.x, scale.y, 0.0F));
    shader_.SetMatrix4("transform", matrix);
    shader_.SetVec4("color", color);
    Renderer::BindTexture(data.texture_data.opengl_id);
    Renderer::DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void BulletRenderer::RenderEffect(const glm::mat4& transform,
                                  const WeaponVisualEffect& effect,
                                  float frame_percent)
{
    if (effect.kind == WeaponVisualEffectKind::Explosion ||
        effect.kind == WeaponVisualEffectKind::ExplosionSmoke ||
        effect.kind == WeaponVisualEffectKind::ExplosionAfterSmoke ||
        effect.kind == WeaponVisualEffectKind::ClusterSmoke) {
        for (const auto& sprite : GetExplosionSprites(effect, frame_percent)) {
            const BulletSpriteData* data = &big_smoke_sprite_;
            switch (sprite.kind) {
                case ExplosionSpriteKind::Blast:
                    data = &explosion_sprites_.at(static_cast<std::size_t>(sprite.frame));
                    break;
                case ExplosionSpriteKind::Smoke:
                    data = &explosion_smoke_sprites_.at(static_cast<std::size_t>(sprite.frame));
                    break;
                case ExplosionSpriteKind::BigSmoke:
                    break;
                case ExplosionSpriteKind::BigSmoke2:
                    data = &big_smoke2_sprite_;
                    break;
            }
            // soldatbase mod.ini DefaultScale=4.5; preserve the frame's native aspect ratio.
            DrawSprite(transform, *data, sprite.position, 0.0F, sprite.scale / 4.5F, sprite.color);
        }
        return;
    }
    BulletSpriteData data = smoke_sprite_;
    glm::vec2 scale{};
    if (effect.kind == WeaponVisualEffectKind::Casing) {
        static const std::unordered_map<WeaponType, Sprites::WeaponSpriteType> shells{
            { WeaponType::DesertEagles, Sprites::WeaponSpriteType::DeaglesShell },
            { WeaponType::MP5, Sprites::WeaponSpriteType::Mp5Shell },
            { WeaponType::Ak74, Sprites::WeaponSpriteType::Ak74Shell },
            { WeaponType::SteyrAUG, Sprites::WeaponSpriteType::SteyrShell },
            { WeaponType::Spas12, Sprites::WeaponSpriteType::SpasShell },
            { WeaponType::Ruger77, Sprites::WeaponSpriteType::RugerShell },
            { WeaponType::M79, Sprites::WeaponSpriteType::M79Shell },
            { WeaponType::Barrett, Sprites::WeaponSpriteType::BarrettShell },
            { WeaponType::Minimi, Sprites::WeaponSpriteType::MinimiShell },
            { WeaponType::Minigun, Sprites::WeaponSpriteType::MinigunShell },
            { WeaponType::USSOCOM, Sprites::WeaponSpriteType::ColtShell },
        };
        const auto found = shells.find(effect.weapon);
        const auto type = found == shells.end() ? Sprites::WeaponSpriteType::Shell : found->second;
        data = weapon_sprite_type_to_gl_data_.at(type);
        scale = glm::vec2(1.0F / 4.5F);
    } else {
        if (effect.kind == WeaponVisualEffectKind::Flame) {
            data = flame_sprite_;
        } else if (effect.kind == WeaponVisualEffectKind::Spark) {
            data = spark_sprite_;
        }
        if (data.texture_data.width <= 0 || data.texture_data.height <= 0) {
            return;
        }
        scale = glm::vec2(effect.size / static_cast<float>(data.texture_data.width),
                          effect.size / static_cast<float>(data.texture_data.height));
    }
    const auto position = Calc::Lerp(effect.old_position, effect.position, frame_percent);
    const float rotation = Calc::Lerp(effect.old_rotation, effect.rotation, frame_percent);
    const float alpha =
      std::clamp(static_cast<float>(effect.lifetime) / effect.initial_lifetime, 0.0F, 1.0F);
    DrawSprite(transform, data, position, rotation, scale, glm::vec4(1.0F, 1.0F, 1.0F, alpha));
}

} // namespace Soldank
