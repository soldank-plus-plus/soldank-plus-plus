module;

export module Networking.CoreEventsConnectionNotifier;

import Networking.IGameServer;

import Shared.Networking.NetworkMessage;
import Shared.Networking.NetworkEvent;
import Shared.Networking.NetworkPackets;
import Shared.Core.Physics.Polygons.PolygonContact;
import Shared.Core.Physics.Bullets.BulletEvents;
import Shared.Core.Physics.Bullets.BulletTypes;
import Shared.Core.WorldEvents;
import Shared.Core.Physics.PhysicsEvents;
import Shared.Core.Entities.Soldier;

export namespace Soldank
{
class CoreEventsConnectionNotifier
{
public:
    static void ObserveAll(IGameServer* game_server,
                           WorldEvents& world_events,
                           PhysicsEvents& physics_events)
    {
        ObserveAllWorldEvents(game_server, world_events);
        ObserveAllPhysicsEvents(game_server, physics_events);
    }

private:
    static void ObserveAllWorldEvents(IGameServer* game_server, WorldEvents& world_events)
    {
        world_events.after_soldier_spawns.AddObserver([game_server](const Soldier& soldier) {
            game_server->SendNetworkMessageToAll(NetworkMessage(NetworkEvent::SpawnSoldier,
                                                                soldier.id,
                                                                soldier.particle.position.x,
                                                                soldier.particle.position.y));
        });
        world_events.soldier_died.AddObserver([game_server](Soldier& soldier) {
            game_server->SendNetworkMessageToAll(
              NetworkMessage(NetworkEvent::KillSoldier, soldier.id));
        });
    }

    static void ObserveAllPhysicsEvents(IGameServer* game_server, PhysicsEvents& physics_events)
    {
        physics_events.polygon_event.AddObserver([game_server](const PolygonEffectEvent& event) {
            game_server->SendNetworkMessageToAll(NetworkMessage(
              NetworkEvent::PolygonEffect, PolygonEffectPacket{ event, event.map_fingerprint }));
        });
        physics_events.projectile_event.AddObserver([game_server](
                                                      const ProjectilePhysicsEvent& event) {
            const ProjectilePhysicsPacket packet{
                .kind = event.kind,
                .projectile_id = event.projectile_id,
                .style = event.style,
                .weapon = event.weapon,
                .owner_id = event.owner_id,
                .seed = event.seed,
                .tick = event.tick,
                .position_x = event.position.x,
                .position_y = event.position.y,
                .impulse_x = event.impulse.x,
                .impulse_y = event.impulse.y,
                .radius = event.radius,
                .apply_tick = event.apply_tick,
                .soldier_id = event.soldier_id.value_or(255),
                .item_id = event.item_id.value_or(255),
                .particle_id = event.particle_id.value_or(0),
                .item_style = event.item_style,
                .velocity_before_x = event.velocity_before.x,
                .velocity_before_y = event.velocity_before.y,
                .velocity_after_x = event.velocity_after.x,
                .velocity_after_y = event.velocity_after.y,
                .damage = event.damage,
                .distance = event.distance,
                .normal_x = event.collision && event.collision->surface_normal
                              ? event.collision->surface_normal->x
                              : 0.0F,
                .normal_y = event.collision && event.collision->surface_normal
                              ? event.collision->surface_normal->y
                              : 0.0F,
                .has_collision = event.collision.has_value(),
                .collision_kind =
                  event.collision ? event.collision->kind : BulletCollisionKind::MapPolygon,
                .polygon_id =
                  event.collision ? event.collision->polygon_id.value_or(0xffffffffU) : 0xffffffffU,
                .collider_id = event.collision ? event.collision->collider_id.value_or(0xffffffffU)
                                               : 0xffffffffU,
                .target_projectile_id = event.target_projectile_id.value_or(0),
                .reason = event.reason,
            };
            game_server->SendNetworkMessageToAll(
              NetworkMessage(NetworkEvent::ProjectilePhysics, packet));
        });
        physics_events.soldier_hit_by_bullet.AddObserver(
          [game_server](Soldier& soldier, float damage) {
              game_server->SendNetworkMessageToAll(
                NetworkMessage(NetworkEvent::HitSoldier, soldier.id, damage));
          });
    }
};
} // namespace Soldank
