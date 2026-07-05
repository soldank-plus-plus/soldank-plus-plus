module;

#include <cstdint>
#include <memory>
#include <span>

export module Networking.Transport.ClientTransportAdapter;

import Networking.IConnection;
#if defined(SOLDANK_WEBASM_CLIENT_TRANSPORT)
import Networking.Browser.WebRtcBrowserConnection;
#else
import Networking.NetworkingInterface;
import Networking.Connection;
#endif

import Shared.Networking.NetworkEventDispatcher;
import Shared.Networking.NetworkMessage;
import Shared.Networking.DeliveryMode;

#if !defined(SOLDANK_WEBASM_CLIENT_TRANSPORT)
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
#if defined(SOLDANK_WEBASM_CLIENT_TRANSPORT)
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
#if !defined(SOLDANK_WEBASM_CLIENT_TRANSPORT)
        NetworkingInterface::PollConnectionStateChanges();
#endif
    }

    void SendNetworkMessage(const NetworkMessage& network_message, DeliveryMode delivery_mode)
    {
        connection_->SendNetworkMessage(network_message, delivery_mode);
    }

    void SetLag(int lag_to_add_milliseconds)
    {
#if defined(SOLDANK_WEBASM_CLIENT_TRANSPORT)
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
#if !defined(SOLDANK_WEBASM_CLIENT_TRANSPORT)
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
#if defined(SOLDANK_WEBASM_CLIENT_TRANSPORT)
    bool has_logged_ignored_lag_ = false;
#endif
#if !defined(SOLDANK_WEBASM_CLIENT_TRANSPORT)
    std::shared_ptr<Connection> gns_connection_;
#endif
};
} // namespace Soldank
