module;

#include <cstdint>
#include <string>
#include <memory>

export module Networking.SoldierInfoNetworkEventHandler;

import ClientState;

import Shared.Core.IWorld;

import Shared.Networking.NetworkEventDispatcher;
import Shared.Networking.NetworkPackets;
import Shared.Networking.NetworkEvent;

import Extern.Spdlog;

export namespace Soldank
{
class SoldierInfoNetworkEventHandler : public NetworkEventHandlerBase<std::uint8_t, std::string>
{
public:
    SoldierInfoNetworkEventHandler(const std::shared_ptr<IWorld>& world,
                                   const std::shared_ptr<ClientState>& client_state)
        : world_(world)
        , client_state_(client_state)
    {
    }

private:
    NetworkEvent GetTargetNetworkEvent() const override { return NetworkEvent::SoldierInfo; }

    NetworkEventHandlerResult HandleNetworkMessageImpl(unsigned int /*sender_connection_id*/,
                                                       std::uint8_t soldier_id,
                                                       std::string player_nick) override
    {
        bool is_soldier_id_me = false;
        if (client_state_->client_soldier_id.has_value()) {
            is_soldier_id_me = *client_state_->client_soldier_id == soldier_id;
        }

        if (!is_soldier_id_me) {
            Spdlog::info("({}) {} has joined the server", soldier_id, player_nick);
            world_->CreateSoldier(soldier_id);
            world_->GetStateManager()->TransformSoldier(
              soldier_id, [](auto& soldier) { soldier.active = true; });
        }

        return NetworkEventHandlerResult::Success;
    }

    std::shared_ptr<IWorld> world_;
    std::shared_ptr<ClientState> client_state_;
};
} // namespace Soldank
