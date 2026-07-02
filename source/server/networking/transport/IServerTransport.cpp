module;

#include <cstdint>
#include <functional>
#include <span>
#include <string>

export module Networking.Transport.IServerTransport;

import Networking.Transport.TransportTypes;

export namespace Soldank
{
enum class ConnectionState : std::uint8_t
{
    Connecting,
    Connected,
    Closed
};

struct ConnectionStateChangedEvent
{
    ConnectionId connection_id;
    ConnectionState state;
    std::string description;
};

class IServerTransport
{
public:
    using ConnectionStateChangedHandler =
      std::function<void(const ConnectionStateChangedEvent&)>;

    virtual ~IServerTransport() = default;

    IServerTransport(IServerTransport&& other) = delete;
    IServerTransport& operator=(IServerTransport&& other) = delete;
    IServerTransport(const IServerTransport& other) = delete;
    IServerTransport& operator=(const IServerTransport& other) = delete;

    virtual void Init(std::uint16_t port) = 0;
    virtual void PollConnectionStateChanges() = 0;
    virtual void RegisterObserver(ConnectionStateChangedHandler observer) = 0;
    virtual void Send(ConnectionId connection_id,
                      std::span<const char> payload,
                      DeliveryMode delivery_mode) = 0;
    virtual void Close(ConnectionId connection_id) = 0;

protected:
    IServerTransport() = default;
};
} // namespace Soldank
