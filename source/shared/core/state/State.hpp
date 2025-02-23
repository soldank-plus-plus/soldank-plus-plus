#ifndef __STATE_HPP__
#define __STATE_HPP__

#include "core/map/Map.hpp"
#include "core/entities/Bullet.hpp"
#include "core/entities/Soldier.hpp"
#include "core/entities/Item.hpp"

#include <vector>
#include <list>

namespace Soldank
{
struct State
{
    unsigned int game_tick;
    bool paused;
    Map map;
    std::list<Bullet> bullets;
    std::list<Soldier> soldiers;
    std::vector<Item> items;
};
} // namespace Soldank

#endif
