#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <array>
#include <optional>
#include <string>
#include <utility>

import Shared.Networking.NetworkMessage;
import Shared.Networking.NetworkEventDispatcher;
import Shared.Networking.NetworkEvent;

using namespace Soldank;

TEST(NetworkMessageTests, TestNetworkMessageConstructorOneString)
{
    std::vector<unsigned char> expected_bytes{ 1, 0, 0, 0, 4, 0, 'T', 'e', 's', 't' };

    NetworkMessage network_message(NetworkEvent::AssignPlayerId, "Test");
    auto data = network_message.GetData();

    ASSERT_EQ(data.size(), expected_bytes.size());
    for (unsigned int i = 0; i < expected_bytes.size(); i++) {
        ASSERT_EQ(expected_bytes[i], data[i]);
    }

    auto network_event_or_error = network_message.GetNetworkEvent();
    ASSERT_TRUE(network_event_or_error.has_value());
    ASSERT_EQ(*network_event_or_error, NetworkEvent::AssignPlayerId);
}

#pragma pack(push, 1)
struct TestStruct
{
    char a;
    int b;
};
#pragma pack(pop)

TEST(NetworkMessageTests, TestNetworkMessageConstructorOneStruct)
{
    std::vector<unsigned char> expected_bytes{ 1, 0, 0, 0, 't', 4, 0, 0, 0 };

    TestStruct test_struct{ 't', 4 };

    NetworkMessage network_message(NetworkEvent::AssignPlayerId, test_struct);
    auto data = network_message.GetData();

    ASSERT_EQ(data.size(), expected_bytes.size());
    for (unsigned int i = 0; i < expected_bytes.size(); i++) {
        ASSERT_EQ(expected_bytes[i], data[i]);
    }

    auto network_event_or_error = network_message.GetNetworkEvent();
    ASSERT_TRUE(network_event_or_error.has_value());
    ASSERT_EQ(*network_event_or_error, NetworkEvent::AssignPlayerId);
}

TEST(NetworkMessageTests, TestNetworkMessageConstructorManyStrings)
{
    std::vector<unsigned char> expected_bytes{
        1, 0, 0, 0, 4, 0, 'T', 'e', 's', 't', 2, 0, '4', '9'
    };

    NetworkMessage network_message(NetworkEvent::AssignPlayerId, "Test", "49");
    auto data = network_message.GetData();

    ASSERT_EQ(data.size(), expected_bytes.size());
    for (unsigned int i = 0; i < expected_bytes.size(); i++) {
        ASSERT_EQ(expected_bytes[i], data[i]);
    }

    auto network_event_or_error = network_message.GetNetworkEvent();
    ASSERT_TRUE(network_event_or_error.has_value());
    ASSERT_EQ(*network_event_or_error, NetworkEvent::AssignPlayerId);
}

TEST(NetworkMessageTests, TestNetworkMessageConstructorManyVariables)
{
    std::vector<unsigned char> expected_bytes{ 1,   0,   0, 0, 4, 0, 'T', 'e',
                                               's', 't', 5, 0, 0, 0, 6,   0 };

    int variable_2 = 5;
    short variable_3 = 6;
    NetworkMessage network_message(NetworkEvent::AssignPlayerId, "Test", variable_2, variable_3);
    auto data = network_message.GetData();

    ASSERT_EQ(data.size(), expected_bytes.size());
    for (unsigned int i = 0; i < expected_bytes.size(); i++) {
        ASSERT_EQ(expected_bytes[i], data[i]);
    }

    auto network_event_or_error = network_message.GetNetworkEvent();
    ASSERT_TRUE(network_event_or_error.has_value());
    ASSERT_EQ(*network_event_or_error, NetworkEvent::AssignPlayerId);
}

TEST(NetworkMessageTests, TestNetworkMessageParseData)
{
    int expected_variable_1 = 5;
    int expected_variable_2 = 6;
    NetworkMessage network_message(
      NetworkEvent::AssignPlayerId, expected_variable_1, expected_variable_2, "Test", 5);
    auto data = network_message.GetData();

    auto parsed = NetworkMessage::ParseData<NetworkEvent, int, int, std::string, int>(data);

    ASSERT_TRUE(parsed.has_value());

    auto [network_event, variable_1, variable_2, text, variable_3] = *parsed;

    ASSERT_EQ(network_event, NetworkEvent::AssignPlayerId);
    ASSERT_EQ(variable_1, expected_variable_1);
    ASSERT_EQ(variable_2, expected_variable_2);
    ASSERT_EQ(text, "Test");
    ASSERT_EQ(variable_3, 5);
}

TEST(NetworkMessageTests, TestNetworkMessageGetParametersJustEvent)
{
    NetworkMessage network_message(NetworkEvent::AssignPlayerId);
    auto data = network_message.GetData();
    ASSERT_EQ(data.size(), sizeof(NetworkEvent));
    auto parsed = NetworkMessage::ParseData<NetworkEvent>(data);
    ASSERT_TRUE(parsed.has_value());
    auto [network_event] = *parsed;
    ASSERT_EQ(network_event, NetworkEvent::AssignPlayerId);
}

TEST(NetworkMessageTests, TestNetworkMessageConstructFromBytes)
{
    std::array<char, 8> expected_bytes{ 1, 0, 0, 0, 3, 0, 0, 0 };
    std::span<char> exp = expected_bytes;
    NetworkMessage network_message(exp);
    auto data = network_message.GetData();
    ASSERT_EQ(data.size(), 8);
    auto parsed = NetworkMessage::ParseData<NetworkEvent, int>(data);
    ASSERT_TRUE(parsed.has_value());
    auto [network_event, player_id] = *parsed;
    ASSERT_EQ(network_event, NetworkEvent::AssignPlayerId);
    ASSERT_EQ(player_id, 3);
    auto network_event_or_error = network_message.GetNetworkEvent();
    ASSERT_TRUE(network_event_or_error.has_value());
    ASSERT_EQ(*network_event_or_error, NetworkEvent::AssignPlayerId);
}

TEST(NetworkMessageTests, TestNetworkMessageParseDataUnalignedBuffer)
{
    NetworkMessage network_message(NetworkEvent::AssignPlayerId, std::uint8_t{ 3 });
    std::vector<char> bytes_with_padding{ 'x' };
    auto data = network_message.GetData();
    bytes_with_padding.insert(bytes_with_padding.end(), data.begin(), data.end());

    std::span<const char> unaligned_data{ bytes_with_padding.data() + 1, data.size() };
    auto parsed = NetworkMessage::ParseData<NetworkEvent, std::uint8_t>(unaligned_data);

    ASSERT_TRUE(parsed.has_value());
    auto [network_event, player_id] = *parsed;
    ASSERT_EQ(network_event, NetworkEvent::AssignPlayerId);
    ASSERT_EQ(player_id, 3);
}

TEST(NetworkMessageTests, TestNetworkMessageGetNetworkEventEmptyData)
{
    std::array<char, 0> expected_bytes{};
    std::span<char> exp = expected_bytes;
    NetworkMessage network_message(exp);
    auto data = network_message.GetData();
    ASSERT_EQ(data.size(), 0);
    auto network_event = network_message.GetNetworkEvent();
    ASSERT_FALSE(network_event.has_value());
    ASSERT_EQ(network_event.error(), ParseError::BufferTooSmall);
}

template<typename... Args>
void CheckParseError(std::span<const char> data, ParseError expected_parse_error)
{
    auto parsed = NetworkMessage::ParseData<Args...>(data);
    ASSERT_FALSE(parsed.has_value());
    ASSERT_EQ(parsed.error(), expected_parse_error);
}

TEST(NetworkMessageTests, TestNetworkMessageParseDataLastArgumentDoesNotExist)
{
    std::array<char, 8> expected_bytes{ 5, 0, 0, 0, 0, 0, 0, 0 };
    std::span<char> exp = expected_bytes;
    NetworkMessage network_message(exp);
    auto data = network_message.GetData();
    ASSERT_EQ(data.size(), 8);
    CheckParseError<NetworkEvent, unsigned int, std::string>(data, ParseError::BufferTooSmall);
}

TEST(NetworkMessageTests, TestNetworkMessageParseDataLastTwoArgumentsDoNotExist)
{
    std::array<char, 4> expected_bytes{ 5, 0, 0, 0 };
    std::span<char> exp = expected_bytes;
    NetworkMessage network_message(exp);
    auto data = network_message.GetData();
    ASSERT_EQ(data.size(), 4);
    CheckParseError<NetworkEvent, unsigned int, std::string>(data, ParseError::BufferTooSmall);
}

TEST(NetworkMessageTests, TestNetworkMessageParseDataEmptyData)
{
    std::array<char, 0> expected_bytes{};
    std::span<char> exp = expected_bytes;
    NetworkMessage network_message(exp);
    auto data = network_message.GetData();
    ASSERT_EQ(data.size(), 0);
    CheckParseError<NetworkEvent, unsigned int, std::string>(data, ParseError::BufferTooSmall);
}

TEST(NetworkMessageTests, TestNetworkMessageParseDataValidMessageInvalidParse)
{
    NetworkMessage network_message(NetworkEvent::AssignPlayerId);
    auto data = network_message.GetData();
    ASSERT_EQ(data.size(), sizeof(NetworkEvent));
    CheckParseError<NetworkEvent, int>(data, ParseError::BufferTooSmall);
}

TEST(NetworkMessageTests, TestNetworkMessageParseDataNetworkEventDifferentSize)
{
    std::vector<char> data{ 1 };
    CheckParseError<NetworkEvent>(data, ParseError::BufferTooSmall);
}

TEST(NetworkMessageTests, TestNetworkMessageParseDataTextLongerThanSize)
{
    std::vector<char> data{ 1, 0, 0, 0, 3, 0, 'T', 'e', 's', 't' };
    CheckParseError<NetworkEvent, std::string>(data, ParseError::BufferTooBig);
}

TEST(NetworkMessageTests, TestNetworkMessageParseDataTextShorterThanSize)
{
    std::vector<char> data{ 1, 0, 0, 0, 3, 0, 'T', 'e' };
    CheckParseError<NetworkEvent, std::string>(data, ParseError::InvalidStringSize);
}

TEST(NetworkMessageTests, TestNetworkMessageParseDataTextSizeEqualsZero)
{
    std::vector<char> data{ 1, 0, 0, 0, 0, 0, 'T', 'e', 's', 't' };
    CheckParseError<NetworkEvent, std::string>(data, ParseError::InvalidStringSize);
}

TEST(NetworkMessageTests, TestNetworkMessageParseDataAdditionalParameterDifferentSize)
{
    std::vector<char> data{ 1, 0, 0, 0, 4, 0, 0 };
    CheckParseError<NetworkEvent, int>(data, ParseError::BufferTooSmall);
}

TEST(NetworkMessageTests, TestNetworkMessageParseDataBufferBiggerThanExpected)
{
    std::vector<char> data{ 1, 0, 0, 0, 4, 0, 0, 0, 5, 6 };
    CheckParseError<NetworkEvent, int>(data, ParseError::BufferTooBig);
}

TEST(NetworkMessageTests, TestNetworkMessageParseDataMoreDataThanExpected)
{
    std::vector<char> data{ 1, 0, 0, 0, 4, 0, 'T', 'e', 's', 't', 1 };
    CheckParseError<NetworkEvent, std::string>(data, ParseError::BufferTooBig);
}

TEST(NetworkMessageTests, TestNetworkMessageParseDataStringHasZeroInTheMiddle)
{
    std::vector<char> data{ 1, 0, 0, 0, 4, 0, 'T', 'e', 0, 't' };
    CheckParseError<NetworkEvent, std::string>(data, ParseError::InvalidString);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
