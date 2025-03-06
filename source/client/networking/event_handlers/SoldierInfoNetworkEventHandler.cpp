#include "networking/event_handlers/SoldierInfoNetworkEventHandler.hpp"

#include "communication/NetworkPackets.hpp"
#include "spdlog/spdlog.h"

namespace Soldank
{
SoldierInfoNetworkEventHandler::SoldierInfoNetworkEventHandler(
  const std::shared_ptr<IWorld>& world,
  const std::shared_ptr<ClientState>& client_state)
    : world_(world)
    , client_state_(client_state)
{
}

NetworkEventHandlerResult SoldierInfoNetworkEventHandler::HandleNetworkMessageImpl(
  unsigned int /*sender_connection_id*/,
  std::uint8_t soldier_id,
  std::string player_nick)
{
    bool is_soldier_id_me = false;
    if (client_state_->client_soldier_id.has_value()) {
        is_soldier_id_me = *client_state_->client_soldier_id == soldier_id;
    }

    if (!is_soldier_id_me) {
        spdlog::info("({}) {} has joined the server", soldier_id, player_nick);
        world_->CreateSoldier(soldier_id);
        world_->GetStateManager()->TransformSoldier(soldier_id,
                                                    [](auto& soldier) { soldier.active = true; });
    }

    return NetworkEventHandlerResult::Success;
}
} // namespace Soldank
