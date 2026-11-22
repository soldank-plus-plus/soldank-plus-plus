#include <gtest/gtest.h>
#include <cstddef>
#include <span>
#include <string>
#include <vector>

import Shared.Networking.ChatProtocol;
import Shared.Networking.ClientHello;

using namespace Soldank;

TEST(ChatProtocolTest, RoundTripsAllPayloads)
{
    const ChatRequest request{
        .request_id = 7,
        .channel = ChatChannel::Team,
        .text = "**Hello** __world__",
    };
    const auto parsed = DecodeChatRequest(EncodeChat(request));
    ASSERT_TRUE(parsed);
    EXPECT_EQ(parsed->text, request.text);
    EXPECT_EQ(parsed->channel, ChatChannel::Team);
    const ChatDelivery delivery{
        .message_id = 99,
        .kind = ChatKind::Player,
        .channel = ChatChannel::Team,
        .sender_session = 12345,
        .team = TeamType::Delta,
        .name = "Player",
        .text = "Здраво",
    };
    const auto decoded = DecodeChatDelivery(EncodeChat(delivery));
    ASSERT_TRUE(decoded);
    EXPECT_EQ(decoded->sender_session, 12345U);
    EXPECT_EQ(decoded->text, delivery.text);
    EXPECT_EQ(decoded->team, TeamType::Delta);
    const auto result = DecodeChatResult(
      EncodeChat(ChatResult{ .request_id = 7, .status = ChatStatus::Accepted, .message_id = 99 }));
    ASSERT_TRUE(result);
    EXPECT_EQ(result->message_id, 99U);
}

TEST(ChatProtocolTest, RejectsTruncatedRequests)
{
    const auto encoded = EncodeChat(ChatRequest{ .request_id = 1, .text = "test" });
    for (std::size_t size = 0; size < encoded.GetData().size(); ++size) {
        EXPECT_FALSE(DecodeChatRequest(NetworkMessage(encoded.GetData().first(size))));
    }
}

TEST(ChatProtocolTest, RejectsTrailingBytesAndUnknownEnums)
{
    const auto encoded = EncodeChat(ChatRequest{ .request_id = 1, .text = "test" });
    std::vector<char> bytes(encoded.GetData().begin(), encoded.GetData().end());
    bytes.push_back('!');
    EXPECT_FALSE(DecodeChatRequest(NetworkMessage(std::span<const char>(bytes))));
    bytes.pop_back();
    bytes.at(sizeof(NetworkEvent) + 4) = 7;
    EXPECT_FALSE(DecodeChatRequest(NetworkMessage(std::span<const char>(bytes))));
}

TEST(ChatProtocolTest, RejectsOversizeWrongDirectionAndInconsistentResults)
{
    EXPECT_FALSE(
      DecodeChatRequest(EncodeChat(ChatRequest{ .request_id = 1, .text = std::string(513, 'a') })));
    EXPECT_FALSE(
      DecodeChatResult(EncodeChat(ChatResult{ .request_id = 1, .status = ChatStatus::Accepted })));
    EXPECT_FALSE(DecodeChatRequest(
      EncodeChat(ChatDelivery{ .message_id = 1, .name = "System", .text = "x" })));
}

TEST(ChatProtocolTest, AcceptsUtf8Text)
{
    for (const auto* text : { "Hello", "Здраво", "日本語", "**bold**", "👩‍💻" }) {
        EXPECT_TRUE(IsValidChatText(text));
    }
}

TEST(ChatProtocolTest, RejectsControlsMalformedUtf8AndEmptyMessages)
{
    for (const auto& text : {
           std::string(),
           std::string("   "),
           std::string("a\nb"),
           std::string("a\0b", 3),
           std::string("\xC0\xAF"),
           std::string("\xED\xA0\x80"),
           std::string("\xF4\x90\x80\x80"),
           std::string{ static_cast<char>(0xE2), static_cast<char>(0x80), static_cast<char>(0xAE) },
           std::string("\xE2\x82"),
         }) {
        EXPECT_FALSE(IsValidChatText(text));
    }
}

TEST(ChatProtocolTest, EnforcesSizeAndHandshakeCompatibility)
{
    EXPECT_TRUE(IsValidChatText(std::string(512, 'x')));
    EXPECT_FALSE(IsValidChatText(std::string(513, 'x')));
    EXPECT_FALSE(ParseClientHello("soldank-mode/1:Player"));
    EXPECT_FALSE(ParseClientHello("soldank-mode/2:Player"));
    EXPECT_FALSE(ParseClientHello(MakeClientHello("bad\nname")));
    EXPECT_EQ(ParseClientHello(MakeClientHello("Player")), "Player");
}
