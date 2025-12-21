module;

#include "communication/NetworkEventDispatcher.hpp"

#include "core/IWorld.hpp"

#include "spdlog/spdlog.h"

export module AssignPlayerIdNetworkEventHandler;

import ClientState;

export namespace Soldank
{
class AssignPlayerIdNetworkEventHandler : public NetworkEventHandlerBase<std::uint8_t>
{
public:
    AssignPlayerIdNetworkEventHandler(const std::shared_ptr<IWorld>& world,
                                      const std::shared_ptr<ClientState>& client_state)
        : world_(world)
        , client_state_(client_state)
    {
    }

private:
    NetworkEvent GetTargetNetworkEvent() const override { return NetworkEvent::AssignPlayerId; }

    NetworkEventHandlerResult HandleNetworkMessageImpl(unsigned int /*sender_connection_id*/,
                                                       std::uint8_t assigned_player_id) override
    {
        const auto& soldier = world_->CreateSoldier(assigned_player_id);
        client_state_->client_soldier_id = soldier.id;
        spdlog::info(
          "OnAssignPlayerId: {} {}", assigned_player_id, *client_state_->client_soldier_id);
        return NetworkEventHandlerResult::Success;
    }

    std::shared_ptr<IWorld> world_;
    std::shared_ptr<ClientState> client_state_;
};
} // namespace Soldank
