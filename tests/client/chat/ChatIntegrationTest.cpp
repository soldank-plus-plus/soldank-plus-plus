#include <gtest/gtest.h>
#include <array>
#include <algorithm>
#include <cstddef>
#include <memory>
#include <span>
#include <vector>

import Chat.Model;
import ClientState;
import Networking.ChatNetworkEventHandler;
import Networking.ChatSendRequestNetworkEventHandler;
import Server.Chat.ChatService;
import Shared.Networking.NetworkEventDispatcher;
import Shared.Networking.ChatProtocol;

using namespace Soldank;

namespace
{
struct ChatIntegrationTest : testing::Test
{
    std::vector<ChatPeer> peers{
        { .connection_id = 1, .session = 10, .team = TeamType::Alpha, .name = "Alpha" },
        { .connection_id = 2, .session = 20, .team = TeamType::Bravo, .name = "Bravo" },
        { .connection_id = 3, .session = 30, .team = TeamType::Spectator, .name = "Spectator" },
    };
    std::array<std::shared_ptr<ClientState>, 3> clients;
    std::array<std::shared_ptr<NetworkEventDispatcher>, 3> client_dispatchers;
    ChatService service{
        [this] { return peers; },
        [this](unsigned int connection, const NetworkMessage& message) {
            client_dispatchers.at(connection - 1)
              ->ProcessNetworkMessage(
                {
                  .connection_id = 0,
                  .send_message_to_connection = {},
                },
                message);
        },
    };
    std::shared_ptr<NetworkEventDispatcher> server_dispatcher;

protected:
    void SetUp() override
    {
        server_dispatcher = std::make_shared<NetworkEventDispatcher>(
          std::vector<std::shared_ptr<INetworkEventHandler>>{
            std::make_shared<ChatSendRequestNetworkEventHandler>(service),
          });
        for (std::size_t i = 0; i < clients.size(); ++i) {
            clients.at(i) = std::make_shared<ClientState>();
            clients.at(i)->chat.online = true;
            clients.at(i)->chat.team_available = i < 2;
            client_dispatchers.at(i) = std::make_shared<NetworkEventDispatcher>(
              std::vector<std::shared_ptr<INetworkEventHandler>>{
                std::make_shared<ChatNetworkEventHandler>(clients.at(i)),
              });
            clients.at(i)->chat.send = [this, i](const NetworkMessage& message) {
                server_dispatcher->ProcessNetworkMessage(
                  {
                    .connection_id = static_cast<unsigned int>(i + 1),
                    .send_message_to_connection = {},
                  },
                  message);
            };
        }
    }
};
} // namespace

TEST_F(ChatIntegrationTest, PublicDeliveryAndAcknowledgmentReachRealClientHandlers)
{
    auto& sender = clients.at(0)->chat;
    sender.draft = "**Hello** __everyone__";
    ASSERT_TRUE(sender.Submit());
    EXPECT_TRUE(std::ranges::all_of(clients, [](const auto& client) {
        return client->chat.entries.size() == 1 &&
               client->chat.entries.front().message.name == "Alpha" &&
               client->chat.entries.front().message.text == "**Hello** __everyone__";
    }));
    EXPECT_TRUE(sender.pending.empty());
}

TEST_F(ChatIntegrationTest, TeamTextNeverReachesOtherClientHandlers)
{
    auto& sender = clients.at(0)->chat;
    sender.channel = ChatChannel::Team;
    sender.draft = "private team message";
    ASSERT_TRUE(sender.Submit());
    EXPECT_EQ(sender.entries.size(), 1U);
    EXPECT_TRUE(clients.at(1)->chat.entries.empty());
    EXPECT_TRUE(clients.at(2)->chat.entries.empty());
    EXPECT_TRUE(sender.pending.empty());
}

TEST_F(ChatIntegrationTest, SystemTargetsAndBroadcastsUseSameDeliveryPipeline)
{
    ASSERT_TRUE(service.SendSystemMessage(std::span<const ChatPeer>(&peers.at(2), 1),
                                          "Private spectator notice"));
    EXPECT_TRUE(clients.at(0)->chat.entries.empty());
    EXPECT_EQ(clients.at(2)->chat.entries.size(), 1U);
    ASSERT_TRUE(service.BroadcastSystemMessage("Server notice"));
    EXPECT_EQ(clients.at(0)->chat.entries.size(), 1U);
    EXPECT_EQ(clients.at(2)->chat.entries.size(), 2U);
}

TEST_F(ChatIntegrationTest, ClientsCannotForgeSystemDeliveries)
{
    const auto forged = EncodeChat(ChatDelivery{
      .message_id = 1,
      .kind = ChatKind::System,
      .name = "System",
      .text = "forged notice",
    });
    const auto result = server_dispatcher->ProcessNetworkMessage(
      {
        .connection_id = 1,
        .send_message_to_connection = {},
      },
      forged);
    EXPECT_EQ(result.first, NetworkEventDispatchResult::ParseError);
    EXPECT_TRUE(clients.at(0)->chat.entries.empty());
    EXPECT_TRUE(clients.at(1)->chat.entries.empty());
    EXPECT_TRUE(clients.at(2)->chat.entries.empty());
}
