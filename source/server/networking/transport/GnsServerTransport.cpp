module;

#include <span>
#include <string>

export module Networking.Transport.GnsServerTransport;

import Networking.Transport.TransportTypes;

import Shared.Networking.NetworkMessage;

import Extern.GameNetworkingSockets;

export namespace Soldank
{
class GnsServerTransport
{
public:
    static ConnectionId ToConnectionId(GNS::HSteamNetConnection connection_handle)
    {
        return static_cast<ConnectionId>(connection_handle);
    }

    static GNS::HSteamNetConnection ToConnectionHandle(ConnectionId connection_id)
    {
        return static_cast<GNS::HSteamNetConnection>(connection_id);
    }

    static int ToSendFlag(DeliveryMode delivery_mode)
    {
        return delivery_mode == DeliveryMode::Reliable
                 ? GNS::nSteamNetworkingSend::Reliable
                 : GNS::nSteamNetworkingSend::Unreliable;
    }

    static void SendNetworkMessage(GNS::ISteamNetworkingSockets* interface,
                                   ConnectionId connection_id,
                                   const NetworkMessage& network_message,
                                   DeliveryMode delivery_mode)
    {
        interface->SendMessageToConnection(ToConnectionHandle(connection_id),
                                           network_message.GetData().data(),
                                           network_message.GetData().size(),
                                           ToSendFlag(delivery_mode),
                                           nullptr);
    }

    static void SendString(GNS::ISteamNetworkingSockets* interface,
                           ConnectionId connection_id,
                           const std::string& message,
                           DeliveryMode delivery_mode)
    {
        interface->SendMessageToConnection(ToConnectionHandle(connection_id),
                                           message.c_str(),
                                           message.size(),
                                           ToSendFlag(delivery_mode),
                                           nullptr);
    }

    static bool SetConnectionPollGroup(GNS::ISteamNetworkingSockets* interface,
                                       ConnectionId connection_id,
                                       GNS::HSteamNetPollGroup poll_group_handle)
    {
        return interface->SetConnectionPollGroup(ToConnectionHandle(connection_id),
                                                 poll_group_handle);
    }

    static void CloseConnection(GNS::ISteamNetworkingSockets* interface,
                                ConnectionId connection_id)
    {
        interface->CloseConnection(ToConnectionHandle(connection_id), 0, nullptr, false);
    }

    static void SetConnectionName(GNS::ISteamNetworkingSockets* interface,
                                  ConnectionId connection_id,
                                  const std::string& nick)
    {
        interface->SetConnectionName(ToConnectionHandle(connection_id), nick.c_str());
    }
};
} // namespace Soldank
