module;

#include <gtest/gtest.h>

#include <ios>
#include <optional>
#include <string>
#include <vector>

#include "core/utility/Expected.hpp"

export module Tests.Shared.Core.Animations.AnimationStateTestHelpers;

import Shared.Core.Animations;
import Shared.Core.Data.IFileReader;
import Shared.Core.Entities.Weapon;
import Shared.Core.Entities.WeaponParameters;
import Extern.Glm;
import Shared.Core.Physics.Constants;
import Shared.Core.State.Control;
import Shared.Core.Types.BulletType;
import Shared.Core.Types.WeaponType;

export namespace Soldank::Test
{
std::string CreateTestAnimationData()
{
    std::string animation_data;
    for (int frame = 1; frame <= 30; frame += 1) {
        animation_data += "1\n";
        animation_data += std::to_string(frame) + ".0\n";
        animation_data += "0\n";
        animation_data += std::to_string(frame) + ".0\n";
        animation_data += frame == 30 ? "ENDFILE\n" : "NEXTFRAME\n";
    }
    return animation_data;
}

class AnimationDataReaderStub : public IFileReader
{
public:
    std::expected<std::string, FileReaderError> Read(
      const std::string& file_path,
      std::ios_base::openmode /*mode*/) const override
    {
        if (file_path != "test_animation.poa") {
            return std::unexpected(FileReaderError::FileNotFound);
        }

        return CreateTestAnimationData();
    }
};

AnimationDataManager CreateAnimationDataManager(AnimationType animation_type)
{
    AnimationDataReaderStub animation_data_reader;
    AnimationDataManager animation_data_manager;
    animation_data_manager.LoadAnimationData(
      animation_type, "test_animation.poa", false, 2, animation_data_reader);
    return animation_data_manager;
}

template<typename TAnimationState>
TAnimationState CreateAnimationState(AnimationType animation_type)
{
    auto animation_data_manager = CreateAnimationDataManager(animation_type);
    return TAnimationState{ animation_data_manager };
}

Weapon CreateWeapon(WeaponType weapon_type)
{
    WeaponParameters weapon_parameters{};
    weapon_parameters.kind = weapon_type;
    weapon_parameters.ammo = 50;
    weapon_parameters.bullet_style = BulletType::Bullet;
    weapon_parameters.fire_interval = 1;
    weapon_parameters.reload_time = 1;
    return Weapon{ weapon_parameters };
}

AnimationState::HandleInputParams CreateHandleInputParams()
{
    return AnimationState::HandleInputParams{
        .control = Control{},
        .weapons = { CreateWeapon(WeaponType::DesertEagles), CreateWeapon(WeaponType::USSOCOM) },
        .stance = PhysicsConstants::STANCE_CROUCH,
        .active_weapon = 0,
        .on_ground = true,
        .direction = 1,
        .old_direction = 1,
        .jets_count = 0,
        .legs_animation_type = AnimationType::Stand,
        .grenade_can_throw = true,
    };
}

AnimationState::UpdateParams CreateUpdateParams()
{
    return AnimationState::UpdateParams{
        .control = Control{},
        .direction = 1,
        .on_ground = true,
        .stance = PhysicsConstants::STANCE_STAND,
        .velocity = glm::vec2{},
        .force = glm::vec2{},
        .should_switch_weapon = false,
    };
}

AnimationState::EnterParams CreateEnterParams()
{
    return AnimationState::EnterParams{
        .on_ground = true,
        .direction = 1,
        .force = glm::vec2{},
        .grenade_can_throw = true,
        .weapons = { CreateWeapon(WeaponType::DesertEagles), CreateWeapon(WeaponType::USSOCOM) },
        .active_weapon = 0,
    };
}

AnimationState::ExitParams CreateExitParams()
{
    return AnimationState::ExitParams{ .should_throw_active_weapon = false };
}

void ExpectTransition(const std::optional<AnimationState::Transition>& transition,
                      AnimationType expected_animation_type,
                      std::optional<unsigned int> expected_initial_frame)
{
    ASSERT_TRUE(transition.has_value());
    EXPECT_EQ(transition->animation_type, expected_animation_type);
    EXPECT_EQ(transition->initial_frame, expected_initial_frame);
}

void ExpectTransition(const std::optional<AnimationState::Transition>& transition,
                      AnimationType expected_animation_type)
{
    ExpectTransition(transition, expected_animation_type, std::nullopt);
}

void ExpectNoTransition(const std::optional<AnimationState::Transition>& transition)
{
    EXPECT_FALSE(transition.has_value());
}
} // namespace Soldank::Test
