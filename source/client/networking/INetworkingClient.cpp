module;

#include <memory>

export module Networking.INetworkingClient;

import Shared.Networking.NetworkEventDispatcher;
import Shared.Networking.NetworkMessage;
import Shared.Networking.DeliveryMode;

export namespace Soldank
{
class INetworkingClient
{
public:
    virtual ~INetworkingClient() = default;

    virtual void Update(
      const std::shared_ptr<NetworkEventDispatcher>& network_event_dispatcher) = 0;

    virtual void SendNetworkMessage(const NetworkMessage& network_message,
                                    DeliveryMode delivery_mode = DeliveryMode::Unreliable) = 0;

    virtual void SetLag(int lag_to_add_milliseconds) = 0;
};
} // namespace Soldank
