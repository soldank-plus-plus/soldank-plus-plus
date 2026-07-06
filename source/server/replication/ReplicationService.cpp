module;

#include <cstdint>
#include <memory>
#include <span>

export module Replication.ReplicationService;

import Networking.IGameServer;
import Networking.ServerNetworkHost;
import Sessions.PlayerSessionManager;

import Shared.Core.Entities.Soldier;
import Shared.Core.State.StateManager;
import Shared.Core.Simulation.SimulationEventSink;
import Shared.Core.Simulation.SimulationEvents;
import Shared.Core.Utility.VisitHelper;
import Shared.Networking.NetworkEvent;
import Shared.Networking.NetworkMessage;
import Shared.Networking.NetworkPackets;
import Shared.Networking.ProtocolConversions;

export namespace Soldank
{
class ReplicationService : public SimulationEventSink
{
public:
    ReplicationService(ServerNetworkHost& network_host,
                       const PlayerSessionManager& player_session_manager)
        : network_host_(network_host)
        , player_session_manager_(player_session_manager)
    {
    }

    void BroadcastTick(const StateManager& state_manager)
    {
        state_manager.ForEachSoldier([&](const auto& soldier) {
            SoldierStatePacket packet{
                .game_tick = state_manager.GetGameTick(),
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
                .last_processed_input_id =
                  player_session_manager_.GetLastProcessedInputId(soldier.id),
            };
            network_host_.SendNetworkMessageToAll({ NetworkEvent::SoldierState, packet });
        });
    }

    void OnSimulationEvents(std::span<const SimulationEvent> events) override
    {
        for (const auto& event : events) {
            std::visit(VisitOverload{
                         [&](const ProjectileSpawnedEvent& projectile_spawned_event) {
                             auto packet = ProtocolConversions::ToProjectileSpawnPacket(
                               0, projectile_spawned_event);
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
    ServerNetworkHost& network_host_;
    const PlayerSessionManager& player_session_manager_;
};
} // namespace Soldank
