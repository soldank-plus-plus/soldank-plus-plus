module;

#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

export module Networking.Transport.WebRtcServerTransport;

import Networking.Transport.IServerTransport;
import Networking.Transport.TransportTypes;

import Extern.Spdlog;

export namespace Soldank
{
class WebRtcServerTransport final : public IServerTransport
{
public:
    void Init(std::uint16_t port) override
    {
        port_ = port;
        Spdlog::info("[WebRtcServerTransport] Stub initialized on port {}", port_);
    }

    void PollConnectionStateChanges() override {}

    void RegisterObserver(ConnectionStateChangedHandler observer) override
    {
        observers_.push_back(std::move(observer));
        Spdlog::info("[WebRtcServerTransport] Registered connection observer");
    }

    void Send(ConnectionId connection_id,
              std::span<const char> payload,
              DeliveryMode delivery_mode) override
    {
        Spdlog::info("[WebRtcServerTransport] Stub send skipped: connection {}, bytes {}, mode {}",
                     connection_id,
                     payload.size(),
                     delivery_mode == DeliveryMode::Reliable ? "reliable" : "unreliable");
    }

    void Close(ConnectionId connection_id) override
    {
        Spdlog::info("[WebRtcServerTransport] Stub close skipped: connection {}", connection_id);
    }

private:
    std::uint16_t port_ = 0;
    std::vector<ConnectionStateChangedHandler> observers_;
};
} // namespace Soldank
