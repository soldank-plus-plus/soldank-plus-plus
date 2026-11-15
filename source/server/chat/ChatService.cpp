module;

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

export module Server.Chat.ChatService;

export import Shared.Networking.ChatProtocol;

export namespace Soldank
{
struct ChatPeer
{
    unsigned int connection_id{};
    std::uint64_t session{};
    TeamType team{};
    std::string name;
};

class ChatService
{
public:
    using Clock = std::chrono::steady_clock;
    using Peers = std::function<std::vector<ChatPeer>()>;
    using Send = std::function<void(unsigned int, const NetworkMessage&)>;
    ChatService(Peers peers, Send send)
        : peers_(std::move(peers))
        , send_(std::move(send))
    {
    }

    void Receive(unsigned int connection_id,
                 const ChatRequest& request,
                 Clock::time_point now = Clock::now())
    {
        const auto peers = peers_();
        Prune(peers);
        const auto sender = std::ranges::find(peers, connection_id, &ChatPeer::connection_id);
        if (sender == peers.end()) {
            return;
        }
        ChatResult result{ .request_id = request.request_id };
        auto& bucket = buckets_[sender->session];
        if (!bucket.initialized) {
            bucket.updated = now;
            bucket.initialized = true;
        }
        bucket.tokens =
          std::min(4.0,
                   bucket.tokens +
                     std::max(0.0, std::chrono::duration<double>(now - bucket.updated).count()));
        bucket.updated = now;
        if (bucket.tokens < 1.0) {
            result.status = ChatStatus::RateLimited;
        } else {
            bucket.tokens -= 1.0;
            result.status = Validate(request, *sender);
        }
        if (result.status == ChatStatus::Accepted) {
            ChatDelivery const delivery{
                .message_id = next_message_id_++,
                .kind = ChatKind::Player,
                .channel = request.channel,
                .sender_session = sender->session,
                .team = sender->team,
                .name = sender->name,
                .text = request.text,
            };
            result.message_id = delivery.message_id;
            const auto message = EncodeChat(delivery);
            for (const auto& peer : peers) {
                if (request.channel == ChatChannel::All || peer.team == sender->team) {
                    send_(peer.connection_id, message);
                }
            }
        }
        send_(connection_id, EncodeChat(result));
    }

    bool BroadcastSystemMessage(std::string_view text) { return SystemMessage(peers_(), text); }
    // Recipients carry session identities, never reusable soldier slots alone.
    bool SendSystemMessage(std::span<const ChatPeer> recipients, std::string_view text)
    {
        const auto live = peers_();
        std::vector<ChatPeer> selected;
        for (const auto& peer : live) {
            if (std::ranges::any_of(recipients, [&](const ChatPeer& target) {
                    return target.connection_id == peer.connection_id &&
                           target.session == peer.session;
                })) {
                selected.push_back(peer);
            }
        }
        return SystemMessage(selected, text);
    }

private:
    struct Bucket
    {
        double tokens{ 4.0 };
        Clock::time_point updated;
        bool initialized{};
    };
    static ChatStatus Validate(const ChatRequest& request, const ChatPeer& sender)
    {
        if (request.request_id == 0 || request.channel > ChatChannel::Team ||
            !IsValidChatText(request.text) || !IsValidChatText(sender.name, CHAT_NAME_LIMIT)) {
            return ChatStatus::InvalidText;
        }
        if (request.channel == ChatChannel::Team && !IsChatTeam(sender.team)) {
            return ChatStatus::NoTeam;
        }
        return ChatStatus::Accepted;
    }
    void Prune(const std::vector<ChatPeer>& peers)
    {
        std::erase_if(buckets_, [&](const auto& item) {
            return std::ranges::none_of(
              peers, [&](const ChatPeer& peer) { return peer.session == item.first; });
        });
    }
    bool SystemMessage(std::span<const ChatPeer> recipients, std::string_view text)
    {
        if (!IsValidChatText(text)) {
            return false;
        }
        const auto message = EncodeChat(ChatDelivery{
          .message_id = next_message_id_++,
          .kind = ChatKind::System,
          .name = "System",
          .text = std::string(text),
        });
        for (const auto& peer : recipients) {
            send_(peer.connection_id, message);
        }
        return true;
    }
    Peers peers_;
    Send send_;
    std::unordered_map<std::uint64_t, Bucket> buckets_;
    std::uint64_t next_message_id_{ 1 };
};
} // namespace Soldank
