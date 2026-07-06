module;

#include "core/math/Glm.hpp"

#include <cstdint>
#include <variant>

export module Shared.Core.Simulation.SimulationEvents;

import Shared.Core.Entities.Bullet;
import Shared.Core.Types.ItemType;

export namespace Soldank
{
struct SoldierSpawnedEvent
{
    std::uint8_t soldier_id;
    glm::vec2 position;
};

struct SoldierKilledEvent
{
    std::uint8_t soldier_id;
};

struct ProjectileSpawnedEvent
{
    BulletParams bullet_params;
};

struct ItemPickedUpEvent
{
    std::uint8_t soldier_id;
    std::uint8_t item_id;
    ItemType item_type;
};

using SimulationEvent =
  std::variant<SoldierSpawnedEvent, SoldierKilledEvent, ProjectileSpawnedEvent, ItemPickedUpEvent>;
} // namespace Soldank
