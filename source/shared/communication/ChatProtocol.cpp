module;

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

export module Shared.Networking.ChatProtocol;

export import Shared.Networking.NetworkMessage;
export import Shared.Networking.NetworkEvent;
export import Shared.Core.Types.TeamType;

export namespace Soldank
{
constexpr std::size_t CHAT_TEXT_LIMIT = 512;
constexpr std::size_t CHAT_NAME_LIMIT = 64;

enum class ChatChannel : std::uint8_t
{
    All,
    Team,
};
enum class ChatKind : std::uint8_t
{
    Player,
    System,
};
enum class ChatStatus : std::uint8_t
{
    Accepted,
    InvalidText,
    NoTeam,
    RateLimited,
    Unavailable,
};

struct ChatRequest
{
    std::uint32_t request_id{};
    ChatChannel channel{};
    std::string text;
};
struct ChatDelivery
{
    std::uint64_t message_id{};
    ChatKind kind{};
    ChatChannel channel{};
    std::uint64_t sender_session{};
    TeamType team{};
    std::string name;
    std::string text;
};
struct ChatResult
{
    std::uint32_t request_id{};
    ChatStatus status{};
    std::uint64_t message_id{};
};

bool IsChatTeam(TeamType team)
{
    return team >= TeamType::Alpha && team <= TeamType::Delta;
}

bool IsAllowedChatCodepoint(std::uint32_t value)
{
    return value <= 0x10FFFF && (value < 0xD800 || value > 0xDFFF) && value >= 0x20 &&
           (value < 0x7F || value > 0x9F) && value != 0x2028 && value != 0x2029 &&
           (value < 0x202A || value > 0x202E) && (value < 0x2066 || value > 0x2069);
}

// Reject overlong sequences, surrogates, controls, and non-Unicode values.
bool IsValidChatText(std::string_view text, std::size_t limit = CHAT_TEXT_LIMIT)
{
    if (text.empty() || text.size() > limit) {
        return false;
    }
    bool visible = false;
    for (std::size_t i = 0; i < text.size();) {
        const auto first = static_cast<unsigned char>(text.at(i++));
        std::uint32_t value = first;
        unsigned int count = 0;
        std::uint32_t minimum = 0;
        if (first >= 0xC2 && first <= 0xDF) {
            value = first & 0x1FU;
            count = 1;
            minimum = 0x80;
        } else if (first >= 0xE0 && first <= 0xEF) {
            value = first & 0x0FU;
            count = 2;
            minimum = 0x800;
        } else if (first >= 0xF0 && first <= 0xF4) {
            value = first & 0x07U;
            count = 3;
            minimum = 0x10000;
        } else if (first >= 0x80) {
            return false;
        }
        if (text.size() - i < count) {
            return false;
        }
        for (unsigned int n = 0; n < count; ++n) {
            const auto next = static_cast<unsigned char>(text.at(i++));
            if ((next & 0xC0U) != 0x80U) {
                return false;
            }
            value = (value << 6U) | (next & 0x3FU);
        }
        if (value < minimum || !IsAllowedChatCodepoint(value)) {
            return false;
        }
        visible = visible || (value != 0x20 && value != 0xA0);
    }
    return visible;
}

namespace ChatWire
{
class Writer
{
public:
    explicit Writer(NetworkEvent event)
    {
        const NetworkMessage header(event);
        bytes_.assign(header.GetData().begin(), header.GetData().end());
    }
    void Number(std::uint64_t value, unsigned int width)
    {
        for (unsigned int i = 0; i < width; ++i) {
            bytes_.push_back(static_cast<char>((value >> (i * 8U)) & 0xFFU));
        }
    }
    void Text(std::string_view text)
    {
        Number(text.size(), 2);
        bytes_.insert(bytes_.end(), text.begin(), text.end());
    }
    NetworkMessage Finish() const { return { std::span<const char>(bytes_) }; }

private:
    std::vector<char> bytes_;
};
class Reader
{
public:
    Reader(const NetworkMessage& message, NetworkEvent event)
    {
        const auto actual = message.GetNetworkEvent();
        valid_ = actual && *actual == event && message.GetData().size() <= 640;
        if (valid_) {
            bytes_ = message.GetData().subspan(sizeof(NetworkEvent));
        }
    }
    std::uint64_t Number(unsigned int width)
    {
        if (bytes_.size() < width) {
            valid_ = false;
            return 0;
        }
        std::uint64_t value = 0;
        for (unsigned int i = 0; i < width; ++i) {
            value |=
              static_cast<std::uint64_t>(static_cast<unsigned char>(bytes_.subspan(i, 1).front()))
              << (i * 8U);
        }
        bytes_ = bytes_.subspan(width);
        return value;
    }
    std::string Text(std::size_t limit)
    {
        const auto size = Number(2);
        if (!valid_ || size > limit || size > bytes_.size()) {
            valid_ = false;
            return {};
        }
        const std::string text(bytes_.begin(), bytes_.begin() + static_cast<std::ptrdiff_t>(size));
        bytes_ = bytes_.subspan(size);
        return text;
    }
    bool Done() const { return valid_ && bytes_.empty(); }

private:
    std::span<const char> bytes_;
    bool valid_{};
};
} // namespace ChatWire

NetworkMessage EncodeChat(const ChatRequest& request)
{
    ChatWire::Writer writer(NetworkEvent::ChatSendRequest);
    writer.Number(request.request_id, 4);
    writer.Number(static_cast<std::uint8_t>(request.channel), 1);
    writer.Text(request.text);
    return writer.Finish();
}
NetworkMessage EncodeChat(const ChatDelivery& message)
{
    ChatWire::Writer writer(NetworkEvent::ChatMessage);
    writer.Number(message.message_id, 8);
    writer.Number(static_cast<std::uint8_t>(message.kind), 1);
    writer.Number(static_cast<std::uint8_t>(message.channel), 1);
    writer.Number(message.sender_session, 8);
    writer.Number(static_cast<unsigned int>(message.team), 1);
    writer.Text(message.name);
    writer.Text(message.text);
    return writer.Finish();
}
NetworkMessage EncodeChat(const ChatResult& result)
{
    ChatWire::Writer writer(NetworkEvent::ChatSendResult);
    writer.Number(result.request_id, 4);
    writer.Number(static_cast<std::uint8_t>(result.status), 1);
    writer.Number(result.message_id, 8);
    return writer.Finish();
}
std::optional<ChatRequest> DecodeChatRequest(const NetworkMessage& message)
{
    ChatWire::Reader reader(message, NetworkEvent::ChatSendRequest);
    ChatRequest request;
    request.request_id = static_cast<std::uint32_t>(reader.Number(4));
    request.channel = static_cast<ChatChannel>(reader.Number(1));
    request.text = reader.Text(CHAT_TEXT_LIMIT);
    if (!reader.Done() || request.request_id == 0 || request.channel > ChatChannel::Team) {
        return std::nullopt;
    }
    return request;
}
std::optional<ChatDelivery> DecodeChatDelivery(const NetworkMessage& message)
{
    ChatWire::Reader reader(message, NetworkEvent::ChatMessage);
    ChatDelivery delivery;
    delivery.message_id = reader.Number(8);
    delivery.kind = static_cast<ChatKind>(reader.Number(1));
    delivery.channel = static_cast<ChatChannel>(reader.Number(1));
    delivery.sender_session = reader.Number(8);
    delivery.team = static_cast<TeamType>(reader.Number(1));
    delivery.name = reader.Text(CHAT_NAME_LIMIT);
    delivery.text = reader.Text(CHAT_TEXT_LIMIT);
    if (!reader.Done() || delivery.message_id == 0 || delivery.kind > ChatKind::System ||
        delivery.channel > ChatChannel::Team || delivery.team > TeamType::Spectator ||
        !IsValidChatText(delivery.text) || !IsValidChatText(delivery.name, CHAT_NAME_LIMIT) ||
        (delivery.kind == ChatKind::Player && delivery.sender_session == 0) ||
        (delivery.kind == ChatKind::System &&
         (delivery.sender_session != 0 || delivery.team != TeamType::None ||
          delivery.channel != ChatChannel::All))) {
        return std::nullopt;
    }
    return delivery;
}
std::optional<ChatResult> DecodeChatResult(const NetworkMessage& message)
{
    ChatWire::Reader reader(message, NetworkEvent::ChatSendResult);
    ChatResult result;
    result.request_id = static_cast<std::uint32_t>(reader.Number(4));
    result.status = static_cast<ChatStatus>(reader.Number(1));
    result.message_id = reader.Number(8);
    if (!reader.Done() || result.request_id == 0 || result.status > ChatStatus::Unavailable ||
        ((result.status == ChatStatus::Accepted) != (result.message_id != 0))) {
        return std::nullopt;
    }
    return result;
}
} // namespace Soldank
