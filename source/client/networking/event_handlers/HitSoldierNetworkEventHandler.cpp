module;

#include "communication/NetworkEventDispatcher.hpp"

#include "core/IWorld.hpp"

#include "spdlog/spdlog.h"

#include <chrono>

export module HitSoldierNetworkEventHandler;

export namespace Soldank
{
class HitSoldierNetworkEventHandler : public NetworkEventHandlerBase<std::uint8_t, float>
{
public:
    HitSoldierNetworkEventHandler(const std::shared_ptr<IWorld>& world)
        : world_(world)
    {
    }

private:
    NetworkEvent GetTargetNetworkEvent() const override { return NetworkEvent::HitSoldier; }

    NetworkEventHandlerResult HandleNetworkMessageImpl(
      // TODO: remove from client: sender_connection_id
      unsigned int /*sender_connection_id*/,
      std::uint8_t soldier_id,
      float damage) override
    {
        world_->HitSoldier(soldier_id, damage);
        return NetworkEventHandlerResult::Success;
    }

    std::shared_ptr<IWorld> world_;
};
} // namespace Soldank
