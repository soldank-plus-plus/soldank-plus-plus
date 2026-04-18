module;

#include <functional>
#include <memory>
#include <cstdint>

export module Networking.Interface.NetworkingInterface;

import Networking.PollGroups.IPollGroup;

import Extern.Spdlog;
import Extern.GameNetworkingSockets;

namespace Soldank::NetworkingInterface
{
GNS::ISteamNetworkingSockets* interface;
GNS::HSteamListenSocket listen_socket_handle;
std::vector<std::function<void(GNS::SteamNetConnectionStatusChangedCallback_t*)>> observers;

GNS::ISteamNetworkingSockets* GetInterface()
{
    return interface;
}

void SteamNetConnectionStatusChangedCallback(GNS::SteamNetConnectionStatusChangedCallback_t* p_info)
{
    for (const auto& observer : observers) {
        observer(p_info);
    }
}

export void Init(std::uint16_t port)
{
    interface = GNS::GameNetworkingSockets();
    GNS::SteamNetworkingIPAddr server_local_addr{};
    server_local_addr.Clear();
    server_local_addr.m_port = port;
    GNS::SteamNetworkingConfigValue_t opt{};
    opt.SetPtr(GNS::ESteamNetworkingConfig::Callback_ConnectionStatusChanged,
               (void*)SteamNetConnectionStatusChangedCallback);
    listen_socket_handle = interface->CreateListenSocketIP(server_local_addr, 1, &opt);
    if (listen_socket_handle == GNS::HSteamListenSocket_Enum::Invalid) {
        Spdlog::error("Failed to listen on port {}", port);
    }
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

export template<class TPollGroup = IPollGroup>
std::unique_ptr<TPollGroup> CreatePollGroup()
{
    return std::make_unique<TPollGroup>(GetInterface());
}

export void Free()
{
    interface->CloseListenSocket(listen_socket_handle);
    listen_socket_handle = GNS::HSteamListenSocket_Enum::Invalid;
    observers.clear();
}
} // namespace Soldank::NetworkingInterface
