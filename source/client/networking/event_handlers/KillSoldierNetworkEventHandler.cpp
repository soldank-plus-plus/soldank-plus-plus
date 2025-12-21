module;

#include "communication/NetworkEventDispatcher.hpp"

#include "core/IWorld.hpp"

#include "spdlog/spdlog.h"

#include <chrono>

export module KillSoldierNetworkEventHandler;

export namespace Soldank
{
class KillSoldierNetworkEventHandler : public NetworkEventHandlerBase<std::uint8_t>
{
public:
    KillSoldierNetworkEventHandler(const std::shared_ptr<IWorld>& world)
        : world_(world)
    {
    }

private:
    NetworkEvent GetTargetNetworkEvent() const override { return NetworkEvent::KillSoldier; }

    NetworkEventHandlerResult HandleNetworkMessageImpl(
      // TODO: remove from client: sender_connection_id
      unsigned int /*sender_connection_id*/,
      std::uint8_t soldier_id) override
    {
        world_->KillSoldier(soldier_id);
        return NetworkEventHandlerResult::Success;
    }

    std::shared_ptr<IWorld> world_;
};
} // namespace Soldank
