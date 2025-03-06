#ifndef __STATE_HPP__
#define __STATE_HPP__

#include "core/map/Map.hpp"
#include "core/entities/Bullet.hpp"
#include "core/entities/Soldier.hpp"
#include "core/entities/Item.hpp"

#include <vector>
#include <array>

namespace Soldank
{
const std::size_t MAX_BULLETS_COUNT = 256;
const std::size_t MAX_SOLDIERS_COUNT = 32;
const std::size_t MAX_ITEMS_COUNT = 128;

struct State
{
    State(AnimationDataManager& animation_data_manager, std::shared_ptr<ParticleSystem> skeleton);

    unsigned int game_tick{};
    bool paused{};
    Map map;
    std::array<Bullet, MAX_BULLETS_COUNT> bullets;
    std::array<Soldier, MAX_SOLDIERS_COUNT> soldiers;
    std::array<Item, MAX_ITEMS_COUNT> items;
};
} // namespace Soldank

#endif
