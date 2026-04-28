module;

#include "core/math/Glm.hpp"

#include <utility>
#include <algorithm>
#include <vector>
#include <memory>
#include <optional>

export module Shared.Core.Animations:AnimationState;

import :AnimationData;

import Shared.Core.State.Control;
import Shared.Core.Entities.Weapon;
import Shared.Core.Types.WeaponType;

export namespace Soldank
{
class AnimationState
{
public:
    AnimationState(std::shared_ptr<const AnimationData> animation_data)
        : animation_data_(std::move(animation_data))
        , speed_(animation_data_->GetSpeed())
        , count_(0)
        , frame_(1)
    {
    }

    AnimationState(const AnimationState& other) = default;

    virtual ~AnimationState() = default;

    void DoAnimation()
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

    const glm::vec2& GetPosition(unsigned int index) const
    {
        return animation_data_->GetFrames().at(frame_ - 1).positions.at(index - 1);
    }

    unsigned int GetFramesCount() const { return animation_data_->GetFrames().size(); }

    bool IsAny(const std::vector<AnimationType>& animations) const
    {
        return std::ranges::any_of(animations, [this](AnimationType animation_type) {
            return animation_type == animation_data_->GetAnimationType();
        });
    }

    AnimationType GetType() const { return animation_data_->GetAnimationType(); };

    int GetSpeed() const { return speed_; }
    void SetSpeed(int new_speed) { speed_ = new_speed; }

    unsigned int GetFrame() const { return frame_; }
    void SetFrame(unsigned int new_frame) { frame_ = new_frame; }
    void SetNextFrame() { frame_++; }

    int GetCount() const { return count_; }
    void SetCount(int new_count) { count_ = new_count; }

    struct TryToShootParams
    {
        std::vector<Weapon> weapons;
        std::uint8_t active_weapon;
        bool fire;
        bool should_fire_primary_weapon;
    };

    // This method sets should_fire_primary_weapon if the player can and wants to shoot.
    // It has a separate method for that because it's a shared functionality between many animation
    // types and players are allowed to shoot during animations.
    void TryToShoot(TryToShootParams& params) const
    {
        if (params.weapons[params.active_weapon].GetWeaponParameters().kind ==
              WeaponType::NoWeapon ||
            params.weapons[params.active_weapon].GetWeaponParameters().kind == WeaponType::Knife) {
            return;
        }

        if (params.fire && IsSoldierShootingPossible(params.weapons, params.active_weapon)) {
            params.should_fire_primary_weapon = true;
        }
    }

    struct TryToThrowFlagsParams
    {
        bool flag_throw;
        bool is_holding_flags;
        bool should_throw_flags;
    };

    // This method sets should_throw_flags if the player can and wants to throw flags.
    // It has a separate method for that because it's a shared functionality between many animation
    // types and players are allowed to throw during animations.
    void TryToThrowFlags(TryToThrowFlagsParams& params) const
    {
        if (IsSoldierFlagThrowingPossible()) {
            if (params.flag_throw && params.is_holding_flags) {
                params.should_throw_flags = true;
            }
        }
    }

    struct HandleInputParams
    {
        Control control;
        std::vector<Weapon> weapons;
        std::uint8_t stance;
        std::uint8_t active_weapon;
        bool on_ground;
        std::int8_t direction;
        std::int8_t old_direction;
        std::int32_t jets_count;
        AnimationType legs_animation_type;
        bool grenade_can_throw;
    };

    struct Transition
    {
        AnimationType animation_type;
        std::optional<unsigned int> initial_frame;
    };

    struct UpdateParams
    {
        Control control;
        std::int8_t direction;
        bool on_ground;
        std::uint8_t stance;
        glm::vec2 velocity;
        glm::vec2 force;
        bool should_switch_weapon;
    };

    struct EnterParams
    {
        bool on_ground;
        std::int8_t direction;
        glm::vec2 force;
        bool grenade_can_throw;
        std::vector<Weapon> weapons;
        std::uint8_t active_weapon;
    };

    virtual void Enter(EnterParams& params) {}
    virtual std::optional<Transition> HandleInput(HandleInputParams& params) = 0;
    struct ExitParams
    {
        bool should_throw_active_weapon;
    };

    virtual void Update(UpdateParams& params) {}
    virtual void Exit(ExitParams& params) {}

protected:
    virtual bool IsSoldierShootingPossible(const std::vector<Weapon>& weapons,
                                           std::uint8_t active_weapon) const
    {
        return false;
    }

    virtual bool IsSoldierFlagThrowingPossible() const { return false; }

    std::shared_ptr<const AnimationData> animation_data_;

    int speed_;
    int count_;
    unsigned int frame_;
};
} // namespace Soldank
