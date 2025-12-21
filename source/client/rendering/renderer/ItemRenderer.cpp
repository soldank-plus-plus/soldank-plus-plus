module;

#include "core/types/ItemType.hpp"
#include "rendering/data/sprites/SpriteTypes.hpp"
#include "rendering/shaders/ShaderSources.hpp"

#include "core/math/Calc.hpp"
#include "core/math/Glm.hpp"
#include "core/entities/Item.hpp"

#include <glm/ext/quaternion_transform.hpp>

#include <glad/glad.h>

#include <numbers>
#include <cmath>
#include <map>
#include <unordered_map>

export module ItemRenderer;

import Texture;
import Renderer;
import Shader;
import SpritesManager;

export namespace Soldank
{
class ItemRenderer
{
public:
    ItemRenderer(const Sprites::SpriteManager& sprite_manager);
    ~ItemRenderer();

    // it's not safe to be able to copy/move this because we would also need to take care of the
    // created OpenGL buffers and textures
    ItemRenderer(const ItemRenderer&) = delete;
    ItemRenderer& operator=(ItemRenderer other) = delete;
    ItemRenderer(ItemRenderer&&) = delete;
    ItemRenderer& operator=(ItemRenderer&& other) = delete;

    void Render(glm::mat4 transform,
                const Item& item,
                double frame_percent,
                unsigned int game_tick);

private:
    struct ItemSpriteData
    {
        unsigned int vbo;
        unsigned int ebo;
        Texture::TextureData texture_data;
    };

    void LoadSpriteData(const Sprites::SpriteManager& sprite_manager, ItemType item_type);
    void LoadObjectSpriteData(const Sprites::SpriteManager& sprite_manager,
                              Sprites::ObjectSpriteType object_sprite_type);

    void RenderQuad(glm::mat4 transform, const Item& item, double frame_percent);
    void RenderWeapon(glm::mat4 transform, const Item& item, double frame_percent);
    void RenderFlagSprites(glm::mat4 transform,
                           const Item& item,
                           double frame_percent,
                           unsigned int game_tick);
    void RenderParachute(glm::mat4 transform, const Item& item, double frame_percent);
    void RenderSprite(glm::mat4 transform,
                      const Texture::TextureData& item_sprite_data,
                      glm::vec2 position,
                      float rotation,
                      glm::vec2 scale,
                      glm::vec4 color = { 1.0F, 1.0F, 1.0F, 1.0F },
                      glm::vec2 pivot = { 0.0F, 0.0F });

    static glm::vec4 GetQuadMainColor(ItemType item_type);
    static glm::vec4 GetQuadTopColor(ItemType item_type);
    static glm::vec4 GetQuadLowColor(ItemType item_type);

    Shader shader_;

    std::unordered_map<ItemType, Texture::TextureData> item_sprite_type_to_gl_data_;
    std::unordered_map<ItemType, Texture::TextureData> item_sprite_type_to_flipped_gl_data_;
    std::unordered_map<Sprites::ObjectSpriteType, Texture::TextureData>
      object_sprite_type_to_gl_data_;

    unsigned vbo_;
    unsigned ebo_;
};
} // namespace Soldank

namespace Soldank
{
ItemRenderer::ItemRenderer(const Sprites::SpriteManager& sprite_manager)
    : shader_(ShaderSources::VERTEX_SHADER_SOURCE, ShaderSources::FRAGMENT_SHADER_SOURCE)
{
    // Flags
    LoadSpriteData(sprite_manager, ItemType::AlphaFlag);
    LoadSpriteData(sprite_manager, ItemType::BravoFlag);
    LoadSpriteData(sprite_manager, ItemType::PointmatchFlag);
    LoadObjectSpriteData(sprite_manager, Sprites::ObjectSpriteType::FlagHandle);
    LoadObjectSpriteData(sprite_manager, Sprites::ObjectSpriteType::Ilum);

    // Kits
    LoadSpriteData(sprite_manager, ItemType::GrenadeKit);
    LoadSpriteData(sprite_manager, ItemType::MedicalKit);
    LoadSpriteData(sprite_manager, ItemType::VestKit);
    LoadSpriteData(sprite_manager, ItemType::ClusterKit);
    LoadSpriteData(sprite_manager, ItemType::PredatorKit);
    LoadSpriteData(sprite_manager, ItemType::BerserkKit);
    LoadSpriteData(sprite_manager, ItemType::FlamerKit);

    // Weapons
    LoadSpriteData(sprite_manager, ItemType::DesertEagles);
    LoadSpriteData(sprite_manager, ItemType::MP5);
    LoadSpriteData(sprite_manager, ItemType::Ak74);
    LoadSpriteData(sprite_manager, ItemType::SteyrAUG);
    LoadSpriteData(sprite_manager, ItemType::Spas12);
    LoadSpriteData(sprite_manager, ItemType::Ruger77);
    LoadSpriteData(sprite_manager, ItemType::M79);
    LoadSpriteData(sprite_manager, ItemType::Barrett);
    LoadSpriteData(sprite_manager, ItemType::Minimi);
    LoadSpriteData(sprite_manager, ItemType::Minigun);
    LoadSpriteData(sprite_manager, ItemType::USSOCOM);
    LoadSpriteData(sprite_manager, ItemType::Chainsaw);
    LoadSpriteData(sprite_manager, ItemType::Knife);
    LoadSpriteData(sprite_manager, ItemType::LAW);
    LoadSpriteData(sprite_manager, ItemType::Bow);

    // Parachute
    LoadObjectSpriteData(sprite_manager, Sprites::ObjectSpriteType::ParaRope);
    LoadObjectSpriteData(sprite_manager, Sprites::ObjectSpriteType::Para);
    LoadObjectSpriteData(sprite_manager, Sprites::ObjectSpriteType::Para2);

    // generate vertices
    // clang-format off
    std::vector<float> vertices{
      // position         // color                  // texture
      0.0F, 0.0F, 1.0F,   1.0F, 1.0F, 1.0F, 1.0F,   0.0F, 0.0F,
      0.0F, 0.0F, 1.0F,   1.0F, 1.0F, 1.0F, 1.0F,   1.0F, 0.0F,
      0.0F, 0.0F, 1.0F,   1.0F, 1.0F, 1.0F, 1.0F,   1.0F, 1.0F,
      0.0F, 0.0F, 1.0F,   1.0F, 1.0F, 1.0F, 1.0F,   0.0F, 1.0F
    };
    // clang-format on

    vbo_ = Renderer::CreateVBO(vertices, GL_DYNAMIC_DRAW);
    std::vector<unsigned int> indices{ 0, 1, 3, 1, 2, 3 };
    ebo_ = Renderer::CreateEBO(indices, GL_STATIC_DRAW);
}

ItemRenderer::~ItemRenderer()
{
    Renderer::FreeVBO(vbo_);
    Renderer::FreeEBO(ebo_);
}

void ItemRenderer::LoadSpriteData(const Sprites::SpriteManager& sprite_manager, ItemType item_type)
{
    auto texture_data = sprite_manager.GetItemTexture(item_type);
    item_sprite_type_to_gl_data_[item_type] = texture_data;
    texture_data = sprite_manager.GetItemTexture(item_type, true);
    item_sprite_type_to_flipped_gl_data_[item_type] = texture_data;
}

void ItemRenderer::LoadObjectSpriteData(const Sprites::SpriteManager& sprite_manager,
                                        Sprites::ObjectSpriteType object_sprite_type)
{
    object_sprite_type_to_gl_data_[object_sprite_type] =
      sprite_manager.GetObjectSprite(object_sprite_type);
}

void ItemRenderer::Render(glm::mat4 transform,
                          const Item& item,
                          double frame_percent,
                          unsigned int game_tick)
{
    if (!item.active) {
        return;
    }

    if (IsItemTypeFlag(item.style)) {
        // fade out (sort of)
        if (item.time_out < 300) {
            if ((item.time_out % 6) < 3) {
                return;
            }
        }
        RenderFlagSprites(transform, item, frame_percent, game_tick);
        RenderQuad(transform, item, frame_percent);
    }

    if (IsItemTypeWeapon(item.style)) {
        RenderWeapon(transform, item, frame_percent);
    }

    if (IsItemTypeKit(item.style)) {
        RenderQuad(transform, item, frame_percent);
    }

    if (item.style == ItemType::Parachute) {
        RenderParachute(transform, item, frame_percent);
    }
}

void ItemRenderer::RenderQuad(glm::mat4 transform, const Item& item, double frame_percent)
{
    const auto& item_sprite_data = item_sprite_type_to_gl_data_.at(item.style);
    glm::vec2 pos =
      Calc::Lerp(item.skeleton->GetOldPos(1), item.skeleton->GetPos(1), (float)frame_percent);
    auto main_color = GetQuadMainColor(item.style);
    auto top_color = GetQuadTopColor(item.style);
    auto low_color = GetQuadLowColor(item.style);

    shader_.Use();

    // Set corners of the item on a (0,0) anchor from 1st corner
    glm::vec2 pos1 = item.skeleton->GetPos(1) - item.skeleton->GetPos(1);
    glm::vec2 pos2 = item.skeleton->GetPos(2) - item.skeleton->GetPos(1);
    glm::vec2 pos3 = item.skeleton->GetPos(3) - item.skeleton->GetPos(1);
    glm::vec2 pos4 = item.skeleton->GetPos(4) - item.skeleton->GetPos(1);

    if (IsItemTypeFlag(item.style)) {
        // Move up the position at the handle to halfway between it and the flag tip
        auto diff = pos2 - pos1;
        pos1 += Calc::Vec2Scale(diff, 0.5);
    }

    // OpenGL top screen is 1.0 but all the physics code has top screen at 0.0
    // So we need to flip the corners vertically (horizontal mirror image)
    pos1.y = -pos1.y;
    pos2.y = -pos2.y;
    pos3.y = -pos3.y;
    pos4.y = -pos4.y;

    // clang-format off
    std::vector<float> vertices{
      // position             // color                                          // texture
      pos1.x, pos1.y, 1.0F,   main_color.x, main_color.y, main_color.z, 1.0F,   0.0F, 0.0F,
      pos2.x, pos2.y, 1.0F,   low_color.x, low_color.y, low_color.z, 1.0F,   1.0F, 0.0F,
      pos3.x, pos3.y, 1.0F,   main_color.x, main_color.y, main_color.z, 1.0F,   1.0F, 1.0F,
      pos4.x, pos4.y, 1.0F,   top_color.x, top_color.y, top_color.z, 1.0F,   0.0F, 1.0F
    };
    // clang-format on

    glm::mat4 current_scenery_transform = transform;

    // We need to move the corners from (0,0) anchor to the position on the map
    current_scenery_transform =
      glm::translate(current_scenery_transform, glm::vec3(pos.x, -pos.y, 0.0));

    shader_.SetMatrix4("transform", current_scenery_transform);

    Renderer::ModifyVBOVertices(vbo_, vertices);
    Renderer::SetupVertexArray(vbo_, ebo_, true, true);
    Renderer::BindTexture(item_sprite_data.opengl_id);
    Renderer::DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void ItemRenderer::RenderWeapon(glm::mat4 transform, const Item& item, double frame_percent)
{
    glm::vec2 position =
      Calc::Lerp(item.skeleton->GetOldPos(1), item.skeleton->GetPos(1), (float)frame_percent);
    glm::vec2 position2 =
      Calc::Lerp(item.skeleton->GetOldPos(2), item.skeleton->GetPos(2), (float)frame_percent);
    float rotation = Calc::Vec2Angle(position2 - position);
    glm::vec2 scale = { 1.0F, 1.0F };
    scale /= 4.5F;

    position.y -= 3.0F;
    glm::vec4 color = { 1.0F, 1.0F, 1.0F, 1.0F };
    glm::vec2 pivot = { 0.0F, 0.5F };

    if (item.flipped) {
        RenderSprite(transform,
                     item_sprite_type_to_flipped_gl_data_.at(item.style),
                     position,
                     rotation,
                     scale,
                     color,
                     pivot);
    } else {
        RenderSprite(transform,
                     item_sprite_type_to_gl_data_.at(item.style),
                     position,
                     rotation,
                     scale,
                     color,
                     pivot);
    }
}

void ItemRenderer::RenderFlagSprites(glm::mat4 transform,
                                     const Item& item,
                                     double frame_percent,
                                     unsigned int game_tick)
{
    glm::vec2 skeleton_position_1 =
      Calc::Lerp(item.skeleton->GetOldPos(1), item.skeleton->GetPos(1), (float)frame_percent);
    glm::vec2 skeleton_position_2 =
      Calc::Lerp(item.skeleton->GetOldPos(2), item.skeleton->GetPos(2), (float)frame_percent);
    float rotation = Calc::Vec2Angle(skeleton_position_2 - skeleton_position_1);
    glm::vec2 scale = { 1.0F, 1.0F };
    scale /= 4.5F;

    RenderSprite(transform,
                 object_sprite_type_to_gl_data_.at(Sprites::ObjectSpriteType::FlagHandle),
                 skeleton_position_1,
                 rotation,
                 scale);

    if (item.in_base) {
        const Texture::TextureData& ilum_texture =
          object_sprite_type_to_gl_data_.at(Sprites::ObjectSpriteType::Ilum);
        glm::vec2 ilum_size = { ilum_texture.width, ilum_texture.height };
        ilum_size *= scale;
        glm::vec2 ilum_position;
        ilum_position.x = skeleton_position_1.x +
                          (skeleton_position_2.x - skeleton_position_1.x) / 2.0F -
                          ilum_size.x / 2.0F;
        ilum_position.y = skeleton_position_1.y +
                          (skeleton_position_2.y - skeleton_position_1.y) / 2.0F +
                          ilum_size.x / 2.0F;

        RenderSprite(
          transform,
          ilum_texture,
          ilum_position,
          0.0F,
          scale,
          { 1.0F, 1.0F, 1.0F, std::abs(5 + 20 * std::sin(0.0765 * game_tick)) / 255.0F });
    }
}

void ItemRenderer::RenderParachute(glm::mat4 transform, const Item& item, double frame_percent)
{
    const Texture::TextureData& parachute1_texture =
      object_sprite_type_to_gl_data_.at(Sprites::ObjectSpriteType::Para);
    const Texture::TextureData& parachute2_texture =
      object_sprite_type_to_gl_data_.at(Sprites::ObjectSpriteType::Para2);
    const Texture::TextureData& parachute_rope_texture =
      object_sprite_type_to_gl_data_.at(Sprites::ObjectSpriteType::ParaRope);

    glm::vec2 skeleton_position_1 =
      Calc::Lerp(item.skeleton->GetOldPos(1), item.skeleton->GetPos(1), (float)frame_percent);
    glm::vec2 skeleton_position_2 =
      Calc::Lerp(item.skeleton->GetOldPos(2), item.skeleton->GetPos(2), (float)frame_percent);
    glm::vec2 skeleton_position_3 =
      Calc::Lerp(item.skeleton->GetOldPos(3), item.skeleton->GetPos(3), (float)frame_percent);
    glm::vec2 skeleton_position_4 =
      Calc::Lerp(item.skeleton->GetOldPos(4), item.skeleton->GetPos(4), (float)frame_percent);

    glm::vec2 scale = { 1.0F, 1.0F };
    scale /= 4.5F;

    RenderSprite(transform,
                 parachute_rope_texture,
                 { skeleton_position_4.x, skeleton_position_4.y - 0.55 },
                 Calc::Vec2Angle(skeleton_position_2 - skeleton_position_4),
                 scale,
                 { 1.0F, 1.0F, 1.0F, 1.0F },
                 { 0.07F, 0.0F });
    RenderSprite(transform,
                 parachute_rope_texture,
                 { skeleton_position_4.x, skeleton_position_4.y - 0.55 },
                 Calc::Vec2Angle(skeleton_position_3 - skeleton_position_4) +
                   (5.0F * 3.14F / 180.0F),
                 scale,
                 { 1.0F, 1.0F, 1.0F, 1.0F },
                 { 0.07F, 0.0F });
    RenderSprite(transform,
                 parachute_rope_texture,
                 { skeleton_position_4.x, skeleton_position_4.y - 0.55 },
                 Calc::Vec2Angle(skeleton_position_1 - skeleton_position_4),
                 scale,
                 { 1.0F, 1.0F, 1.0F, 1.0F },
                 { 0.07F, 0.0F });

    glm::vec2 a = skeleton_position_2 - skeleton_position_3;
    scale.y = Calc::Vec2Length(a) / 45.83F;
    scale.x = scale.y;
    if (scale.y > 2.0F) {
        return;
    }
    scale /= 4.5F;

    RenderSprite(transform,
                 parachute2_texture,
                 skeleton_position_3,
                 Calc::Vec2Angle(skeleton_position_1 - skeleton_position_3),
                 scale,
                 { 1.0F, 1.0F, 1.0F, 1.0F },
                 { 0.0F, 1.0F });

    RenderSprite(transform,
                 parachute1_texture,
                 skeleton_position_1,
                 Calc::Vec2Angle(skeleton_position_2 - skeleton_position_1),
                 scale,
                 { 1.0F, 1.0F, 1.0F, 1.0F },
                 { 0.0F, 1.0F });
}

void ItemRenderer::RenderSprite(glm::mat4 transform,
                                const Texture::TextureData& item_sprite_data,
                                glm::vec2 position,
                                float rotation,
                                glm::vec2 scale,
                                glm::vec4 color,
                                glm::vec2 pivot)
{
    shader_.Use();
    pivot.x *= (float)item_sprite_data.width;
    pivot.y *= (float)item_sprite_data.height;
    float w0 = 0.0F - pivot.x;
    auto w1 = (float)item_sprite_data.width - pivot.x;
    float h0 = 0.0F - pivot.y;
    auto h1 = (float)item_sprite_data.height - pivot.y;
    glm::vec2 pos1 = { w0, h0 };
    glm::vec2 pos2 = { w1, h0 };
    glm::vec2 pos3 = { w1, h1 };
    glm::vec2 pos4 = { w0, h1 };

    // clang-format off
    std::vector<float> vertices{
      // position             // color                              // texture
      pos1.x, pos1.y, 1.0F,   color.x, color.y, color.z, color.w,   0.0F, 0.0F,
      pos2.x, pos2.y, 1.0F,   color.x, color.y, color.z, color.w,   1.0F, 0.0F,
      pos3.x, pos3.y, 1.0F,   color.x, color.y, color.z, color.w,   1.0F, 1.0F,
      pos4.x, pos4.y, 1.0F,   color.x, color.y, color.z, color.w,   0.0F, 1.0F
    };
    // clang-format on

    glm::mat4 current_scenery_transform = transform;

    // We need to move the corners from (0,0) anchor to the position on the map
    current_scenery_transform =
      glm::translate(current_scenery_transform, glm::vec3(position.x, -position.y, 0.0));
    current_scenery_transform =
      glm::rotate(current_scenery_transform, rotation, glm::vec3(0.0, 0.0, 1.0));
    current_scenery_transform =
      glm::scale(current_scenery_transform, glm::vec3(scale.x, scale.y, 0.0));

    shader_.SetMatrix4("transform", current_scenery_transform);

    Renderer::ModifyVBOVertices(vbo_, vertices);
    Renderer::SetupVertexArray(vbo_, ebo_, true, true);
    Renderer::BindTexture(item_sprite_data.opengl_id);
    Renderer::DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

glm::vec4 ItemRenderer::GetQuadMainColor(ItemType item_type)
{
    switch (item_type) {
        case ItemType::AlphaFlag:
            return { 173.0F / 255.0F, 21.0F / 255.0F, 21.0F / 255.0F, 1.0F };
        case ItemType::BravoFlag:
            return { 5.0F / 255.0F, 16.0F / 255.0F, 173.0F / 255.0F, 1.0F };
        case ItemType::PointmatchFlag:
            return { 173.0F / 255.0F, 173.0F / 255.0F, 21.0F / 255.0F, 1.0F };
        default:
            return { 1.0F, 1.0F, 1.0F, 1.0F };
    }
}

glm::vec4 ItemRenderer::GetQuadTopColor(ItemType item_type)
{
    switch (item_type) {
        case ItemType::AlphaFlag:
            return { 181.0F / 255.0F, 21.0F / 255.0F, 21.0F / 255.0F, 1.0F };
        case ItemType::BravoFlag:
            return { 5.0F / 255.0F, 16.0F / 255.0F, 181.0F / 255.0F, 1.0F };
        case ItemType::PointmatchFlag:
            return { 181.0F / 255.0F, 181.0F / 255.0F, 21.0F / 255.0F, 1.0F };
        default:
            return { 1.0F, 1.0F, 1.0F, 1.0F };
    }
}

glm::vec4 ItemRenderer::GetQuadLowColor(ItemType item_type)
{
    switch (item_type) {
        case ItemType::AlphaFlag:
            return { 149.0F / 255.0F, 21.0F / 255.0F, 21.0F / 255.0F, 1.0F };
        case ItemType::BravoFlag:
            return { 5.0F / 255.0F, 16.0F / 255.0F, 149.0F / 255.0F, 1.0F };
        case ItemType::PointmatchFlag:
            return { 149.0F / 255.0F, 149.0F / 255.0F, 21.0F / 255.0F, 1.0F };
        default:
            return { 1.0F, 1.0F, 1.0F, 1.0F };
    }
}
} // namespace Soldank
