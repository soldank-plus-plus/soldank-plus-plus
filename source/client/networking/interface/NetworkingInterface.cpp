module;

#include <vector>
#include <functional>
#include <array>
#include <memory>
#include <cstdint>

export module Networking.NetworkingInterface;

import Networking.Connection;
import Networking.IConnection;
import Extern.GameNetworkingSockets;
import Extern.Spdlog;

namespace Soldank::NetworkingInterface
{
namespace
{
GNS::ISteamNetworkingSockets* interface;
std::vector<std::function<void(GNS::SteamNetConnectionStatusChangedCallback_t*)>> observers;

void SteamNetConnectionStatusChangedCallback(GNS::SteamNetConnectionStatusChangedCallback_t* p_info)
{
    for (const auto& observer : observers) {
        observer(p_info);
    }
}
} // namespace

export void Init()
{
    interface = GNS::GameNetworkingSockets();
}

export std::shared_ptr<IConnection> CreateConnection(const char* server_ip,
                                                     std::uint16_t server_port)
{
    GNS::SteamNetworkingIPAddr server_addr{};
    server_addr.Clear();
    server_addr.ParseString(server_ip); // TODO: ip=localhost doesn't work
    server_addr.m_port = server_port;
    Spdlog::info("Connecting to server");

    GNS::SteamNetworkingConfigValue_t opt{};
    opt.SetPtr(GNS::ESteamNetworkingConfig::Callback_ConnectionStatusChanged,
               (void*)SteamNetConnectionStatusChangedCallback);
    GNS::HSteamNetConnection connection_handle =
      interface->ConnectByIPAddress(server_addr, 1, &opt);
    if (connection_handle == GNS::HSteamNetConnection_Enum::Invalid) {
        Spdlog::error("Failed to create connection");
    }

    return std::make_shared<Connection>(interface, connection_handle);
}

export void PollConnectionStateChanges()
{
    interface->RunCallbacks();
}

export void RegisterObserver(
  const std::function<void(GNS::SteamNetConnectionStatusChangedCallback_t*)>& observer)
{
    observers.push_back(observer);
}

export void Free()
{
    observers.clear();
}
} // namespace Soldank::NetworkingInterface
