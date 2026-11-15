module;
#include <optional>
#include <functional>

export module Networking.ChatSendRequestNetworkEventHandler;

import Server.Chat.ChatService;
import Shared.Networking.NetworkEventDispatcher;

export namespace Soldank
{
class ChatSendRequestNetworkEventHandler final : public INetworkEventHandler
{
public:
    explicit ChatSendRequestNetworkEventHandler(ChatService& service)
        : service_(service)
    {
    }
    bool ShouldHandleNetworkEvent(NetworkEvent event) const override
    {
        return event == NetworkEvent::ChatSendRequest;
    }
    std::optional<ParseError> ValidateNetworkMessage(const NetworkMessage& message) const override
    {
        if (!DecodeChatRequest(message)) {
            return ParseError::InvalidString;
        }
        return std::nullopt;
    }
    NetworkEventHandlerResult HandleNetworkMessage(unsigned int sender,
                                                   const NetworkMessage& message) override
    {
        const auto request = DecodeChatRequest(message);
        if (!request) {
            return NetworkEventHandlerResult::Failure;
        }
        service_.get().Receive(sender, *request);
        return NetworkEventHandlerResult::Success;
    }

private:
    std::reference_wrapper<ChatService> service_;
};
} // namespace Soldank
