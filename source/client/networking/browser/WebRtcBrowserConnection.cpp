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
        soldank_webrtc_connect(server_ip, static_cast<int>(server_port));

        const std::string nick = "test name";
        soldank_webrtc_send_reliable(reinterpret_cast<const std::uint8_t*>(nick.data()),
                                     static_cast<int>(nick.size()));
    }

    void PollIncomingMessages(
      const std::shared_ptr<NetworkEventDispatcher>& network_event_dispatcher) final
    {
        std::array<std::uint8_t, 64 * 1024> buffer{};
        while (true) {
            const int message_size =
              soldank_webrtc_poll_message(buffer.data(), static_cast<int>(buffer.size()));
            if (message_size <= 0) {
                return;
            }

            const auto clamped_message_size =
              std::min(static_cast<std::size_t>(message_size), buffer.size());
            std::span<const char> received_bytes{
                reinterpret_cast<const char*>(buffer.data()), clamped_message_size
            };
            NetworkMessage received_message(received_bytes);
            ConnectionMetadata connection_metadata{
                .connection_id = 0,
                .send_message_to_connection = [](const NetworkMessage& /*message*/) {}
            };
            network_event_dispatcher->ProcessNetworkMessage(connection_metadata, received_message);
        }
    }

    void CloseConnection() final { soldank_webrtc_close(); }

    void SendNetworkMessage(const NetworkMessage& network_message) final
    {
        soldank_webrtc_send_unreliable(
          reinterpret_cast<const std::uint8_t*>(network_message.GetData().data()),
          static_cast<int>(network_message.GetData().size()));
    }
};
} // namespace Soldank
