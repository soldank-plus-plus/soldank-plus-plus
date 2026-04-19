module;

#include "communication/NetworkEventDispatcher.hpp"

#include "core/IWorld.hpp"

#include <chrono>

export module Networking.PlayerLeaveNetworkEventHandler;

export namespace Soldank
{
class PlayerLeaveNetworkEventHandler : public NetworkEventHandlerBase<std::uint8_t>
{
public:
    PlayerLeaveNetworkEventHandler(const std::shared_ptr<IWorld>& world)
        : world_(world)
    {
    }

private:
    NetworkEvent GetTargetNetworkEvent() const override { return NetworkEvent::PlayerLeave; }

    NetworkEventHandlerResult HandleNetworkMessageImpl(unsigned int /*sender_connection_id*/,
                                                       std::uint8_t soldier_id) override
    {
        world_->GetStateManager()->RemoveSoldier(soldier_id);

        return NetworkEventHandlerResult::Success;
    }

    std::shared_ptr<IWorld> world_;
};
} // namespace Soldank
