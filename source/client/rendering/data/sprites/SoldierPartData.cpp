module;

#include "rendering/data/sprites/SpriteTypes.hpp"

#include "core/math/Glm.hpp"

#include <string>
#include <vector>
#include <optional>
#include <variant>

export module SoldierPartData;

import Texture;

export namespace Soldank::Sprites
{
class SoldierPartData
{
public:
    SoldierPartData(const Texture::TextureData& texture_data,
                    glm::uvec2 point,
                    glm::vec2 center,
                    bool visible,
                    const std::optional<Texture::TextureData>& flipped_texture_data,
                    bool team,
                    float flexibility,
                    SoldierSpriteColor color,
                    SoldierSpriteAlpha alpha)
        : point_(point)
        , center_(center)
        , visible_(visible)
        , flip_(flipped_texture_data.has_value())
        , team_(team)
        , flexibility_(flexibility)
        , color_(color)
        , alpha_(alpha)
        , texture_(texture_data.opengl_id)
        , texture_width_(texture_data.width)
        , texture_height_(texture_data.height)
    {
        if (flipped_texture_data.has_value()) {
            texture_flipped_ = flipped_texture_data->opengl_id;
            texture_flipped_width_ = flipped_texture_data->width;
            texture_flipped_height_ = flipped_texture_data->height;
        }
    }
    ~SoldierPartData() = default;

    // it's not safe to be able to copy/move this because we would also need to take care of the
    // created OpenGL buffers and textures
    SoldierPartData(const SoldierPartData&) = delete;
    SoldierPartData& operator=(SoldierPartData other) = delete;
    SoldierPartData(SoldierPartData&&) = delete;
    SoldierPartData& operator=(SoldierPartData&& other) = delete;

    glm::uvec2 GetPoint() const { return point_; }
    glm::vec2 GetCenter() const { return center_; }

    bool IsVisible() const { return visible_; }
    bool IsFlippable() const { return flip_; }

    float GetFlexibility() const { return flexibility_; }

    SoldierSpriteColor GetSoldierColor() const { return color_; }

    SoldierSpriteAlpha GetSoldierAlpha() const { return alpha_; }

    unsigned int GetTexture() const { return texture_; }
    unsigned int GetTextureFlipped() const { return texture_flipped_; }

    int GetTextureWidth() const { return texture_width_; }
    int GetTextureHeight() const { return texture_height_; }

    int GetTextureFlippedWidth() const { return texture_flipped_width_; }
    int GetTextureFlippedHeight() const { return texture_flipped_height_; }

private:
    glm::uvec2 point_;
    glm::vec2 center_;
    bool visible_;
    bool flip_;
    bool team_;
    float flexibility_;
    SoldierSpriteColor color_;
    SoldierSpriteAlpha alpha_;
    unsigned int texture_;
    unsigned int texture_flipped_;
    int texture_width_;
    int texture_height_;
    int texture_flipped_width_;
    int texture_flipped_height_;
};
} // namespace Soldank::Sprites
