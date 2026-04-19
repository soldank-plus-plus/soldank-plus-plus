module;

#include "communication/NetworkEventDispatcher.hpp"
#include "communication/NetworkMessage.hpp"

#include <string>
#include <cassert>
#include <span>
#include <memory>

export module Networking.Connection;

import Networking.IConnection;

import Extern.GameNetworkingSockets;
import Extern.Spdlog;

export namespace Soldank
{
class Connection : public IConnection
{
public:
    Connection(GNS::ISteamNetworkingSockets* interface, GNS::HSteamNetConnection connection_handle)
        : interface_(interface)
        , connection_handle_(connection_handle)
    {
        std::string message = "test name";
        interface_->SendMessageToConnection(connection_handle_,
                                            message.c_str(),
                                            (std::uint32_t)message.length(),
                                            GNS::nSteamNetworkingSend::Reliable,
                                            nullptr);
    }

    void PollIncomingMessages(
      const std::shared_ptr<NetworkEventDispatcher>& network_event_dispatcher) final
    {
        while (true) {
            GNS::ISteamNetworkingMessage* p_incoming_msg = nullptr;
            int num_msgs =
              interface_->ReceiveMessagesOnConnection(connection_handle_, &p_incoming_msg, 1);
            if (num_msgs == 0) {
                return;
            }
            if (num_msgs < 0) {
                Spdlog::error("Error checking for messages");
                return;
            }

            std::span<const char> received_bytes{ static_cast<char*>(p_incoming_msg->m_pData),
                                                  static_cast<unsigned int>(
                                                    p_incoming_msg->m_cbSize) };

            NetworkMessage received_message(received_bytes);
            ConnectionMetadata connection_metadata{ .connection_id = p_incoming_msg->m_conn,
                                                    .send_message_to_connection =
                                                      [](const NetworkMessage& message) {} };
            // We don't need this anymore.
            p_incoming_msg->Release();
            network_event_dispatcher->ProcessNetworkMessage(connection_metadata, received_message);
        }
    }

    void CloseConnection() final
    {
        interface_->CloseConnection(connection_handle_, 0, nullptr, false);
    }

    void AssertConnectionInfo(
      GNS::SteamNetConnectionStatusChangedCallback_t* connection_info) const final
    {
        assert(connection_info->m_hConn == connection_handle_ ||
               connection_handle_ == GNS::HSteamNetConnection_Enum::Invalid);
    }

    void SendNetworkMessage(const NetworkMessage& network_message) final
    {
        interface_->SendMessageToConnection(connection_handle_,
                                            network_message.GetData().data(),
                                            network_message.GetData().size(),
                                            GNS::nSteamNetworkingSend::Unreliable,
                                            nullptr);
    }

private:
    GNS::ISteamNetworkingSockets* interface_;
    GNS::HSteamNetConnection connection_handle_;
};
} // namespace Soldank
