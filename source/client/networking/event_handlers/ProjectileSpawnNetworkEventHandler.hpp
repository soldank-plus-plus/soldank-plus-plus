#ifndef __PROJECTILE_SPAWN_NETWORK_EVENT_HANDLER_HPP__
#define __PROJECTILE_SPAWN_NETWORK_EVENT_HANDLER_HPP__

#include "communication/NetworkPackets.hpp"
#include "core/IWorld.hpp"

#include "communication/NetworkEventDispatcher.hpp"
#include "rendering/ClientState.hpp"

namespace Soldank
{
class ProjectileSpawnNetworkEventHandler : public NetworkEventHandlerBase<ProjectileSpawnPacket>
{
public:
    ProjectileSpawnNetworkEventHandler(const std::shared_ptr<IWorld>& world,
                                       const std::shared_ptr<ClientState>& client_state);

private:
    NetworkEvent GetTargetNetworkEvent() const override { return NetworkEvent::ProjectileSpawn; }

    NetworkEventHandlerResult HandleNetworkMessageImpl(
      unsigned int sender_connection_id,
      ProjectileSpawnPacket projectile_spawn_packet) override;

    std::shared_ptr<IWorld> world_;
    std::shared_ptr<ClientState> client_state_;
};
} // namespace Soldank

#endif
