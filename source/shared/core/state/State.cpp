module;

#include <memory>
#include <utility>
#include <vector>
#include <array>

export module Shared.Core.State.State;

import Shared.Core.Physics.Particles;
import Shared.Core.Animations;
import Shared.Core.Entities.WeaponParametersFactory;
import Shared.Core.Map.Map;
import Shared.Core.Entities.Bullet;
import Shared.Core.Entities.Soldier;
import Shared.Core.Entities.Item;
import Shared.Core.Entities.Weapon;
import Shared.Core.Types.WeaponType;

export namespace Soldank
{
const std::size_t MAX_BULLETS_COUNT = 256;
const std::size_t MAX_SOLDIERS_COUNT = 32;
const std::size_t MAX_ITEMS_COUNT = 128;

struct State
{
    State(AnimationDataManager& animation_data_manager, std::shared_ptr<ParticleSystem> skeleton)
    {
        soldiers.fill(
          { 0,
            animation_data_manager,
            std::move(skeleton),
            std::vector<Weapon>{
              { WeaponParametersFactory::GetParameters(WeaponType::DesertEagles, false) },
              { WeaponParametersFactory::GetParameters(WeaponType::Knife, false) },
              { WeaponParametersFactory::GetParameters(WeaponType::FragGrenade, false) } } });
    }

    unsigned int game_tick{};
    bool paused{};
    Map map;
    std::array<Bullet, MAX_BULLETS_COUNT> bullets;
    std::array<Soldier, MAX_SOLDIERS_COUNT> soldiers;
    std::array<Item, MAX_ITEMS_COUNT> items;
};
} // namespace Soldank
