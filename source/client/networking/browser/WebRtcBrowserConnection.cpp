module;

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>

export module Networking.Browser.WebRtcBrowserConnection;

import Networking.IConnection;

import Shared.Networking.NetworkEventDispatcher;
import Shared.Networking.NetworkMessage;
import Shared.Networking.DeliveryMode;

import Extern.Spdlog;

extern "C"
{
void soldank_webrtc_connect(const char* server_ip, int server_port);
void soldank_webrtc_close();
int soldank_webrtc_poll_message(std::uint8_t* buffer, int buffer_size);
void soldank_webrtc_send_unreliable(const std::uint8_t* data, int data_size);
void soldank_webrtc_send_reliable(const std::uint8_t* data, int data_size);
}

export namespace Soldank
{
class WebRtcBrowserConnection : public IConnection
{
public:
    WebRtcBrowserConnection(const char* server_ip, std::uint16_t server_port)
    {
        Spdlog::info("[WebRtcBrowserConnection] Connecting to {}:{}", server_ip, server_port);
        soldank_webrtc_connect(server_ip, static_cast<int>(server_port));

        const std::string nick = "test name";
        soldank_webrtc_send_reliable(reinterpret_cast<const std::uint8_t*>(nick.data()),
                                     static_cast<int>(nick.size()));
        Spdlog::info("[WebRtcBrowserConnection] Queued nick: {}", nick);
    }

    void PollIncomingMessages(
      const std::shared_ptr<NetworkEventDispatcher>& network_event_dispatcher) final
    {
        while (true) {
            const int message_size =
              soldank_webrtc_poll_message(receive_buffer_.data(),
                                          static_cast<int>(receive_buffer_.size()));
            if (message_size <= 0) {
                return;
            }

            const auto clamped_message_size =
              std::min(static_cast<std::size_t>(message_size), receive_buffer_.size());
            std::span<const char> received_bytes{
                reinterpret_cast<const char*>(receive_buffer_.data()), clamped_message_size
            };
            NetworkMessage received_message(received_bytes);
            ConnectionMetadata connection_metadata{
                .connection_id = 0,
                .send_message_to_connection = [](const NetworkMessage& /*message*/) {}
            };
            auto dispatch_result =
              network_event_dispatcher->ProcessNetworkMessage(connection_metadata,
                                                              received_message);
            if (dispatch_result.first != NetworkEventDispatchResult::Success) {
                Spdlog::warn("[WebRtcBrowserConnection] Packet dispatch failed: result={}",
                             static_cast<int>(dispatch_result.first));
            }
        }
    }

    void CloseConnection() final { soldank_webrtc_close(); }

    void SendNetworkMessage(const NetworkMessage& network_message,
                            DeliveryMode delivery_mode = DeliveryMode::Unreliable) final
    {
        const auto* data =
          reinterpret_cast<const std::uint8_t*>(network_message.GetData().data());
        const auto data_size = static_cast<int>(network_message.GetData().size());
        if (delivery_mode == DeliveryMode::Reliable) {
            soldank_webrtc_send_reliable(data, data_size);
        } else {
            soldank_webrtc_send_unreliable(data, data_size);
        }
    }

private:
    std::array<std::uint8_t, 64 * 1024> receive_buffer_{};
};
} // namespace Soldank
