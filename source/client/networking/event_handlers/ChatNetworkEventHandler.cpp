module;
#include <memory>
#include <optional>
#include <utility>

export module Networking.ChatNetworkEventHandler;
import Chat.Model;
import ClientState;
import Shared.Networking.NetworkEventDispatcher;

export namespace Soldank
{
class ChatNetworkEventHandler final : public INetworkEventHandler
{
public:
    explicit ChatNetworkEventHandler(std::shared_ptr<ClientState> state)
        : state_(std::move(state))
    {
    }
    bool ShouldHandleNetworkEvent(NetworkEvent event) const override
    {
        return event == NetworkEvent::ChatMessage || event == NetworkEvent::ChatSendResult;
    }
    std::optional<ParseError> ValidateNetworkMessage(const NetworkMessage& message) const override
    {
        if (DecodeChatDelivery(message) || DecodeChatResult(message)) {
            return std::nullopt;
        }
        return ParseError::InvalidString;
    }
    NetworkEventHandlerResult HandleNetworkMessage(unsigned int /*sender*/,
                                                   const NetworkMessage& message) override
    {
        if (const auto delivery = DecodeChatDelivery(message)) {
            state_->chat.Receive(*delivery);
        } else if (const auto result = DecodeChatResult(message)) {
            state_->chat.Result(*result);
        } else {
            return NetworkEventHandlerResult::Failure;
        }
        return NetworkEventHandlerResult::Success;
    }

private:
    std::shared_ptr<ClientState> state_;
};
} // namespace Soldank
