module;

#include <algorithm>
#include <cmath>
#include <numbers>

export module SoldierVisuals;

import Extern.Glm;

export namespace Soldank
{
struct DreadlockTransform
{
    glm::vec2 anchor_offset{};
    float longitudinal_scale{ 1.0F };
};

// GostekGraphics.pas places each strand using head rotation, then rotates the strand
// along its own skeleton pair. The renderer's vertex buffer already contains a pivot.
DreadlockTransform GetDreadlockTransform(glm::vec2 head_delta,
                                         glm::vec2 center,
                                         glm::vec2 texture_size,
                                         float render_rotation,
                                         int direction,
                                         unsigned int strand)
{
    constexpr float ASSET_SCALE = 4.5F;
    const float head_rotation =
      std::atan2(head_delta.y, head_delta.x) - std::numbers::pi_v<float> / 2.0F;
    const auto pivot = center * texture_size / ASSET_SCALE;
    const glm::vec2 displacement(-pivot.y * direction, pivot.x);
    DreadlockTransform result;
    result.anchor_offset = {
        std::cos(head_rotation) * displacement.x - std::sin(head_rotation) * displacement.y,
        std::sin(head_rotation) * displacement.x + std::cos(head_rotation) * displacement.y
    };
    result.longitudinal_scale = 0.75F + 0.25F / 5.0F * std::min(strand, 4U);
    const glm::vec2 correction(pivot.x * result.longitudinal_scale,
                               (center.y - 0.5F) * texture_size.y / ASSET_SCALE * direction);
    result.anchor_offset += glm::vec2(
      std::cos(-render_rotation) * correction.x - std::sin(-render_rotation) * correction.y,
      std::sin(-render_rotation) * correction.x + std::cos(-render_rotation) * correction.y);
    return result;
}
} // namespace Soldank
