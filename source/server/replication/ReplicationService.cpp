module;

#include <cstdint>
#include <memory>
#include <span>
#include <array>
#include <algorithm>
#include <optional>

export module Replication.ReplicationService;

import Runtime.ServerRuntimeServices;
import Sessions.PlayerSessionManager;

import Shared.Core.Entities.Soldier;
import Shared.Core.Entities.WeaponParametersFactory;
import Shared.Core.State.StateManager;
import Shared.Core.Simulation.SimulationEventSink;
import Shared.Core.Simulation.SimulationEvents;
import Shared.Core.Utility.VisitHelper;
import Shared.Networking.NetworkEvent;
import Shared.Networking.NetworkMessage;
import Shared.Networking.NetworkPackets;
import Shared.Networking.GameModeProtocol;
import Shared.Core.GameModes.GameModeRuntime;
import Shared.Networking.ProtocolConversions;

export namespace Soldank
{
class ReplicationService : public SimulationEventSink
{
public:
    ReplicationService(IServerNetworkHost& network_host,
                       const PlayerSessionManager& player_session_manager)
        : network_host_(network_host)
        , player_session_manager_(player_session_manager)
    {
    }

    void BroadcastTick(const StateManager& state_manager)
    {
        const auto* mode = state_manager.GetGameMode();
        if (mode != nullptr &&
            (!last_match_epoch_ || *last_match_epoch_ != mode->GetState().epoch ||
             last_match_revision_ != mode->GetState().revision ||
             state_manager.GetGameTick() % 60U == 0)) {
            network_host_.SendNetworkMessageToAll(
              { NetworkEvent::MatchState, MakeMatchStatePacket(state_manager) });
            last_match_epoch_ = mode->GetState().epoch;
            last_match_revision_ = mode->GetState().revision;
        }
        state_manager.ForEachSoldier([&](const auto& soldier) {
            SoldierStatePacket packet{
                .server_tick = state_manager.GetGameTick(),
                .player_id = soldier.id,
                .position_x = soldier.particle.position.x,
                .position_y = soldier.particle.position.y,
                .old_position_x = soldier.particle.old_position.x,
                .old_position_y = soldier.particle.old_position.y,
                .body_animation_type = soldier.body_animation->GetType(),
                .body_animation_frame = soldier.body_animation->GetFrame(),
                .body_animation_speed = soldier.body_animation->GetSpeed(),
                .body_animation_count = soldier.body_animation->GetCount(),
                .legs_animation_type = soldier.legs_animation->GetType(),
                .legs_animation_frame = soldier.legs_animation->GetFrame(),
                .legs_animation_speed = soldier.legs_animation->GetSpeed(),
                .legs_animation_count = soldier.legs_animation->GetCount(),
                .velocity_x = soldier.particle.GetVelocity().x,
                .velocity_y = soldier.particle.GetVelocity().y,
                .force_x = soldier.particle.GetForce().x,
                .force_y = soldier.particle.GetForce().y,
                .on_ground = soldier.on_ground,
                .on_ground_for_law = soldier.on_ground_for_law,
                .on_ground_last_frame = soldier.on_ground_last_frame,
                .on_ground_permanent = soldier.on_ground_permanent,
                .old_direction = soldier.old_direction,
                .stance = soldier.stance,
                .mouse_map_position_x = static_cast<float>(soldier.control.mouse_aim_x),
                .mouse_map_position_y = static_cast<float>(soldier.control.mouse_aim_y),
                .using_jets = soldier.control.jets,
                .jets_count = soldier.jets_count,
                .active_weapon = soldier.active_weapon,
                .last_applied_input_id = player_session_manager_.GetLastAppliedInputId(soldier.id),
            };
            packet.weapon_count = static_cast<std::uint8_t>(soldier.weapons.size());
            packet.weapon_generation = soldier.weapon_generation;
            packet.weapon_config_hash =
              WeaponParametersFactory::GetConfigurationHash(state_manager.IsRealistic());
            for (std::size_t slot = 0; slot < packet.weapons.size(); ++slot) {
                packet.weapons.at(slot) = soldier.weapons.at(slot).GetState();
            }
            packet.grenade_can_throw = soldier.grenade_can_throw;
            packet.hit_spray = soldier.hit_spray;
            packet.mounted_weapon = soldier.mounted_weapon;
            packet.combat_bonus = soldier.combat_bonus;
            packet.bonus_ticks = soldier.bonus_ticks;
            packet.fired = soldier.fired;
            packet.control = soldier.control;
            packet.has_polygon_state = true;
            packet.polygon_background = soldier.polygon_background;
            packet.carrying_flag = soldier.is_holding_flags;
            packet.appearance = soldier.appearance;
            packet.wear_helmet = soldier.wear_helmet;
            packet.has_cigar = soldier.has_cigar;
            packet.on_fire = soldier.on_fire;
            packet.visual_alpha = soldier.alpha;
            packet.visual_visible = soldier.visible;
            packet.life_sequence = soldier.life_sequence;
            packet.death_severity = soldier.death_severity;
            packet.health = soldier.health;
            packet.vest = soldier.vest;
            packet.dead_meat = soldier.dead_meat;
            packet.ticks_to_respawn = soldier.ticks_to_respawn;
            network_host_.SendNetworkMessageToAll({ NetworkEvent::SoldierState, packet });
        });
        std::array<ItemStatePacket, 128> items{};
        for (std::size_t id = 0; id < items.size(); ++id) {
            items.at(id).id = static_cast<std::uint8_t>(id);
            items.at(id).server_tick = state_manager.GetGameTick();
        }
        state_manager.ForEachItem([&](const auto& item) {
            auto& packet = items.at(item.id);
            packet.active = item.active;
            packet.style = item.style;
            packet.owner = item.owner;
            packet.ammo = item.ammo_count;
            packet.radius = item.radius;
            packet.timeout = item.time_out;
            packet.is_static = item.static_type;
            packet.heat = item.interest;
            packet.mounted_soldier = item.mounted_soldier;
            packet.holding_soldier_id = item.holding_soldier_id;
            packet.polygon_background = item.polygon_background;
            packet.particle_count = static_cast<std::uint8_t>(
              std::min<std::size_t>(4, item.skeleton->GetParticles().size()));
            for (unsigned int i = 0; i < packet.particle_count; ++i) {
                const auto pos = item.skeleton->GetPos(i + 1);
                const auto old = item.skeleton->GetOldPos(i + 1);
                packet.particles.at(i) = { pos.x, pos.y, old.x, old.y };
            }
        });
        for (std::size_t id = 0; id < items.size(); ++id) {
            auto& previous = last_items_.at(id);
            if (!items.at(id).active && !previous.active) {
                continue;
            }
            previous.server_tick = items.at(id).server_tick;
            if (items.at(id) != previous || state_manager.GetGameTick() % 60U == 0) {
                network_host_.SendNetworkMessageToAll({ NetworkEvent::ItemState, items.at(id) });
                previous = items.at(id);
            }
        }
    }

    void OnSimulationEvents(std::span<const SimulationEvent> events) override
    {
        for (const auto& event : events) {
            std::visit(VisitOverload{
                         [&](const ProjectileSpawnedEvent& projectile_spawned_event) {
                             auto packet = ProtocolConversions::ToProjectileSpawnPacket(
                               projectile_spawned_event.bullet_params.id, projectile_spawned_event);
                             network_host_.SendNetworkMessageToAll(
                               { NetworkEvent::ProjectileSpawn, packet });
                         },
                         [](const SoldierSpawnedEvent& /*event*/) {},
                         [](const SoldierKilledEvent& /*event*/) {},
                         [](const ItemPickedUpEvent& /*event*/) {},
                       },
                       event);
        }
    }

private:
    std::optional<std::uint32_t> last_match_epoch_;
    std::uint32_t last_match_revision_{};
    std::array<ItemStatePacket, 128> last_items_{};
    IServerNetworkHost& network_host_;
    const PlayerSessionManager& player_session_manager_;
};
} // namespace Soldank
