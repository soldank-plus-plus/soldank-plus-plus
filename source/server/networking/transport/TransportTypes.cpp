module;

#include <cstddef>
#include <cstdint>
#include <span>

export module Networking.Transport.TransportTypes;

export namespace Soldank
{
using ConnectionId = unsigned int;

enum class DeliveryMode : std::uint8_t
{
    Unreliable,
    Reliable
};

struct ReceivedPacket
{
    ConnectionId connection_id;
    std::span<const std::byte> payload;
};
} // namespace Soldank
