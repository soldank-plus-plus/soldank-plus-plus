module;

#include <cstddef>
#include <span>

export module Networking.Transport.TransportTypes;

export import Shared.Networking.DeliveryMode;

export namespace Soldank
{
using ConnectionId = unsigned int;

struct ReceivedPacket
{
    ConnectionId connection_id;
    std::span<const std::byte> payload;
};
} // namespace Soldank
