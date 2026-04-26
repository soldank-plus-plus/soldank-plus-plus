#include <gtest/gtest.h>

#include <memory>
#include <vector>

import Shared.Networking.NetworkEventDispatcher;
import Shared.Networking.NetworkMessage;
import Shared.Networking.NetworkEvent;

using namespace Soldank;

class NetworkEventHandlerForAssignPlayerIdExample : public NetworkEventHandlerBase<unsigned int>
{
public:
    unsigned int GetLastAssignedPlayerId() const { return last_assigned_player_id_; }
    void SetResult(NetworkEventHandlerResult result) { result_ = result; }

private:
    NetworkEvent GetTargetNetworkEvent() const override { return NetworkEvent::AssignPlayerId; }

    NetworkEventHandlerResult HandleNetworkMessageImpl(unsigned int /*sender_connection_id*/,
                                                       unsigned int assigned_player_id) override
    {
        last_assigned_player_id_ = assigned_player_id;
        return result_;
    }

    NetworkEventHandlerResult result_{ NetworkEventHandlerResult::Success };
    unsigned int last_assigned_player_id_{};
};

class NetworkEventHandlerForChatMessageExample : public NetworkEventHandlerBase<std::string>
{
public:
    std::string GetLastChatMessage() const { return last_chat_message_; }
    void SetResult(NetworkEventHandlerResult result) { result_ = result; }

private:
    NetworkEvent GetTargetNetworkEvent() const override { return NetworkEvent::ChatMessage; }

    NetworkEventHandlerResult HandleNetworkMessageImpl(unsigned int /*sender_connection_id*/,
                                                       std::string chat_message) override
    {
        last_chat_message_ = chat_message;
        return result_;
    }

    NetworkEventHandlerResult result_{ NetworkEventHandlerResult::Success };
    std::string last_chat_message_;
};

class NetworkEventDispatcherTests : public ::testing::Test
{
protected:
    NetworkEventDispatcherTests() = default;

    void SetUp() override
    {
        assign_player_id_network_event_handler_.SetResult(NetworkEventHandlerResult::Success);
        chat_message_network_event_handler_.SetResult(NetworkEventHandlerResult::Success);
        auto assign_player_id_network_event_handler_ptr =
          std::shared_ptr<NetworkEventHandlerForAssignPlayerIdExample>(
            &assign_player_id_network_event_handler_, [](auto*) {});
        auto chat_message_network_event_handler_ptr =
          std::shared_ptr<NetworkEventHandlerForChatMessageExample>(
            &chat_message_network_event_handler_, [](auto*) {});
        std::vector<std::shared_ptr<INetworkEventHandler>> network_event_handlers{
            assign_player_id_network_event_handler_ptr, chat_message_network_event_handler_ptr
        };
        network_event_dispatcher_ =
          std::make_shared<NetworkEventDispatcher>(network_event_handlers);
    }

    void TearDown() override { network_event_dispatcher_.reset(); }

    NetworkEventDispatcher::TDispatchResult ProcessNetworkMessage(
      const NetworkMessage& network_message)
    {
        ConnectionMetadata connection_metadata;
        return network_event_dispatcher_->ProcessNetworkMessage(connection_metadata,
                                                                network_message);
    }

    NetworkEventHandlerForAssignPlayerIdExample& GetAssignPlayerIdHandler()
    {
        return assign_player_id_network_event_handler_;
    }

    NetworkEventHandlerForChatMessageExample& GetChatMessageHandler()
    {
        return chat_message_network_event_handler_;
    }

private:
    std::shared_ptr<NetworkEventDispatcher> network_event_dispatcher_;
    NetworkEventHandlerForAssignPlayerIdExample assign_player_id_network_event_handler_;
    NetworkEventHandlerForChatMessageExample chat_message_network_event_handler_;
};

TEST_F(NetworkEventDispatcherTests, TestNetworkEventDispatcherParseError)
{
    std::vector<char> invalid_data{ 1 };
    NetworkMessage network_message(invalid_data);
    auto result = ProcessNetworkMessage(network_message);
    ASSERT_EQ(result.first, NetworkEventDispatchResult::ParseError);
    ASSERT_EQ(std::get<ParseError>(result.second), ParseError::BufferTooSmall);
}

TEST_F(NetworkEventDispatcherTests, TestNetworkEventDispatcherProcessAssignPlayerIdEvent)
{
    NetworkMessage network_message(NetworkEvent::AssignPlayerId, 5U);
    auto result = ProcessNetworkMessage(network_message);
    ASSERT_EQ(result.first, NetworkEventDispatchResult::Success);
    ASSERT_EQ(GetAssignPlayerIdHandler().GetLastAssignedPlayerId(), 5U);
}

TEST_F(NetworkEventDispatcherTests, TestNetworkEventDispatcherParseErrorInAssignPlayerIdEvent)
{
    std::vector<char> invalid_data{ 1, 0, 0, 0, 1 };
    NetworkMessage network_message(invalid_data);
    auto result = ProcessNetworkMessage(network_message);
    ASSERT_EQ(result.first, NetworkEventDispatchResult::ParseError);
    ASSERT_EQ(std::get<ParseError>(result.second), ParseError::BufferTooSmall);
}

TEST_F(NetworkEventDispatcherTests, TestNetworkEventDispatcherAssignPlayerIdEventFailure)
{
    GetAssignPlayerIdHandler().SetResult(NetworkEventHandlerResult::Failure);
    NetworkMessage network_message(NetworkEvent::AssignPlayerId, 5U);
    auto result = ProcessNetworkMessage(network_message);
    ASSERT_EQ(result.first, NetworkEventDispatchResult::HandlerFailure);
    ASSERT_EQ(std::get<NetworkEventHandlerResult>(result.second),
              NetworkEventHandlerResult::Failure);
}

TEST_F(NetworkEventDispatcherTests, TestNetworkEventDispatcherProcessChatMessageEvent)
{
    NetworkMessage network_message(NetworkEvent::ChatMessage, "Test Message");
    auto result = ProcessNetworkMessage(network_message);
    ASSERT_EQ(result.first, NetworkEventDispatchResult::Success);
    ASSERT_EQ(GetChatMessageHandler().GetLastChatMessage(), "Test Message");
}

TEST_F(NetworkEventDispatcherTests, TestNetworkEventDispatcherParseErrorInChatMessageEvent)
{
    std::vector<char> invalid_data{ 0, 0, 0, 0, 5, 0, 'T', 'e', 's', 't' };
    NetworkMessage network_message(invalid_data);
    auto result = ProcessNetworkMessage(network_message);
    ASSERT_EQ(result.first, NetworkEventDispatchResult::ParseError);
    ASSERT_EQ(std::get<ParseError>(result.second), ParseError::InvalidStringSize);
}

TEST_F(NetworkEventDispatcherTests, TestNetworkEventDispatcherProcessChatMessageEventFailure)
{
    GetChatMessageHandler().SetResult(NetworkEventHandlerResult::Failure);
    NetworkMessage network_message(NetworkEvent::ChatMessage, "Test Message");
    auto result = ProcessNetworkMessage(network_message);
    ASSERT_EQ(result.first, NetworkEventDispatchResult::HandlerFailure);
    ASSERT_EQ(std::get<NetworkEventHandlerResult>(result.second),
              NetworkEventHandlerResult::Failure);
}

TEST_F(NetworkEventDispatcherTests, TestNetworkEventDispatcherParseErrorInvalidNetworkEvent)
{
    std::vector<char> invalid_data{ 100, 100, 0, 0 };
    NetworkMessage network_message(invalid_data);
    auto result = ProcessNetworkMessage(network_message);
    ASSERT_EQ(result.first, NetworkEventDispatchResult::ParseError);
    ASSERT_EQ(std::get<ParseError>(result.second), ParseError::InvalidNetworkEvent);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
