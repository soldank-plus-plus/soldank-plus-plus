#include <gtest/gtest.h>
#include <chrono>
#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

import Server.Chat.ChatService;
import Shared.Networking.ChatProtocol;

using namespace Soldank;

namespace
{
struct ChatServiceTest : public testing::Test
{
    std::vector<ChatPeer> peers{
        { .connection_id = 1, .session = 100, .team = TeamType::Alpha, .name = "Alpha" },
        { .connection_id = 2, .session = 200, .team = TeamType::Bravo, .name = "Bravo" },
        { .connection_id = 3, .session = 300, .team = TeamType::Alpha, .name = "Teammate" },
        { .connection_id = 4, .session = 400, .team = TeamType::Spectator, .name = "Spectator" },
    };
    std::vector<std::pair<unsigned int, NetworkMessage>> sent;
    ChatService service{ [this] { return peers; },
                         [this](unsigned int id, const NetworkMessage& message) {
                             sent.emplace_back(id, message);
                         } };
    std::vector<unsigned int> DeliveryRecipients() const
    {
        std::vector<unsigned int> result;
        for (const auto& [id, message] : sent) {
            if (DecodeChatDelivery(message)) {
                result.push_back(id);
            }
        }
        return result;
    }
};
} // namespace
TEST_F(ChatServiceTest, PublicAndTeamRoutingUsesCurrentAuthoritativeMembership)
{
    service.Receive(1, { .request_id = 1, .text = "public" });
    EXPECT_EQ(DeliveryRecipients(), (std::vector<unsigned int>{ 1, 2, 3, 4 }));
    sent.clear();
    service.Receive(1, { .request_id = 2, .channel = ChatChannel::Team, .text = "secret" });
    EXPECT_EQ(DeliveryRecipients(), (std::vector<unsigned int>{ 1, 3 }));
    const auto snapshot = DecodeChatDelivery(sent.front().second);
    ASSERT_TRUE(snapshot);
    EXPECT_EQ(snapshot->name, "Alpha");
    EXPECT_EQ(snapshot->team, TeamType::Alpha);
    peers.at(2).team = TeamType::Bravo;
    peers.at(0).name = "Changed";
    sent.clear();
    service.Receive(1, { .request_id = 3, .channel = ChatChannel::Team, .text = "secret again" });
    EXPECT_EQ(DeliveryRecipients(), (std::vector<unsigned int>{ 1 }));
    EXPECT_EQ(snapshot->name, "Alpha");
}
TEST_F(ChatServiceTest, SpectatorsNoTeamAndUnknownSendersCannotLeakTeamMessages)
{
    service.Receive(4, { .request_id = 1, .channel = ChatChannel::Team, .text = "private" });
    EXPECT_TRUE(DeliveryRecipients().empty());
    ASSERT_EQ(sent.size(), 1U);
    const auto result = DecodeChatResult(sent.front().second);
    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, ChatStatus::NoTeam);
    sent.clear();
    peers.at(0).team = TeamType::None;
    service.Receive(1, { .request_id = 2, .channel = ChatChannel::Team, .text = "private" });
    EXPECT_TRUE(DeliveryRecipients().empty());
    sent.clear();
    service.Receive(99, { .request_id = 3, .text = "forged" });
    EXPECT_TRUE(sent.empty());
}
TEST_F(ChatServiceTest, SystemBroadcastAndTargetsAreSessionSafe)
{
    ASSERT_TRUE(service.BroadcastSystemMessage("**Notice**"));
    EXPECT_EQ(DeliveryRecipients(), (std::vector<unsigned int>{ 1, 2, 3, 4 }));
    sent.clear();
    const std::vector<ChatPeer> targets{ peers.at(0), peers.at(3) };
    ASSERT_TRUE(service.SendSystemMessage(targets, "private notice"));
    EXPECT_EQ(DeliveryRecipients(), (std::vector<unsigned int>{ 1, 4 }));
    peers.at(0).session = 999;
    sent.clear();
    ASSERT_TRUE(service.SendSystemMessage(targets, "old target"));
    EXPECT_EQ(DeliveryRecipients(), (std::vector<unsigned int>{ 4 }));
    sent.clear();
    EXPECT_FALSE(service.BroadcastSystemMessage("bad\nnotice"));
    EXPECT_TRUE(sent.empty());
}
TEST_F(ChatServiceTest, TokenBucketUsesMonotonicTimeAndResetsForNewSessions)
{
    const ChatService::Clock::time_point now{};
    for (std::uint32_t i = 1; i <= 4; ++i) {
        service.Receive(1, { .request_id = i, .text = "message" }, now);
    }
    sent.clear();
    service.Receive(1, { .request_id = 5, .text = "too fast" }, now);
    ASSERT_EQ(sent.size(), 1U);
    const auto result = DecodeChatResult(sent.front().second);
    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, ChatStatus::RateLimited);
    sent.clear();
    service.Receive(1, { .request_id = 6, .text = "refilled" }, now + std::chrono::seconds(1));
    EXPECT_EQ(DeliveryRecipients().size(), 4U);
    peers.at(0).session = 999;
    sent.clear();
    service.Receive(1, { .request_id = 7, .text = "new session" }, now + std::chrono::seconds(1));
    EXPECT_EQ(DeliveryRecipients().size(), 4U);
}
TEST_F(ChatServiceTest, InvalidTextProducesOnlyPrivateFailure)
{
    service.Receive(1, { .request_id = 1, .text = std::string(513, 'x') });
    ASSERT_EQ(sent.size(), 1U);
    EXPECT_TRUE(DeliveryRecipients().empty());
    EXPECT_EQ(sent.front().first, 1U);
    const auto result = DecodeChatResult(sent.front().second);
    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, ChatStatus::InvalidText);
}
