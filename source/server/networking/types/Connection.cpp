module;

#include <string>

export module Networking.Types.Connection;

import Networking.Transport.TransportTypes;

export namespace Soldank
{
struct Connection
{
    ConnectionId connection_id;
    std::string nick;
    unsigned int soldier_id = 0;
};
} // namespace Soldank
