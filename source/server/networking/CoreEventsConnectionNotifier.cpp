#include <utility>

#include "communication/NetworkMessage.hpp"
#include "networking/CoreEventsConnectionNotifier.hpp"

#include "spdlog/spdlog.h"

namespace Soldank
{
void CoreEventsConnectionNotifier::ObserveAll(IGameServer* game_server,
                                              WorldEvents& world_events,
                                              PhysicsEvents& physics_events)
{
    ObserveAllWorldEvents(game_server, world_events);
    ObserveAllPhysicsEvents(game_server, physics_events);
}

void CoreEventsConnectionNotifier::ObserveAllWorldEvents(IGameServer* game_server,
                                                         WorldEvents& world_events)
{
    world_events.after_soldier_spawns.AddObserver([game_server](const Soldier& soldier) {
        game_server->SendNetworkMessageToAll(NetworkMessage(NetworkEvent::SpawnSoldier,
                                                            soldier.id,
                                                            soldier.particle.position.x,
                                                            soldier.particle.position.y));
    });
    world_events.soldier_died.AddObserver([game_server](Soldier& soldier) {
        game_server->SendNetworkMessageToAll(NetworkMessage(NetworkEvent::KillSoldier, soldier.id));
    });
}

void CoreEventsConnectionNotifier::ObserveAllPhysicsEvents(IGameServer* game_server,
                                                           PhysicsEvents& physics_events)
{
    physics_events.soldier_hit_by_bullet.AddObserver([game_server](Soldier& soldier, float damage) {
        game_server->SendNetworkMessageToAll(
          NetworkMessage(NetworkEvent::HitSoldier, soldier.id, damage));
    });
}
} // namespace Soldank
