module;

#include "core/math/Glm.hpp"

#include <memory>
#include <optional>
#include <expected>
#include <variant>
#include <utility>
#include <functional>
#include <vector>

#include "core/utility/Expected.hpp"

export module Shared.Networking.NetworkEventDispatcher;

import Shared.Networking.NetworkEvent;
import Shared.Networking.NetworkMessage;
import Shared.Networking.NetworkPackets;
import Shared.Core.State.Control;
import Shared.Core.Animations;
import Shared.Core.Types.BulletType;
import Shared.Core.Types.TeamType;
import Shared.Core.Types.WeaponType;

export namespace Soldank
{
enum class NetworkEventDispatchResult
{
    Success = 0,
    ParseError,
    ObserverFailure,
    HandlerFailure
};

enum class NetworkEventHandlerResult
{
    Success = 0,
    Failure = 1,
};

struct ConnectionMetadata
{
    unsigned int connection_id;
    std::function<void(const NetworkMessage&)> send_message_to_connection;
};

class INetworkEventHandler
{
public:
    virtual ~INetworkEventHandler() = default;
    virtual bool ShouldHandleNetworkEvent(NetworkEvent network_event) const = 0;
    virtual std::optional<ParseError> ValidateNetworkMessage(
      const NetworkMessage& network_message) const = 0;
    virtual NetworkEventHandlerResult HandleNetworkMessage(
      unsigned int sender_connection_id,
      const NetworkMessage& network_message) = 0;
};

template<typename... NetworkMessageArgs>
class NetworkEventHandlerBase : public INetworkEventHandler
{
public:
    bool ShouldHandleNetworkEvent(NetworkEvent network_event) const override
    {
        return network_event == GetTargetNetworkEvent();
    }

    std::optional<ParseError> ValidateNetworkMessage(
      const NetworkMessage& network_message) const override
    {
        auto parsed = GetNetworkMessageOrError(network_message);
        if (!parsed.has_value()) {
            return parsed.error();
        }

        return std::nullopt;
    }

    NetworkEventHandlerResult HandleNetworkMessage(unsigned int sender_connection_id,
                                                   const NetworkMessage& network_message) override
    {
        auto parsed = GetNetworkMessageOrError(network_message);
        if (!parsed.has_value()) {
            // Before using this method, the network_message should be validated. Here we just
            // assume the network_message is correct
            return NetworkEventHandlerResult::Failure;
        }

        return std::apply(
          [this, sender_connection_id](NetworkEvent /*ignore*/,
                                       NetworkMessageArgs... network_message_args) {
              return HandleNetworkMessageImpl(sender_connection_id, network_message_args...);
          },
          *parsed);
    }

protected:
    std::expected<std::tuple<NetworkEvent, NetworkMessageArgs...>, ParseError>
    GetNetworkMessageOrError(const NetworkMessage& network_message) const
    {
        return network_message.Parse<NetworkEvent, NetworkMessageArgs...>();
    }

    virtual NetworkEvent GetTargetNetworkEvent() const = 0;
    virtual NetworkEventHandlerResult HandleNetworkMessageImpl(unsigned int sender_connection_id,
                                                               NetworkMessageArgs...) = 0;
};

class NetworkEventDispatcher
{

public:
    using TDispatchResult =
      std::pair<NetworkEventDispatchResult, std::variant<ParseError, NetworkEventHandlerResult>>;

    NetworkEventDispatcher(
      const std::vector<std::shared_ptr<INetworkEventHandler>>& network_event_handlers)
        : network_event_handlers_(network_event_handlers)
    {
    }

    TDispatchResult ProcessNetworkMessage(const ConnectionMetadata& connection_metadata,
                                          const NetworkMessage& network_message)
    {
        auto network_event_or_error = network_message.GetNetworkEvent();
        if (!network_event_or_error.has_value()) {
            return { NetworkEventDispatchResult::ParseError, network_event_or_error.error() };
        }

        auto network_event = *network_event_or_error;

        for (const auto& network_event_handler : network_event_handlers_) {
            if (!network_event_handler->ShouldHandleNetworkEvent(network_event)) {
                continue;
            }

            auto parse_error_or_nothing =
              network_event_handler->ValidateNetworkMessage(network_message);
            if (parse_error_or_nothing.has_value()) {
                return { NetworkEventDispatchResult::ParseError, parse_error_or_nothing.value() };
            }

            auto handler_result = network_event_handler->HandleNetworkMessage(
              connection_metadata.connection_id, network_message);
            switch (handler_result) {
                case NetworkEventHandlerResult::Success:
                    return { NetworkEventDispatchResult::Success, handler_result };
                case NetworkEventHandlerResult::Failure:
                    return { NetworkEventDispatchResult::HandlerFailure, handler_result };
            }
        }

        return { NetworkEventDispatchResult::ParseError, ParseError::InvalidNetworkEvent };
    }

    void AddNetworkEventHandler(const std::shared_ptr<INetworkEventHandler>& network_event_handler)
    {
        network_event_handlers_.push_back(network_event_handler);
    }

private:
    std::vector<std::shared_ptr<INetworkEventHandler>> network_event_handlers_;
};
} // namespace Soldank
