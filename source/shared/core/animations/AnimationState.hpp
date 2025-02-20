#ifndef __ANIMATION_HPP__
#define __ANIMATION_HPP__

#include "core/animations/AnimationData.hpp"

#include "core/math/Glm.hpp"

#include <vector>
#include <memory>
#include <optional>

namespace Soldank
{
struct Soldier;
struct PhysicsEvents;

class AnimationState
{
public:
    AnimationState(std::shared_ptr<const AnimationData> animation_data);
    AnimationState(const AnimationState& other) = default;
    virtual ~AnimationState() = default;

    void DoAnimation();
    const glm::vec2& GetPosition(unsigned int index) const;
    unsigned int GetFramesCount() const;
    bool IsAny(const std::vector<AnimationType>& animations) const;

    AnimationType GetType() const { return animation_data_->GetAnimationType(); };

    int GetSpeed() const { return speed_; }
    void SetSpeed(int new_speed) { speed_ = new_speed; }

    unsigned int GetFrame() const { return frame_; }
    void SetFrame(unsigned int new_frame) { frame_ = new_frame; }
    void SetNextFrame() { frame_++; }

    int GetCount() const { return count_; }
    void SetCount(int new_count) { count_ = new_count; }

    // This method calls a shooting event to spawn a projectile for the current primary weapon
    // if a player presses the fire button. It has a separate method for that because it's a shared
    // functionality between many animation types and players are allowed to shoot during
    // animations. That's why we need to handle it in a special way.
    void TryToShoot(Soldier& soldier, const PhysicsEvents& physics_events) const;

    // This method calls a flag throwing event.It has a separate method for that because it's a
    // shared functionality between many animation types and players are allowed to shoot during
    // animations. That's why we need to handle it in a special way.
    void TryToThrowFlags(Soldier& soldier, const PhysicsEvents& physics_events) const;

    virtual void Enter(Soldier& soldier);
    virtual std::optional<std::shared_ptr<AnimationState>> HandleInput(Soldier& soldier) = 0;
    virtual void Update(Soldier& soldier, const PhysicsEvents& physics_events);
    virtual void Exit(Soldier& soldier, const PhysicsEvents& physics_events);

protected:
    // This method should be overwritten by child classes to determine whether it's possible to
    // shoot during the current animation state
    virtual bool IsSoldierShootingPossible(const Soldier& soldier) const;

    // This method should be overwritten by child classes to determine whether it's possible to
    // throw all the flags during the current animation state
    virtual bool IsSoldierFlagThrowingPossible(const Soldier& soldier) const;

    std::shared_ptr<const AnimationData> animation_data_;

    int speed_;
    int count_;
    unsigned int frame_;
};
} // namespace Soldank

#endif
