module;

#include "rendering/shaders/ShaderSources.hpp"
#include "rendering/data/sprites/SpriteTypes.hpp"

#include "core/types/WeaponType.hpp"
#include "core/math/Calc.hpp"
#include "core/utility/VisitHelper.hpp"
#include "core/map/Map.hpp"
#include "core/entities/Soldier.hpp"
#include "core/math/Glm.hpp"

#include <glad/glad.h>

#include <algorithm>
#include <filesystem>
#include <cmath>
#include <vector>
#include <optional>
#include <variant>

export module SoldierRenderer;

import Texture;
import Renderer;
import SpritesManager;
import SoldierPartData;
import Shader;

export namespace Soldank
{
class SoldierRenderer
{
public:
    SoldierRenderer(const Sprites::SpriteManager& sprite_manager);
    ~SoldierRenderer();

    // it's not safe to be able to copy/move this because we would also need to take care of the
    // created OpenGL buffers and textures
    SoldierRenderer(const SoldierRenderer&) = delete;
    SoldierRenderer& operator=(SoldierRenderer other) = delete;
    SoldierRenderer(SoldierRenderer&&) = delete;
    SoldierRenderer& operator=(SoldierRenderer&& other) = delete;

    static void GenerateVertices(std::vector<float>& vertices,
                                 const Sprites::SoldierPartData& part_data,
                                 bool flipped);

    void Render(glm::mat4 transform,
                const Sprites::SpriteManager& sprite_manager,
                const Soldier& soldier,
                double frame_percent);

private:
    static bool IsSoldierPartTypeVisible(Sprites::SoldierPartSpriteType soldier_part_type,
                                         const Soldier& soldier,
                                         bool part_base_visibility);
    static bool IsPrimaryWeaponTypeVisible(
      Sprites::SoldierPartPrimaryWeaponSpriteType soldier_part_type,
      const Soldier& soldier,
      bool part_base_visibility);
    static bool IsSecondaryWeaponTypeVisible(
      Sprites::SoldierPartSecondaryWeaponSpriteType soldier_part_type,
      const Soldier& soldier);
    static bool IsTertiaryWeaponTypeVisible(
      Sprites::SoldierPartTertiaryWeaponSpriteType soldier_part_type,
      const Soldier& soldier);

    static glm::vec4 GetColorForSoldierPart(const Soldier& soldier,
                                            Sprites::SoldierSpriteColor soldier_color,
                                            Sprites::SoldierSpriteAlpha soldier_alpha);

    static const Weapon& GetPrimaryWeapon(const Soldier& soldier)
    {
        return soldier.weapons[soldier.active_weapon];
    }

    static const Weapon& GetSecondaryWeapon(const Soldier& soldier)
    {
        return soldier.weapons[(soldier.active_weapon + 1) % 2];
    }

    static const Weapon& GetTertiaryWeapon(const Soldier& soldier) { return soldier.weapons[2]; }

    Shader shader_;

    std::vector<std::pair<unsigned int, std::optional<unsigned int>>> vbos_;
    std::vector<unsigned int> ebos_;
    std::vector<Sprites::SoldierPartSpriteType> part_types_;
};
} // namespace Soldank

namespace Soldank
{
SoldierRenderer::SoldierRenderer(const Sprites::SpriteManager& sprite_manager)
    : shader_(ShaderSources::DYNAMIC_COLOR_VERTEX_SHADER_SOURCE,
              ShaderSources::DYNAMIC_COLOR_FRAGMENT_SHADER_SOURCE)
{
    std::vector<float> vertices;
    std::vector<unsigned int> indices{ 0, 1, 2, 1, 3, 2 };

    for (unsigned int i = 0; i < sprite_manager.GetSoldierPartCount(); i++) {
        vertices.clear();

        const Sprites::SoldierPartData* part_data = sprite_manager.GetSoldierPartData(i);
        if (part_data == nullptr) {
            vbos_.emplace_back(0, 0);
            ebos_.push_back(0);
            continue;
        }

        GenerateVertices(vertices, *part_data, false);
        unsigned int vbo_for_original = Renderer::CreateVBO(vertices, GL_STATIC_DRAW);
        std::optional<unsigned int> vbo_for_flipped = std::nullopt;
        if (part_data->IsFlippable()) {
            vertices.clear();
            GenerateVertices(vertices, *part_data, true);
            vbo_for_flipped = Renderer::CreateVBO(vertices, GL_STATIC_DRAW);
        }

        vbos_.emplace_back(vbo_for_original, vbo_for_flipped);
        ebos_.push_back(Renderer::CreateEBO(indices, GL_STATIC_DRAW));
    }
}

SoldierRenderer::~SoldierRenderer()
{
    for (unsigned int ebo : ebos_) {
        Renderer::FreeEBO(ebo);
    }
    for (auto vbo : vbos_) {
        Renderer::FreeVBO(vbo.first);
        if (vbo.second.has_value()) {
            Renderer::FreeVBO(*vbo.second);
        }
    }
}

void SoldierRenderer::GenerateVertices(std::vector<float>& vertices,
                                       const Sprites::SoldierPartData& part_data,
                                       bool flipped)
{
    float texture_width = NAN;
    float texture_height = NAN;
    float pivot_x = part_data.GetCenter().x;
    float pivot_y = (1.0F - part_data.GetCenter().y);
    if (!flipped) {
        texture_width = (float)part_data.GetTextureWidth();
        texture_height = (float)part_data.GetTextureHeight();
    } else {
        texture_width = (float)part_data.GetTextureFlippedWidth();
        texture_height = (float)part_data.GetTextureFlippedHeight();
        pivot_y = 1.0F - pivot_y;
    }
    pivot_x *= texture_width;
    pivot_y *= texture_height;

    float w0 = 0.0F - pivot_x;
    float w1 = texture_width - pivot_x;
    float h0 = 0.0F - pivot_y;
    float h1 = texture_height - pivot_y;

    // clang-format off
    vertices = {
        // position         // texture
        w0, h0, 1.0,        0.0, 0.0,
        w1, h0, 1.0,        1.0, 0.0,
        w0, h1, 1.0,        0.0, 1.0,
        w1, h1, 1.0,        1.0, 1.0,
    };
    // clang-format on
}

void SoldierRenderer::Render(glm::mat4 transform,
                             const Sprites::SpriteManager& sprite_manager,
                             const Soldier& soldier,
                             double frame_percent)
{
    shader_.Use();

    for (unsigned int i = 0; i < vbos_.size(); i++) {
        const Sprites::SoldierPartData* part_data = sprite_manager.GetSoldierPartData(i);
        if (part_data == nullptr) {
            continue;
        }

        bool part_base_visibility = part_data->IsVisible();
        auto part_type = sprite_manager.GetSoldierPartDataType(i);
        bool part_visible = std::visit(
          VisitOverload{
            [&soldier, part_base_visibility](Sprites::SoldierPartSpriteType soldier_part_type) {
                return SoldierRenderer::IsSoldierPartTypeVisible(
                  soldier_part_type, soldier, part_base_visibility);
            },
            [&soldier,
             part_base_visibility](Sprites::SoldierPartPrimaryWeaponSpriteType weapon_type) {
                return SoldierRenderer::IsPrimaryWeaponTypeVisible(
                  weapon_type, soldier, part_base_visibility);
            },
            [&soldier](Sprites::SoldierPartSecondaryWeaponSpriteType weapon_type) {
                return SoldierRenderer::IsSecondaryWeaponTypeVisible(weapon_type, soldier);
            },
            [&soldier](Sprites::SoldierPartTertiaryWeaponSpriteType weapon_type) {
                return SoldierRenderer::IsTertiaryWeaponTypeVisible(weapon_type, soldier);
            } },
          part_type);

        if (!part_visible) {
            continue;
        }

        Sprites::SoldierSpriteColor part_soldier_color = part_data->GetSoldierColor();
        Sprites::SoldierSpriteAlpha part_soldier_alpha = part_data->GetSoldierAlpha();
        glm::vec4 part_color = std::visit(
          VisitOverload{ [&soldier, part_soldier_color, part_soldier_alpha](
                           Sprites::SoldierPartSpriteType /*soldier_part_type*/) {
                            return SoldierRenderer::GetColorForSoldierPart(
                              soldier, part_soldier_color, part_soldier_alpha);
                        },
                         [](Sprites::SoldierPartPrimaryWeaponSpriteType /*weapon_type*/) {
                             return glm::vec4{ 1.0F, 1.0F, 1.0F, 1.0F };
                         },
                         [](Sprites::SoldierPartSecondaryWeaponSpriteType /*weapon_type*/) {
                             return glm::vec4{ 1.0F, 1.0F, 1.0F, 1.0F };
                         },
                         [](Sprites::SoldierPartTertiaryWeaponSpriteType /*weapon_type*/) {
                             return glm::vec4{ 1.0F, 1.0F, 1.0F, 1.0F };
                         } },
          part_type);

        unsigned int sprite_texture = part_data->GetTexture();
        glm::vec2 scale = glm::vec2(1.0, 1.0);
        unsigned int px = part_data->GetPoint().x;
        unsigned int py = part_data->GetPoint().y;
        glm::vec2 p0 = Calc::Lerp(
          soldier.skeleton->GetOldPos(px), soldier.skeleton->GetPos(px), (float)frame_percent);
        glm::vec2 p1 = Calc::Lerp(
          soldier.skeleton->GetOldPos(py), soldier.skeleton->GetPos(py), (float)frame_percent);
        float rot = Calc::Vec2Angle(p1 - p0);

        unsigned int vbo_to_use = vbos_[i].first;
        if (soldier.direction != 1) {
            if (part_data->IsFlippable()) {
                sprite_texture = part_data->GetTextureFlipped();
                vbo_to_use = *vbos_[i].second;
            } else {
                scale.y = -1.0;
            }
        }

        if (part_data->GetFlexibility() > 0.0) {
            scale.x = std::min(1.5F, glm::length(p1 - p0) / part_data->GetFlexibility());
        }

        // TODO: magic numbers, this is in mod.ini
        scale.x /= 4.5;
        scale.y /= 4.5;

        Renderer::SetupVertexArray(vbo_to_use, ebos_[i], false, true);
        glm::mat4 current_part_transform = transform;

        current_part_transform =
          glm::translate(current_part_transform, glm::vec3(p0.x, -p0.y - 1.0, 0.0));
        current_part_transform = glm::rotate(current_part_transform, rot, glm::vec3(0.0, 0.0, 1.0));
        current_part_transform =
          glm::scale(current_part_transform, glm::vec3(scale.x, scale.y, 0.0));

        shader_.SetMatrix4("transform", current_part_transform);
        shader_.SetVec4("color", part_color);

        Renderer::BindTexture(sprite_texture);
        Renderer::DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, std::nullopt);
    }
}

bool SoldierRenderer::IsSoldierPartTypeVisible(Sprites::SoldierPartSpriteType soldier_part_type,
                                               const Soldier& soldier,
                                               bool part_base_visibility)
{
    bool has_blood = false; // TODO: need to get real value
    if (has_blood) {
        switch (soldier_part_type) {
            case Sprites::SoldierPartSpriteType::RannyUdo:
            case Sprites::SoldierPartSpriteType::RannyNoga:
            case Sprites::SoldierPartSpriteType::RannyRamie:
            case Sprites::SoldierPartSpriteType::RannyReka:
            case Sprites::SoldierPartSpriteType::RannyKlata:
            case Sprites::SoldierPartSpriteType::RannyBiodro:
            case Sprites::SoldierPartSpriteType::RannyMorda:
                return true;
            default:
                break;
        }
    }

    if (soldier.control.jets && soldier.jets_count > 0) {
        switch (soldier_part_type) {
            case Sprites::SoldierPartSpriteType::Stopa:
                return false;
            case Sprites::SoldierPartSpriteType::Lecistopa:
                return true;
            default:
                break;
        }
    }

    if (soldier.vest > 0.0F && soldier_part_type == Sprites::SoldierPartSpriteType::Kamizelka) {
        return true;
    }

    return part_base_visibility;
}

// TODO: rework this to something more readable
bool SoldierRenderer::IsPrimaryWeaponTypeVisible(
  Sprites::SoldierPartPrimaryWeaponSpriteType soldier_part_type,
  const Soldier& soldier,
  bool part_base_visibility)
{
    auto primary_weapon_type = GetPrimaryWeapon(soldier).GetWeaponParameters().kind;
    int ammo = GetPrimaryWeapon(soldier).GetAmmoCount();
    std::uint16_t reload_time_count = GetPrimaryWeapon(soldier).GetReloadTimeCount();

    if (primary_weapon_type == WeaponType::Minigun) {
        if (soldier_part_type == Sprites::SoldierPartPrimaryWeaponSpriteType::Minigun) {
            return true;
        }

        if (ammo > 0 || (ammo == 0 && reload_time_count < 65)) {
            if (soldier_part_type == Sprites::SoldierPartPrimaryWeaponSpriteType::MinigunClip) {
                return true;
            }
        }

        if (soldier.fired > 0) {
            if (soldier_part_type == Sprites::SoldierPartPrimaryWeaponSpriteType::MinigunFire) {
                return true;
            }
        }
    } else if (primary_weapon_type == WeaponType::Bow ||
               primary_weapon_type == WeaponType::FlameBow) {
        if (ammo == 0) {
            if (soldier_part_type == Sprites::SoldierPartPrimaryWeaponSpriteType::BowArrowReload) {
                return true;
            }
        } else {
            if (soldier_part_type == Sprites::SoldierPartPrimaryWeaponSpriteType::BowArrow) {
                return true;
            }
        }

        if (soldier.body_animation->GetType() == AnimationType::ReloadBow) {
            if (soldier_part_type == Sprites::SoldierPartPrimaryWeaponSpriteType::BowReload) {
                return true;
            }
            if (soldier_part_type == Sprites::SoldierPartPrimaryWeaponSpriteType::BowStringReload) {
                return true;
            }
        } else {
            if (soldier_part_type == Sprites::SoldierPartPrimaryWeaponSpriteType::Bow) {
                return true;
            }
            if (soldier_part_type == Sprites::SoldierPartPrimaryWeaponSpriteType::BowString) {
                return true;
            }
        }

        if (soldier.fired > 0) {
            if (soldier_part_type == Sprites::SoldierPartPrimaryWeaponSpriteType::BowFire) {
                return true;
            }
        }
    } else if (!soldier.dead_meat) {
        std::array<WeaponType, 14> weapon_types{
            WeaponType::DesertEagles, WeaponType::MP5,     WeaponType::Ak74,  WeaponType::SteyrAUG,
            WeaponType::Spas12,       WeaponType::Ruger77, WeaponType::M79,   WeaponType::Barrett,
            WeaponType::Minimi,       WeaponType::USSOCOM, WeaponType::Knife, WeaponType::Chainsaw,
            WeaponType::LAW,          WeaponType::Flamer
        };

        std::array<Sprites::SoldierPartPrimaryWeaponSpriteType, 14>
          soldier_part_primary_weapon_types{ Sprites::SoldierPartPrimaryWeaponSpriteType::Deagles,
                                             Sprites::SoldierPartPrimaryWeaponSpriteType::Mp5,
                                             Sprites::SoldierPartPrimaryWeaponSpriteType::Ak74,
                                             Sprites::SoldierPartPrimaryWeaponSpriteType::Steyr,
                                             Sprites::SoldierPartPrimaryWeaponSpriteType::Spas,
                                             Sprites::SoldierPartPrimaryWeaponSpriteType::Ruger,
                                             Sprites::SoldierPartPrimaryWeaponSpriteType::M79,
                                             Sprites::SoldierPartPrimaryWeaponSpriteType::Barrett,
                                             Sprites::SoldierPartPrimaryWeaponSpriteType::Minimi,
                                             Sprites::SoldierPartPrimaryWeaponSpriteType::Socom,
                                             Sprites::SoldierPartPrimaryWeaponSpriteType::Knife,
                                             Sprites::SoldierPartPrimaryWeaponSpriteType::Chainsaw,
                                             Sprites::SoldierPartPrimaryWeaponSpriteType::Law,
                                             Sprites::SoldierPartPrimaryWeaponSpriteType::Flamer };

        if (std::ranges::contains(soldier_part_primary_weapon_types, soldier_part_type)) {
            for (int i = 0; i < (int)soldier_part_primary_weapon_types.size(); i++) {
                if (soldier_part_primary_weapon_types.at(i) == soldier_part_type) {
                    return weapon_types.at(i) == primary_weapon_type;
                }
            }
        }

        std::array<Sprites::SoldierPartPrimaryWeaponSpriteType, 14>
          soldier_part_primary_weapon_clip_types{
              Sprites::SoldierPartPrimaryWeaponSpriteType::DeaglesClip,
              Sprites::SoldierPartPrimaryWeaponSpriteType::Mp5Clip,
              Sprites::SoldierPartPrimaryWeaponSpriteType::Ak74Clip,
              Sprites::SoldierPartPrimaryWeaponSpriteType::SteyrClip,
              Sprites::SoldierPartPrimaryWeaponSpriteType::SpasClip,
              Sprites::SoldierPartPrimaryWeaponSpriteType::RugerClip,
              Sprites::SoldierPartPrimaryWeaponSpriteType::M79Clip,
              Sprites::SoldierPartPrimaryWeaponSpriteType::BarrettClip,
              Sprites::SoldierPartPrimaryWeaponSpriteType::MinimiClip,
              Sprites::SoldierPartPrimaryWeaponSpriteType::SocomClip,
              Sprites::SoldierPartPrimaryWeaponSpriteType::KnifeClip,
              Sprites::SoldierPartPrimaryWeaponSpriteType::ChainsawClip,
              Sprites::SoldierPartPrimaryWeaponSpriteType::LawClip,
              Sprites::SoldierPartPrimaryWeaponSpriteType::FlamerClip
          };

        auto reload_count = GetPrimaryWeapon(soldier).GetReloadTimeCount();
        auto clip_in_time = GetPrimaryWeapon(soldier).GetClipInTime();
        auto clip_out_time = GetPrimaryWeapon(soldier).GetClipOutTime();

        if (std::ranges::contains(soldier_part_primary_weapon_clip_types, soldier_part_type)) {
            for (int i = 0; i < (int)soldier_part_primary_weapon_clip_types.size(); i++) {
                if (soldier_part_primary_weapon_clip_types.at(i) == soldier_part_type) {
                    if (weapon_types.at(i) == primary_weapon_type) {
                        if (ammo > 0 || (ammo == 0 && (reload_count < clip_in_time ||
                                                       reload_count > clip_out_time))) {
                            return true;
                        }
                    }
                }
            }
        }

        std::array<Sprites::SoldierPartPrimaryWeaponSpriteType, 14>
          soldier_part_primary_weapon_fire_types{
              Sprites::SoldierPartPrimaryWeaponSpriteType::DeaglesFire,
              Sprites::SoldierPartPrimaryWeaponSpriteType::Mp5Fire,
              Sprites::SoldierPartPrimaryWeaponSpriteType::Ak74Fire,
              Sprites::SoldierPartPrimaryWeaponSpriteType::SteyrFire,
              Sprites::SoldierPartPrimaryWeaponSpriteType::SpasFire,
              Sprites::SoldierPartPrimaryWeaponSpriteType::RugerFire,
              Sprites::SoldierPartPrimaryWeaponSpriteType::M79Fire,
              Sprites::SoldierPartPrimaryWeaponSpriteType::BarrettFire,
              Sprites::SoldierPartPrimaryWeaponSpriteType::MinimiFire,
              Sprites::SoldierPartPrimaryWeaponSpriteType::SocomFire,
              Sprites::SoldierPartPrimaryWeaponSpriteType::KnifeFire,
              Sprites::SoldierPartPrimaryWeaponSpriteType::ChainsawFire,
              Sprites::SoldierPartPrimaryWeaponSpriteType::LawFire,
              Sprites::SoldierPartPrimaryWeaponSpriteType::FlamerFire
          };

        if (std::ranges::contains(soldier_part_primary_weapon_fire_types, soldier_part_type)) {
            for (int i = 0; i < (int)soldier_part_primary_weapon_fire_types.size(); i++) {
                if (soldier_part_primary_weapon_fire_types.at(i) == soldier_part_type) {
                    if (weapon_types.at(i) == primary_weapon_type) {
                        if (soldier.fired > 0) {
                            return true;
                        }
                    }
                }
            }
        }
    }

    return part_base_visibility;
}

bool SoldierRenderer::IsSecondaryWeaponTypeVisible(
  Sprites::SoldierPartSecondaryWeaponSpriteType soldier_part_type,
  const Soldier& soldier)
{
    auto secondary_weapon_type = GetSecondaryWeapon(soldier).GetWeaponParameters().kind;
    switch (soldier_part_type) {
        case Sprites::SoldierPartSecondaryWeaponSpriteType::Deagles:
            return secondary_weapon_type == WeaponType::DesertEagles;
        case Sprites::SoldierPartSecondaryWeaponSpriteType::Mp5:
            return secondary_weapon_type == WeaponType::MP5;
        case Sprites::SoldierPartSecondaryWeaponSpriteType::Ak74:
            return secondary_weapon_type == WeaponType::Ak74;
        case Sprites::SoldierPartSecondaryWeaponSpriteType::Steyr:
            return secondary_weapon_type == WeaponType::SteyrAUG;
        case Sprites::SoldierPartSecondaryWeaponSpriteType::Spas:
            return secondary_weapon_type == WeaponType::Spas12;
        case Sprites::SoldierPartSecondaryWeaponSpriteType::Ruger:
            return secondary_weapon_type == WeaponType::Ruger77;
        case Sprites::SoldierPartSecondaryWeaponSpriteType::M79:
            return secondary_weapon_type == WeaponType::M79;
        case Sprites::SoldierPartSecondaryWeaponSpriteType::Barrett:
            return secondary_weapon_type == WeaponType::Barrett;
        case Sprites::SoldierPartSecondaryWeaponSpriteType::Minimi:
            return secondary_weapon_type == WeaponType::Minimi;
        case Sprites::SoldierPartSecondaryWeaponSpriteType::Minigun:
            return secondary_weapon_type == WeaponType::Minigun;
        case Sprites::SoldierPartSecondaryWeaponSpriteType::Socom:
            return secondary_weapon_type == WeaponType::USSOCOM;
        case Sprites::SoldierPartSecondaryWeaponSpriteType::Knife:
            return secondary_weapon_type == WeaponType::Knife;
        case Sprites::SoldierPartSecondaryWeaponSpriteType::Chainsaw:
            return secondary_weapon_type == WeaponType::Chainsaw;
        case Sprites::SoldierPartSecondaryWeaponSpriteType::Law:
            return secondary_weapon_type == WeaponType::LAW;
        case Sprites::SoldierPartSecondaryWeaponSpriteType::Flamebow:
            return secondary_weapon_type == WeaponType::FlameBow;
        case Sprites::SoldierPartSecondaryWeaponSpriteType::Bow:
            return secondary_weapon_type == WeaponType::Bow;
        case Sprites::SoldierPartSecondaryWeaponSpriteType::Flamer:
            return secondary_weapon_type == WeaponType::Flamer;
    }
}

bool SoldierRenderer::IsTertiaryWeaponTypeVisible(
  Sprites::SoldierPartTertiaryWeaponSpriteType soldier_part_type,
  const Soldier& soldier)
{
    auto tertiary_weapon_type = GetTertiaryWeapon(soldier).GetWeaponParameters().kind;
    int ammo = GetTertiaryWeapon(soldier).GetAmmoCount();
    if (tertiary_weapon_type == WeaponType::FragGrenade) {
        std::vector<Sprites::SoldierPartTertiaryWeaponSpriteType> sprite_types{
            Sprites::SoldierPartTertiaryWeaponSpriteType::FragGrenade1,
            Sprites::SoldierPartTertiaryWeaponSpriteType::FragGrenade2,
            Sprites::SoldierPartTertiaryWeaponSpriteType::FragGrenade3,
            Sprites::SoldierPartTertiaryWeaponSpriteType::FragGrenade4,
            Sprites::SoldierPartTertiaryWeaponSpriteType::FragGrenade5
        };

        int n = 0;
        if (soldier.body_animation->GetType() == AnimationType::Throw) {
            n = std::min(5, ammo - 1);
        } else {
            n = std::min(5, ammo);
        }

        for (int i = 0; i < (int)sprite_types.size(); i++) {
            if (sprite_types[i] == soldier_part_type) {
                return i < n;
            }
        }
    } else if (tertiary_weapon_type == WeaponType::ClusterGrenade) {
        std::vector<Sprites::SoldierPartTertiaryWeaponSpriteType> sprite_types{
            Sprites::SoldierPartTertiaryWeaponSpriteType::ClusterGrenade1,
            Sprites::SoldierPartTertiaryWeaponSpriteType::ClusterGrenade2,
            Sprites::SoldierPartTertiaryWeaponSpriteType::ClusterGrenade3,
            Sprites::SoldierPartTertiaryWeaponSpriteType::ClusterGrenade4,
            Sprites::SoldierPartTertiaryWeaponSpriteType::ClusterGrenade5
        };

        int n = 0;
        if (soldier.body_animation->GetType() == AnimationType::Throw) {
            n = std::min(5, ammo - 1);
        } else {
            n = std::min(5, ammo);
        }

        for (int i = 0; i < (int)sprite_types.size(); i++) {
            if (sprite_types[i] == soldier_part_type) {
                return i < n;
            }
        }
    }

    return false;
}

glm::vec4 SoldierRenderer::GetColorForSoldierPart(const Soldier& soldier,
                                                  Sprites::SoldierSpriteColor soldier_color,
                                                  Sprites::SoldierSpriteAlpha soldier_alpha)
{
    auto alpha_base = soldier.alpha;
    auto alpha_blood = std::max(0.0F, std::min(255.0F, 200.0F - soldier.health));

    glm::vec3 color;
    switch (soldier_color) {
        case Sprites::SoldierSpriteColor::Cygar:
            if (soldier.has_cigar != 0) {
                color = { 97.0F, 97.0F, 97.0F };
            } else {
                color = { 255.0F, 255.0F, 255.0F };
            }
            break;
        case Sprites::SoldierSpriteColor::None:
            color = { 255.0F, 255.0F, 255.0F };
            break;
        case Sprites::SoldierSpriteColor::Main:  // TODO: Player.Color1
        case Sprites::SoldierSpriteColor::Pants: // TODO: Player.Color2
        case Sprites::SoldierSpriteColor::Hair:  // TODO: Player.HairColor
            color = { 0.0F, 0.0F, 0.0F };
            break;
        case Sprites::SoldierSpriteColor::Skin: // TODO: Player.SkinColor
            color = { 230.0F, 180.0F, 120.0F };
            break;
        case Sprites::SoldierSpriteColor::Headblood:
            color = { 172.0F, 169.0F, 168.0F };
            break;
    }

    bool realistic_mode = false; // TODO: get real value

    if (soldier.health > (90.0F - 40.0F * (float)realistic_mode)) {
        alpha_blood = 0;
    }

    if (realistic_mode && soldier.visible > 0 && soldier.visible < 45 && soldier.alpha > 60) {
        // TODO: if this really needs to change it should be done somewhere during update, not
        // here soldier.alpha = 3 * soldier.visible;
        alpha_base = 3 * soldier.visible;
        alpha_blood = 0;
    }

    float alpha_nades = (0.75F * (float)alpha_base);

    float alpha = 0;
    switch (soldier_alpha) {
        case Sprites::SoldierSpriteAlpha::Base:
            alpha = alpha_base;
            break;
        case Sprites::SoldierSpriteAlpha::Blood:
            alpha = alpha_blood;
            break;
        case Sprites::SoldierSpriteAlpha::Nades:
            alpha = alpha_nades;
            break;
    }

    return { color.x / 255.0F, color.y / 255.0F, color.z / 255.0F, alpha / 255.0F };
}
} // namespace Soldank
