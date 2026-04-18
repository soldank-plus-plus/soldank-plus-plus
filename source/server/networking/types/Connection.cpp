module;

#include <string>

export module Networking.Types.Connection;

import Extern.GameNetworkingSockets;

export namespace Soldank
{
struct Connection
{
    GNS::HSteamNetConnection connection_handle;
    std::string nick;
    unsigned int soldier_id;
};
} // namespace Soldank
