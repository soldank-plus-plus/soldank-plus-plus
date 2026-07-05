module;

#include <string>
#include <memory>
#include <cstdint>

export module Networking.NetworkingClient;

import Networking.INetworkingClient;
import Networking.Transport.ClientTransportAdapter;

import Shared.Networking.NetworkEventDispatcher;
import Shared.Networking.NetworkMessage;
import Shared.Networking.DeliveryMode;

import Extern.Spdlog;

export namespace Soldank
{
class NetworkingClient : public INetworkingClient
{
public:
    NetworkingClient(const char* server_ip, std::uint16_t server_port)
        : transport_(std::make_unique<ClientTransportAdapter>(server_ip, server_port))
    {
    }

    void Update(const std::shared_ptr<NetworkEventDispatcher>& network_event_dispatcher) final
    {
        if (!has_logged_first_update_) {
            Spdlog::info("NetworkingClient::Update polling incoming messages");
            has_logged_first_update_ = true;
        }
        transport_->Update(network_event_dispatcher);
    }

    void SendNetworkMessage(
      const NetworkMessage& network_message,
      DeliveryMode delivery_mode = DeliveryMode::Unreliable) final
    {
        transport_->SendNetworkMessage(network_message, delivery_mode);
    }

    void SetLag(int lag_to_add_milliseconds) final
    {
        transport_->SetLag(lag_to_add_milliseconds);
    }

private:
    std::unique_ptr<ClientTransportAdapter> transport_;
    bool has_logged_first_update_ = false;
};

} // namespace Soldank
