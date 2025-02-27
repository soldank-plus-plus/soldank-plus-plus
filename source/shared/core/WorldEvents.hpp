#ifndef __WORLD_EVENTS_HPP__
#define __WORLD_EVENTS_HPP__

#include "core/utility/Observable.hpp"
#include "core/entities/Soldier.hpp"

namespace Soldank
{
struct WorldEvents
{
    Observable<const Soldier&> after_soldier_spawns;
    Observable<Soldier&> soldier_died;
};
} // namespace Soldank

#endif
