module;

#include <steam/steamnetworkingsockets.h>

#include <string>

export module Networking.Types.Connection;

export namespace Soldank
{
struct Connection
{
    HSteamNetConnection connection_handle;
    std::string nick;
    unsigned int soldier_id;
};
} // namespace Soldank
