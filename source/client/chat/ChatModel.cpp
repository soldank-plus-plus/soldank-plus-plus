module;
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>

export module Chat.Model;
export import Shared.Networking.ChatProtocol;

export namespace Soldank
{
struct ChatEntry
{
    ChatDelivery message;
    std::chrono::steady_clock::time_point received{ std::chrono::steady_clock::now() };
};
struct ChatHistoryEntry
{
    ChatChannel channel{};
    std::string text;
};
struct ChatModel
{
    bool open{};
    bool focus_input{};
    bool online{};
    bool connected{ true };
    bool team_available{};
    ChatChannel channel{};
    TeamType local_team{};
    std::string local_name{ "You" };
    std::string draft;
    std::string error;
    std::deque<ChatEntry> entries;
    std::deque<ChatHistoryEntry> history;
    std::unordered_map<std::uint32_t, ChatHistoryEntry> pending;
    std::function<void(const NetworkMessage&)> send;
    std::size_t revision{};
    std::size_t evictions{};
    std::optional<std::size_t> history_cursor;
    ChatHistoryEntry saved_draft;
    std::uint32_t next_request{ 1 };
    std::uint64_t local_message_id{ 1 };

    void Open(ChatChannel requested)
    {
        open = true;
        focus_input = true;
        channel = requested;
        error.clear();
        if (channel == ChatChannel::Team && !team_available) {
            error = "Team chat is unavailable in this mode or for spectators.";
        }
    }
    void Receive(const ChatDelivery& delivery)
    {
        if (std::ranges::any_of(entries, [&](const auto& entry) {
                return entry.message.message_id == delivery.message_id;
            })) {
            return;
        }
        entries.push_back({ .message = delivery });
        if (entries.size() > 500) {
            entries.pop_front();
            ++evictions;
        }
        ++revision;
    }
    void Result(const ChatResult& result)
    {
        const auto found = pending.find(result.request_id);
        if (found == pending.end()) {
            return;
        }
        if (result.status != ChatStatus::Accepted) {
            if (draft.empty()) {
                draft = found->second.text;
                channel = found->second.channel;
            }
            switch (result.status) {
                case ChatStatus::RateLimited:
                    error = "Sending too quickly. Try again shortly.";
                    break;
                case ChatStatus::NoTeam:
                    error = "Team chat is unavailable.";
                    break;
                default:
                    error = "Message was not sent. Check the text and connection.";
                    break;
            }
            open = true;
            focus_input = true;
        }
        pending.erase(found);
    }
    bool Submit()
    {
        if (draft.empty()) {
            open = false;
            focus_input = false;
            error.clear();
            return true;
        }
        // Paste may contain multiple lines; the wire contract remains single-line.
        for (auto& character : draft) {
            if (character == '\r' || character == '\n' || character == '\t') {
                character = ' ';
            }
        }
        if (!IsValidChatText(draft)) {
            error = "Enter a message of at most 512 UTF-8 bytes, without control characters.";
            return false;
        }
        if (channel == ChatChannel::Team && !team_available) {
            error = "Team chat is unavailable in this mode or for spectators.";
            return false;
        }
        if (!connected || (online && !send) || pending.size() >= 8) {
            error = "Chat is unavailable or waiting for the server.";
            return false;
        }
        const ChatRequest request{
            .request_id = next_request++,
            .channel = channel,
            .text = draft,
        };
        if (next_request == 0) {
            ++next_request;
        }
        if (history.empty() || history.back().text != draft || history.back().channel != channel) {
            history.push_back({ .channel = channel, .text = draft });
            if (history.size() > 100) {
                history.pop_front();
            }
        }
        pending.emplace(request.request_id, ChatHistoryEntry{ .channel = channel, .text = draft });
        draft.clear();
        history_cursor.reset();
        saved_draft = {};
        error.clear();
        open = false;
        if (online) {
            send(EncodeChat(request));
        } else {
            const auto id = local_message_id++;
            Receive({
              .message_id = id,
              .kind = ChatKind::Player,
              .channel = request.channel,
              .sender_session = 1,
              .team = local_team,
              .name = local_name,
              .text = request.text,
            });
            Result({
              .request_id = request.request_id,
              .status = ChatStatus::Accepted,
              .message_id = id,
            });
        }
        return true;
    }
    void NavigateHistory(bool older)
    {
        if (history.empty()) {
            return;
        }
        if (older) {
            if (!history_cursor) {
                saved_draft = {
                    .channel = channel,
                    .text = draft,
                };
                history_cursor = history.size() - 1;
            } else if (*history_cursor > 0) {
                --*history_cursor;
            }
        } else if (history_cursor) {
            if (*history_cursor + 1 < history.size()) {
                ++*history_cursor;
            } else {
                history_cursor.reset();
            }
        } else {
            return;
        }
        const auto& entry = history_cursor ? history.at(*history_cursor) : saved_draft;
        draft = entry.text;
        channel = entry.channel;
        error = channel == ChatChannel::Team && !team_available
                  ? "This history entry requires a team."
                  : "";
    }
    void Disconnect()
    {
        entries.clear();
        history.clear();
        pending.clear();
        draft.clear();
        saved_draft = {};
        history_cursor.reset();
        open = false;
        focus_input = false;
        connected = false;
        ++revision;
        ++evictions;
        next_request = 1;
        local_message_id = 1;
        error.clear();
    }
};
} // namespace Soldank
