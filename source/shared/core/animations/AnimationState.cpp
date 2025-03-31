#include "core/animations/AnimationState.hpp"

#include "core/entities/Soldier.hpp"

#include "core/physics/PhysicsEvents.hpp"

#include <utility>
#include <algorithm>

namespace Soldank
{
AnimationState::AnimationState(std::shared_ptr<const AnimationData> animation_data)
    : animation_data_(std::move(animation_data))
    , speed_(animation_data_->GetSpeed())
    , count_(0)
    , frame_(1)
{
}

void AnimationState::DoAnimation()
{
    count_ += 1;

    if (count_ == speed_) {
        count_ = 0;
        frame_ += 1;

        if (frame_ > GetFramesCount()) {
            if (animation_data_->GetLooped()) {
                frame_ = 1;
            } else {
                frame_ = GetFramesCount();
            }
        }
    }
}

const glm::vec2& AnimationState::GetPosition(unsigned int index) const
{
    return animation_data_->GetFrames().at(frame_ - 1).positions.at(index - 1);
}

unsigned int AnimationState::GetFramesCount() const
{
    return animation_data_->GetFrames().size();
}

bool AnimationState::IsAny(const std::vector<AnimationType>& animations) const
{
    return std::ranges::any_of(animations, [this](AnimationType animation_type) {
        return animation_type == animation_data_->GetAnimationType();
    });
}

bool AnimationState::TryToShoot(Soldier& soldier, const PhysicsEvents& physics_events) const
{
    if (soldier.weapons[soldier.active_weapon].GetWeaponParameters().kind == WeaponType::NoWeapon ||
        soldier.weapons[soldier.active_weapon].GetWeaponParameters().kind == WeaponType::Knife) {

        // We don't spawn projectiles for punching or stabbing... These have to be handled by the
        // animation
        return false;
    }

    if (soldier.control.fire && IsSoldierShootingPossible(soldier)) {
        physics_events.soldier_fires_primary_weapon.Notify(soldier);
        return true;
    }

    return false;
}

void AnimationState::TryToThrowFlags(Soldier& soldier, const PhysicsEvents& physics_events) const
{
    if (IsSoldierFlagThrowingPossible(soldier)) {
        if (soldier.control.flag_throw && soldier.is_holding_flags) {
            physics_events.soldier_throws_flags.Notify(soldier);
        }
    }
}

void AnimationState::Enter(Soldier& soldier) {}

void AnimationState::Update(Soldier& soldier, const PhysicsEvents& physics_events) {}

void AnimationState::Exit(Soldier& soldier, const PhysicsEvents& physics_events) {}

bool AnimationState::IsSoldierShootingPossible(const Soldier& /*soldier*/) const
{
    // By default we return false. Child classes should determine whether the player is able to
    // shoot or not in the current state
    return false;
}

bool AnimationState::IsSoldierFlagThrowingPossible(const Soldier& /*soldier*/) const
{
    // By default we return false. Child classes should determine whether the player is able to
    // throw flags or not in the current state
    return false;
}
} // namespace Soldank
