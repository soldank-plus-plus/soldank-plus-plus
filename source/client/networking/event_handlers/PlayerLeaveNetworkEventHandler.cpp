#include "networking/event_handlers/PlayerLeaveNetworkEventHandler.hpp"

#include "spdlog/spdlog.h"

#include <chrono>

namespace Soldank
{
PlayerLeaveNetworkEventHandler::PlayerLeaveNetworkEventHandler(const std::shared_ptr<IWorld>& world)
    : world_(world)
{
}

NetworkEventHandlerResult PlayerLeaveNetworkEventHandler::HandleNetworkMessageImpl(
  unsigned int /*sender_connection_id*/,
  std::uint8_t soldier_id)
{
    world_->GetStateManager()->RemoveSoldier(soldier_id);

    return NetworkEventHandlerResult::Success;
}
} // namespace Soldank
