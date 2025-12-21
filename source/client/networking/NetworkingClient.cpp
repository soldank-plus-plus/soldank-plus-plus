module;

#include "networking/INetworkingClient.hpp"
#include "networking/IConnection.hpp"

#include "communication/NetworkEventDispatcher.hpp"

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>

#include "spdlog/spdlog.h"

#include <string>
#include <span>
#include <utility>
#include <memory>
#include <cstdint>

export module NetworkingClient;

import NetworkingInterface;
import Connection;

export namespace Soldank
{
class NetworkingClient : public INetworkingClient
{
public:
    NetworkingClient(const char* server_ip, std::uint16_t server_port)
    {
        NetworkingInterface::Init();
        NetworkingInterface::RegisterObserver(
          [this](SteamNetConnectionStatusChangedCallback_t* connection_info) {
              OnSteamNetConnectionStatusChanged(connection_info);
          });

        connection_ = NetworkingInterface::CreateConnection(server_ip, server_port);
    }

    void Update(const std::shared_ptr<NetworkEventDispatcher>& network_event_dispatcher) final
    {
        connection_->PollIncomingMessages(network_event_dispatcher);
        NetworkingInterface::PollConnectionStateChanges();
    }

    void SendNetworkMessage(const NetworkMessage& network_message) final
    {
        connection_->SendNetworkMessage(network_message);
    }

    void SetLag(int lag_to_add_milliseconds) final
    {
        SteamNetworkingUtils()->SetGlobalConfigValueInt32(
          k_ESteamNetworkingConfig_FakePacketLag_Send, lag_to_add_milliseconds / 2);
        SteamNetworkingUtils()->SetGlobalConfigValueInt32(
          k_ESteamNetworkingConfig_FakePacketLag_Recv, lag_to_add_milliseconds / 2);
    }

private:
    void OnSteamNetConnectionStatusChanged(
      SteamNetConnectionStatusChangedCallback_t* connection_info)
    {
        connection_->AssertConnectionInfo(connection_info);

        switch (connection_info->m_info.m_eState) {
            case k_ESteamNetworkingConnectionState_None:
                // NOTE: We will get callbacks here when we destroy connections.  You can ignore
                // these.
                break;

            case k_ESteamNetworkingConnectionState_ClosedByPeer:
            case k_ESteamNetworkingConnectionState_ProblemDetectedLocally: {
                // Print an appropriate message
                if (connection_info->m_eOldState == k_ESteamNetworkingConnectionState_Connecting) {
                    // Note: we could distinguish between a timeout, a rejected connection,
                    // or some other transport problem.
                    spdlog::info(
                      "We sought the remote host, yet our efforts were met with defeat. {}",
                      std::span<char>(connection_info->m_info.m_szEndDebug).data());
                } else if (connection_info->m_info.m_eState ==
                           k_ESteamNetworkingConnectionState_ProblemDetectedLocally) {
                    spdlog::info("Alas, troubles beset us; we have lost contact with the host. {}",
                                 std::span<char>(connection_info->m_info.m_szEndDebug).data());
                } else {
                    // NOTE: We could check the reason code for a normal disconnection
                    spdlog::info("The host hath bidden us farewell. {}",
                                 std::span<char>(connection_info->m_info.m_szEndDebug).data());
                }

                connection_->CloseConnection();
                // connection_handle_ = k_HSteamNetConnection_Invalid;
                break;
            }

            case k_ESteamNetworkingConnectionState_Connecting:
                // We will get this callback when we start connecting.
                // We can ignore this.
                break;

            case k_ESteamNetworkingConnectionState_Connected:
                spdlog::info("Connected to server OK");
                break;

            default:
                // Silences -Wswitch
                break;
        }
    }

    std::shared_ptr<IConnection> connection_;
    std::shared_ptr<NetworkEventDispatcher> network_event_dispatcher_;
};

} // namespace Soldank
