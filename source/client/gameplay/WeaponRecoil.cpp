module;

#include <algorithm>
#include <cmath>
#include <numbers>

export module Gameplay.WeaponRecoil;

import Extern.Glm;
import Shared.Core.Entities.Soldier;
import Shared.Core.Animations;

export namespace Soldank
{
glm::vec2 ApplyWeaponRecoil(const Soldier& soldier, glm::vec2 player, glm::vec2 cursor)
{
    const auto& weapon = soldier.GetActiveWeapon();
    const auto& parameters = weapon.GetWeaponParameters();
    float recoil = static_cast<float>(weapon.GetState().burst_count) / 10.0F *
                   static_cast<float>(parameters.recoil);
    if (soldier.on_ground) {
        const auto legs = soldier.legs_animation->GetType();
        const auto frame = soldier.legs_animation->GetFrame();
        if (legs == AnimationType::Crouch && frame > 13) {
            recoil /= 2.0F;
        } else if (legs == AnimationType::Prone && frame > 23) {
            recoil /= 3.0F;
        }
    }
    if (recoil <= 0.0F) {
        return cursor;
    }
    const auto delta = cursor - player;
    const float radius = glm::length(delta);
    if (radius < 0.000001F) {
        return cursor;
    }
    const float angle_delta =
      -std::numbers::pi_v<float> *
      std::sin(parameters.speed * static_cast<float>(parameters.fire_interval) / 364.0F * recoil *
               std::numbers::pi_v<float> / 180.0F);
    const bool right = delta.x > 0.0F;
    float angle = std::atan2(delta.y, delta.x);
    if (!right && angle < 0.0F) {
        angle += 2.0F * std::numbers::pi_v<float>;
    }
    angle += right ? angle_delta : -angle_delta;
    angle = right ? std::min(angle, std::numbers::pi_v<float> / 2.0F)
                  : std::max(angle, std::numbers::pi_v<float> / 2.0F);
    auto result = player + (delta + glm::vec2(std::cos(angle), std::sin(angle)) * radius) / 2.0F;
    if (right && result.x <= player.x) {
        result.x = player.x + 0.0001F;
    } else if (!right && result.x >= player.x) {
        result.x = player.x - 0.0001F;
    }
    return result;
}
} // namespace Soldank
