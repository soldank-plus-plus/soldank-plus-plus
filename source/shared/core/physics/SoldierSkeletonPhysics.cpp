module;

#include <cmath>
#include <vector>

export module Shared.Core.Physics.SoldierSkeletonPhysics;

import Extern.Glm;

import Shared.Core.Physics.Constants;
import Shared.Core.Entities.Soldier;
import Shared.Core.Math.Calc;
import Shared.Core.Animations;

export namespace Soldank
{
void RepositionSoldierSkeletonParts(Soldier& soldier)
{
    float body_y = 0.0F;
    float arm_s = NAN;

    soldier.skeleton->SetOldPos(21, soldier.skeleton->GetPos(21));
    soldier.skeleton->SetOldPos(23, soldier.skeleton->GetPos(23));
    // skeleton->SetOldPos(25, skeleton->GetPos(25));
    soldier.skeleton->SetPos(21, soldier.skeleton->GetPos(9));
    soldier.skeleton->SetPos(23, soldier.skeleton->GetPos(12));
    // skeleton->SetPos(25, skeleton->GetPos(5));

    if (!soldier.dead_meat) {
        soldier.skeleton->SetPos(21, soldier.skeleton->GetPos(21) + soldier.particle.velocity_);
        soldier.skeleton->SetPos(23, soldier.skeleton->GetPos(23) + soldier.particle.velocity_);
        // skeleton->SetPos(25, skeleton.GetPos(25) + particle.velocity);
    }
    switch (soldier.stance) {
        case PhysicsConstants::STANCE_STAND:
            body_y = 8.0F;
            break;
        case PhysicsConstants::STANCE_CROUCH:
            body_y = 9.0F;
            break;
        case PhysicsConstants::STANCE_PRONE: {
            if (soldier.body_animation->GetType() == AnimationType::Prone) {
                if (soldier.body_animation->GetFrame() > 9) {
                    body_y = -2.0F;
                } else {
                    body_y = 14.0F - (float)soldier.body_animation->GetFrame();
                }
            } else {
                body_y = 9.0F;
            }

            if (soldier.body_animation->GetType() == AnimationType::ProneMove) {
                body_y = 0.0F;
            }
        }
    }
    if (soldier.body_animation->GetType() == AnimationType::GetUp) {
        if (soldier.body_animation->GetFrame() > 18) {
            body_y = 8.0F;
        } else {
            body_y = 4.0F;
        }
    }

    if ((float)soldier.control.mouse_aim_x >= soldier.particle.position.x) {
        soldier.direction = 1;
    } else {
        soldier.direction = -1;
    }

    for (int i = 1; i < 21; i++) {
        if (soldier.skeleton->GetActive(i) && !soldier.dead_meat) {
            glm::vec2 pos = glm::vec2(0.0F, 0.0F);
            soldier.skeleton->SetOldPos(i, soldier.skeleton->GetPos(i));

            if (!soldier.half_dead && ((i >= 1 && i <= 6) || (i == 17) || (i == 18))) {
                glm::vec2 anim_pos = soldier.legs_animation->GetPosition(i);
                pos.x = soldier.particle.position.x + anim_pos.x * (float)soldier.direction;
                pos.y = soldier.particle.position.y + anim_pos.y;
            }

            if (((i >= 7) && (i <= 16)) || (i == 19) || (i == 20)) {
                auto anim_pos = soldier.body_animation->GetPosition(i);
                pos.x = soldier.particle.position.x + anim_pos.x * (float)soldier.direction;
                pos.y = soldier.particle.position.y + anim_pos.y;

                if (soldier.half_dead) {
                    pos.y += 9.0F;
                } else {
                    pos.y += soldier.skeleton->GetPos(6).y - (soldier.particle.position.y - body_y);
                }
            }

            soldier.skeleton->SetPos(i, pos);
        }
    }

    glm::vec2 aim(soldier.control.mouse_aim_x, soldier.control.mouse_aim_y);

    if (!soldier.dead_meat) {
        auto pos = soldier.skeleton->GetPos(9);
        auto r_norm = 0.1F * Calc::Vec2Normalize(soldier.skeleton->GetPos(12) - aim);
        auto dir = (float)soldier.direction;

        soldier.skeleton->SetPos(12, pos + glm::vec2(-dir * r_norm.y, dir * r_norm.x));
        soldier.skeleton->SetPos(23, pos + glm::vec2(-dir * r_norm.y, dir * r_norm.x) * 50.0F);
    }

    auto not_aiming_anims = std::vector{
        AnimationType::Reload,      AnimationType::ReloadBow, AnimationType::ClipIn,
        AnimationType::ClipOut,     AnimationType::SlideBack, AnimationType::Change,
        AnimationType::ThrowWeapon, AnimationType::Punch,     AnimationType::Roll,
        AnimationType::RollBack,    AnimationType::Cigar,     AnimationType::Match,
        AnimationType::Smoke,       AnimationType::Wipe,      AnimationType::TakeOff,
        AnimationType::Groin,       AnimationType::Piss,      AnimationType::Mercy,
        AnimationType::Mercy2,      AnimationType::Victory,   AnimationType::Own,
        AnimationType::Melee,
    };

    if (soldier.body_animation->GetType() == AnimationType::Throw) {
        arm_s = -5.00F;
    } else {
        arm_s = -7.00F;
    }

    if (!soldier.body_animation->IsAny(not_aiming_anims)) {
        auto r_norm = arm_s * Calc::Vec2Normalize(soldier.skeleton->GetPos(15) - aim);
        soldier.skeleton->SetPos(15, soldier.skeleton->GetPos(16) + r_norm);
    }
    if (soldier.body_animation->GetType() == AnimationType::Throw) {
        arm_s = -6.00F;
    } else {
        arm_s = -8.00F;
    }

    if (!soldier.body_animation->IsAny(not_aiming_anims)) {
        auto r_norm = arm_s * Calc::Vec2Normalize(soldier.skeleton->GetPos(19) - aim);
        soldier.skeleton->SetPos(19, soldier.skeleton->GetPos(16) - glm::vec2(0.0F, 4.0F) + r_norm);
    }
}
} // namespace Soldank
