module;

#include <cstddef>
#include <vector>

export module Networking.Transport.TransportTypes;

export import Shared.Networking.DeliveryMode;

export namespace Soldank
{
using ConnectionId = unsigned int;

struct ReceivedPacket
{
    ConnectionId connection_id;
    std::vector<std::byte> payload;
};
} // namespace Soldank
