module;

#include <cstdint>
#include <optional>
#include <variant>

export module Shared.Core.Simulation.SimulationCommands;

import Extern.Glm;

import Shared.Core.State.Control;
import Shared.Core.Types.WeaponType;

export namespace Soldank
{
struct PlayerInputCommand
{
    std::uint8_t soldier_id;
    std::uint32_t input_sequence_id;
    std::uint32_t client_tick;
    std::uint32_t apply_server_tick;
    Control control;
    glm::vec2 mouse_map_position;
};

struct SpawnSoldierCommand
{
    std::uint8_t soldier_id;
    std::optional<glm::vec2> spawn_position = std::nullopt;
};

struct KillSoldierCommand
{
    std::uint8_t soldier_id;
};

struct RemoveSoldierCommand
{
    std::uint8_t soldier_id;
};

struct SetWeaponChoiceCommand
{
    std::uint8_t soldier_id;
    WeaponType primary_weapon_type;
    WeaponType secondary_weapon_type;
};

using SimulationCommand = std::
  variant<SpawnSoldierCommand, KillSoldierCommand, RemoveSoldierCommand, SetWeaponChoiceCommand>;
} // namespace Soldank
