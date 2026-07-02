module;

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <rtc/rtc.hpp>

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
        rtc::Configuration config;
        peer_connection_ = std::make_unique<rtc::PeerConnection>(config);
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
    std::unique_ptr<rtc::PeerConnection> peer_connection_;
    std::vector<ConnectionStateChangedHandler> observers_;
};
} // namespace Soldank
