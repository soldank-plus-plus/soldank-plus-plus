module;

#include "communication/NetworkEventDispatcher.hpp"

#include "spdlog/spdlog.h"

#include <chrono>

export module PingCheckNetworkEventHandler;

import ClientState;

export namespace Soldank
{
class PingCheckNetworkEventHandler : public NetworkEventHandlerBase<>
{
public:
    PingCheckNetworkEventHandler(const std::shared_ptr<ClientState>& client_state)
        : client_state_(client_state)
    {
    }

private:
    NetworkEvent GetTargetNetworkEvent() const override { return NetworkEvent::PingCheck; }

    NetworkEventHandlerResult HandleNetworkMessageImpl(
      unsigned int /*sender_connection_id*/) override
    {
        if (client_state_->ping_timer.IsRunning()) {
            client_state_->ping_timer.Stop();
        }

        return NetworkEventHandlerResult::Success;
    }

    std::shared_ptr<ClientState> client_state_;
};
} // namespace Soldank
