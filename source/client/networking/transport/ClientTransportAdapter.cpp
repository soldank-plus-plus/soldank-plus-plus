module;

#include <cstdint>
#include <memory>
#include <functional>
#include <span>

export module Networking.Transport.ClientTransportAdapter;

import Networking.IConnection;
#ifdef SOLDANK_WEBASM_CLIENT_TRANSPORT
import Networking.Browser.WebRtcBrowserConnection;
#else
import Networking.NetworkingInterface;
import Networking.Connection;
#endif

import Shared.Networking.NetworkEventDispatcher;
import Shared.Networking.NetworkMessage;
import Shared.Networking.DeliveryMode;

#ifndef SOLDANK_WEBASM_CLIENT_TRANSPORT
import Extern.GameNetworkingSockets;
#endif
import Extern.Spdlog;

export namespace Soldank
{
class ClientTransportAdapter
{
public:
    ClientTransportAdapter(const char* server_ip, std::uint16_t server_port)
    {
#ifdef SOLDANK_WEBASM_CLIENT_TRANSPORT
        connection_ = std::make_shared<WebRtcBrowserConnection>(server_ip, server_port);
#else
        NetworkingInterface::Init();
        NetworkingInterface::RegisterObserver(
          [this](GNS::SteamNetConnectionStatusChangedCallback_t* connection_info) {
              OnSteamNetConnectionStatusChanged(connection_info);
          });

        gns_connection_ = NetworkingInterface::CreateConnection(server_ip, server_port);
        connection_ = gns_connection_;
#endif
    }

    void Update(const std::shared_ptr<NetworkEventDispatcher>& network_event_dispatcher)
    {
        connection_->PollIncomingMessages(network_event_dispatcher);
#ifdef SOLDANK_WEBASM_CLIENT_TRANSPORT
        if (!disconnected_ && connection_->HasDisconnected()) {
            disconnected_ = true;
            if (on_disconnect_) {
                on_disconnect_();
            }
        }
#endif
#ifndef SOLDANK_WEBASM_CLIENT_TRANSPORT
        NetworkingInterface::PollConnectionStateChanges();
#endif
    }

    void SendNetworkMessage(const NetworkMessage& network_message, DeliveryMode delivery_mode)
    {
        connection_->SendNetworkMessage(network_message, delivery_mode);
    }

    void SetDisconnectHandler(std::function<void()> handler)
    {
        on_disconnect_ = std::move(handler);
    }

    void SetLag(int lag_to_add_milliseconds)
    {
#ifdef SOLDANK_WEBASM_CLIENT_TRANSPORT
        if (!has_logged_ignored_lag_) {
            Spdlog::info("Ignoring packet lag setting for browser WebRTC transport: {}",
                         lag_to_add_milliseconds);
            has_logged_ignored_lag_ = true;
        }
#else
        GNS::GameNetworkingUtils()->SetGlobalConfigValueInt32(
          GNS::ESteamNetworkingConfig::FakePacketLag_Send, lag_to_add_milliseconds / 2);
        GNS::GameNetworkingUtils()->SetGlobalConfigValueInt32(
          GNS::ESteamNetworkingConfig::FakePacketLag_Recv, lag_to_add_milliseconds / 2);
#endif
    }

private:
#ifndef SOLDANK_WEBASM_CLIENT_TRANSPORT
    void OnSteamNetConnectionStatusChanged(
      GNS::SteamNetConnectionStatusChangedCallback_t* connection_info)
    {
        gns_connection_->AssertConnectionInfo(connection_info);

        switch (connection_info->m_info.m_eState) {
            case GNS::ESteamNetworkingConnectionState::None:
                break;

            case GNS::ESteamNetworkingConnectionState::ClosedByPeer:
            case GNS::ESteamNetworkingConnectionState::ProblemDetectedLocally: {
                if (connection_info->m_eOldState ==
                    GNS::ESteamNetworkingConnectionState::Connecting) {
                    Spdlog::info("Connection to remote host failed. {}",
                                 std::span<char>(connection_info->m_info.m_szEndDebug).data());
                } else if (connection_info->m_info.m_eState ==
                           GNS::ESteamNetworkingConnectionState::ProblemDetectedLocally) {
                    Spdlog::info("Connection to remote host was lost. {}",
                                 std::span<char>(connection_info->m_info.m_szEndDebug).data());
                } else {
                    Spdlog::info("Remote host closed the connection. {}",
                                 std::span<char>(connection_info->m_info.m_szEndDebug).data());
                }

                connection_->CloseConnection();
                if (on_disconnect_) {
                    on_disconnect_();
                }
                break;
            }

            case GNS::ESteamNetworkingConnectionState::Connecting:
                break;

            case GNS::ESteamNetworkingConnectionState::Connected:
                Spdlog::info("Connected to server OK");
                break;

            default:
                break;
        }
    }
#endif

    std::shared_ptr<IConnection> connection_;
    std::function<void()> on_disconnect_;
#ifdef SOLDANK_WEBASM_CLIENT_TRANSPORT
    bool has_logged_ignored_lag_ = false;
    bool disconnected_{};
#endif
#ifndef SOLDANK_WEBASM_CLIENT_TRANSPORT
    std::shared_ptr<Connection> gns_connection_;
#endif
};
} // namespace Soldank
