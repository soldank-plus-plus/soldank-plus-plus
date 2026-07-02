module;

#include <cstdint>
#include <condition_variable>
#include <chrono>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include <rtc/rtc.hpp>

export module Networking.Transport.WebRtcServerTransport;

import Networking.Transport.IServerTransport;
import Networking.Transport.TransportTypes;

import Extern.Httplib;
import Extern.Spdlog;

export namespace Soldank
{
class WebRtcServerTransport final : public IServerTransport
{
public:
    ~WebRtcServerTransport() override
    {
        signaling_server_.Stop();
        if (signaling_thread_.joinable()) {
            signaling_thread_.join();
        }
    }

    void Init(std::uint16_t port) override
    {
        port_ = port;
        signaling_server_.Post("/webrtc/offer", [this](const Httplib::ServerRequest& request) {
            return HandleOffer(request);
        });
        signaling_server_.Post("/webrtc/candidate", [this](const Httplib::ServerRequest& request) {
            return HandleRemoteCandidate(request);
        });
        signaling_thread_ = std::thread([this]() {
            Spdlog::info("[WebRtcServerTransport] Signaling endpoint listening on 0.0.0.0:{}",
                         port_);
            if (!signaling_server_.Listen("0.0.0.0", port_)) {
                Spdlog::error("[WebRtcServerTransport] Signaling endpoint failed to listen on port {}",
                              port_);
            }
        });
    }

    void PollConnectionStateChanges() override {}

    void RegisterObserver(ConnectionStateChangedHandler observer) override
    {
        observers_.push_back(std::move(observer));
        Spdlog::info("[WebRtcServerTransport] Registered connection observer");
    }

    void Send(ConnectionId connection_id,
              std::span<const char> payload,
              DeliveryMode delivery_mode) override
    {
        auto data_channel = FindDataChannel(connection_id, delivery_mode);
        if (!data_channel) {
            Spdlog::warn("[WebRtcServerTransport] No {} DataChannel for connection {}",
                         ToChannelLabel(delivery_mode),
                         connection_id);
            return;
        }
        if (!data_channel->isOpen()) {
            Spdlog::warn("[WebRtcServerTransport] {} DataChannel for connection {} is not open",
                         ToChannelLabel(delivery_mode),
                         connection_id);
            return;
        }

        const auto* bytes =
          reinterpret_cast<const rtc::byte*>(static_cast<const void*>(payload.data()));
        if (!data_channel->send(bytes, payload.size())) {
            Spdlog::warn("[WebRtcServerTransport] {} DataChannel send buffered for connection {}",
                         ToChannelLabel(delivery_mode),
                         connection_id);
        }
    }

    void Close(ConnectionId connection_id) override
    {
        Spdlog::info("[WebRtcServerTransport] Stub close skipped: connection {}", connection_id);
    }

private:
    struct PendingDescription
    {
        std::mutex mutex;
        std::condition_variable condition_variable;
        std::optional<rtc::Description> description;
        bool gathering_complete = false;
    };

    struct PeerSession
    {
        ConnectionId connection_id;
        std::shared_ptr<rtc::PeerConnection> peer_connection;
        std::shared_ptr<rtc::DataChannel> unreliable_data_channel;
        std::shared_ptr<rtc::DataChannel> reliable_data_channel;
    };

    struct LocalCandidate
    {
        std::string candidate;
        std::string mid;
    };

    static std::string JsonEscape(const std::string& value)
    {
        std::ostringstream escaped;
        for (const char character : value) {
            switch (character) {
                case '"':
                    escaped << "\\\"";
                    break;
                case '\\':
                    escaped << "\\\\";
                    break;
                case '\b':
                    escaped << "\\b";
                    break;
                case '\f':
                    escaped << "\\f";
                    break;
                case '\n':
                    escaped << "\\n";
                    break;
                case '\r':
                    escaped << "\\r";
                    break;
                case '\t':
                    escaped << "\\t";
                    break;
                default:
                    escaped << character;
                    break;
            }
        }
        return escaped.str();
    }

    static std::string ExtractJsonString(const std::string& json, const std::string& key)
    {
        const std::string quoted_key = "\"" + key + "\"";
        auto key_position = json.find(quoted_key);
        if (key_position == std::string::npos) {
            return {};
        }

        auto colon_position = json.find(':', key_position + quoted_key.size());
        if (colon_position == std::string::npos) {
            return {};
        }

        auto value_position = json.find('"', colon_position + 1);
        if (value_position == std::string::npos) {
            return {};
        }

        std::string value;
        bool escaped = false;
        for (auto index = value_position + 1; index < json.size(); ++index) {
            const char character = json[index];
            if (escaped) {
                switch (character) {
                    case 'n':
                        value.push_back('\n');
                        break;
                    case 'r':
                        value.push_back('\r');
                        break;
                    case 't':
                        value.push_back('\t');
                        break;
                    default:
                        value.push_back(character);
                        break;
                }
                escaped = false;
                continue;
            }
            if (character == '\\') {
                escaped = true;
                continue;
            }
            if (character == '"') {
                return value;
            }
            value.push_back(character);
        }

        return {};
    }

    static Httplib::ServerResponse JsonResponse(int status, const std::string& body)
    {
        return { .status = status,
                 .body = body,
                 .content_type = "application/json",
                 .headers = { { .name = "Access-Control-Allow-Origin", .value = "*" } } };
    }

    static const char* ToChannelLabel(DeliveryMode delivery_mode)
    {
        return delivery_mode == DeliveryMode::Reliable ? "reliable" : "unreliable";
    }

    std::shared_ptr<rtc::DataChannel> FindDataChannel(ConnectionId connection_id,
                                                      DeliveryMode delivery_mode)
    {
        std::scoped_lock sessions_lock(sessions_mutex_);
        auto session = sessions_.find(connection_id);
        if (session == sessions_.end()) {
            return {};
        }
        return delivery_mode == DeliveryMode::Reliable ? session->second.reliable_data_channel
                                                       : session->second.unreliable_data_channel;
    }

    void RegisterDataChannel(ConnectionId connection_id,
                             const std::shared_ptr<rtc::DataChannel>& data_channel)
    {
        const auto label = data_channel->label();
        {
            std::scoped_lock sessions_lock(sessions_mutex_);
            auto session = sessions_.find(connection_id);
            if (session == sessions_.end()) {
                Spdlog::warn(
                  "[WebRtcServerTransport] DataChannel '{}' arrived for unknown connection {}",
                  label,
                  connection_id);
                return;
            }
            if (label == ToChannelLabel(DeliveryMode::Reliable)) {
                session->second.reliable_data_channel = data_channel;
            } else if (label == ToChannelLabel(DeliveryMode::Unreliable)) {
                session->second.unreliable_data_channel = data_channel;
            } else {
                Spdlog::warn(
                  "[WebRtcServerTransport] Ignoring unexpected DataChannel '{}' for connection {}",
                  label,
                  connection_id);
                return;
            }
        }

        data_channel->onOpen([connection_id, label]() {
            Spdlog::info("[WebRtcServerTransport] {} DataChannel open for connection {}",
                         label,
                         connection_id);
        });
        data_channel->onClosed([connection_id, label]() {
            Spdlog::info("[WebRtcServerTransport] {} DataChannel closed for connection {}",
                         label,
                         connection_id);
        });
    }

    void RemoveSession(ConnectionId connection_id)
    {
        std::scoped_lock sessions_lock(sessions_mutex_);
        sessions_.erase(connection_id);
    }

    Httplib::ServerResponse HandleOffer(const Httplib::ServerRequest& request)
    {
        if (request.body.empty()) {
            return JsonResponse(400, R"({"error":"empty WebRTC offer"})");
        }

        ConnectionId connection_id;
        {
            std::scoped_lock sessions_lock(sessions_mutex_);
            connection_id = next_connection_id_++;
        }
        const auto session_id = std::to_string(connection_id);
        auto peer_connection = std::make_shared<rtc::PeerConnection>(rtc::Configuration{});
        auto pending_description = std::make_shared<PendingDescription>();
        auto local_candidates = std::make_shared<std::vector<LocalCandidate>>();
        auto local_candidates_mutex = std::make_shared<std::mutex>();

        peer_connection->onLocalDescription(
          [pending_description](rtc::Description description) {
              {
                  std::scoped_lock lock(pending_description->mutex);
                  pending_description->description = std::move(description);
              }
              pending_description->condition_variable.notify_all();
          });

        peer_connection->onGatheringStateChange(
          [pending_description](rtc::PeerConnection::GatheringState state) {
              if (state == rtc::PeerConnection::GatheringState::Complete) {
                  {
                      std::scoped_lock lock(pending_description->mutex);
                      pending_description->gathering_complete = true;
                  }
                  pending_description->condition_variable.notify_all();
              }
          });

        {
            std::scoped_lock sessions_lock(sessions_mutex_);
            sessions_.emplace(connection_id,
                              PeerSession{ .connection_id = connection_id,
                                           .peer_connection = peer_connection,
                                           .unreliable_data_channel = nullptr,
                                           .reliable_data_channel = nullptr });
        }

        peer_connection->onDataChannel(
          [this, connection_id](std::shared_ptr<rtc::DataChannel> data_channel) {
              Spdlog::info("[WebRtcServerTransport] Connection {} received DataChannel '{}'",
                           connection_id,
                           data_channel->label());
              RegisterDataChannel(connection_id, data_channel);
          });
        peer_connection->onLocalCandidate(
          [session_id, local_candidates, local_candidates_mutex](rtc::Candidate candidate) {
              {
                  std::scoped_lock lock(*local_candidates_mutex);
                  local_candidates->push_back({ .candidate = static_cast<std::string>(candidate),
                                                .mid = candidate.mid() });
              }
              Spdlog::info("[WebRtcServerTransport] Session {} gathered ICE candidate",
                           session_id);
          });

        try {
            peer_connection->setRemoteDescription(
              rtc::Description(request.body, rtc::Description::Type::Offer));
            peer_connection->setLocalDescription();
        } catch (const std::exception& exception) {
            Spdlog::warn("[WebRtcServerTransport] Failed to process WebRTC offer: {}",
                         exception.what());
            RemoveSession(connection_id);
            return JsonResponse(400, R"({"error":"invalid WebRTC offer"})");
        }

        std::unique_lock lock(pending_description->mutex);
        pending_description->condition_variable.wait_for(
          lock, std::chrono::seconds(5), [&]() {
              return pending_description->description.has_value() &&
                     pending_description->gathering_complete;
          });

        if (!pending_description->description.has_value()) {
            Spdlog::warn("[WebRtcServerTransport] Timed out creating WebRTC answer");
            RemoveSession(connection_id);
            return JsonResponse(504, R"({"error":"timed out creating WebRTC answer"})");
        }

        const std::string answer_sdp = static_cast<std::string>(*pending_description->description);
        std::string local_candidates_json;
        {
            std::scoped_lock local_candidates_lock(*local_candidates_mutex);
            for (std::size_t index = 0; index < local_candidates->size(); ++index) {
                const auto& candidate = local_candidates->at(index);
                if (index > 0) {
                    local_candidates_json += ",";
                }
                local_candidates_json += std::string{ R"({"candidate":")" } +
                                         JsonEscape(candidate.candidate) + R"(","mid":")" +
                                         JsonEscape(candidate.mid) + R"("})";
            }
        }

        Spdlog::info("[WebRtcServerTransport] Created WebRTC signaling session {}", session_id);
        return JsonResponse(200,
                            std::string{ R"({"session_id":")" } + session_id +
                              R"(","type":"answer","sdp":")" + JsonEscape(answer_sdp) +
                              R"(","candidates":[)" + local_candidates_json + R"(]})");
    }

    Httplib::ServerResponse HandleRemoteCandidate(const Httplib::ServerRequest& request)
    {
        const auto session_id = ExtractJsonString(request.body, "session_id");
        const auto candidate = ExtractJsonString(request.body, "candidate");
        const auto mid = ExtractJsonString(request.body, "mid");

        if (session_id.empty() || candidate.empty()) {
            return JsonResponse(400, R"({"error":"session_id and candidate are required"})");
        }

        ConnectionId connection_id = 0;
        try {
            connection_id = static_cast<ConnectionId>(std::stoul(session_id));
        } catch (const std::exception&) {
            return JsonResponse(400, R"({"error":"invalid session_id"})");
        }

        std::shared_ptr<rtc::PeerConnection> peer_connection;
        {
            std::scoped_lock sessions_lock(sessions_mutex_);
            auto session = sessions_.find(connection_id);
            if (session != sessions_.end()) {
                peer_connection = session->second.peer_connection;
            }
        }

        if (!peer_connection) {
            return JsonResponse(404, R"({"error":"unknown WebRTC signaling session"})");
        }

        try {
            peer_connection->addRemoteCandidate(rtc::Candidate(candidate, mid));
        } catch (const std::exception& exception) {
            Spdlog::warn("[WebRtcServerTransport] Failed to add remote ICE candidate: {}",
                         exception.what());
            return JsonResponse(400, R"({"error":"invalid WebRTC ICE candidate"})");
        }

        Spdlog::info("[WebRtcServerTransport] Added remote ICE candidate for session {}",
                     session_id);
        return JsonResponse(200, R"({"status":"ok"})");
    }

    std::uint16_t port_ = 0;
    ConnectionId next_connection_id_ = 1;
    Httplib::Server signaling_server_;
    std::thread signaling_thread_;
    std::mutex sessions_mutex_;
    std::unordered_map<ConnectionId, PeerSession> sessions_;
    std::vector<ConnectionStateChangedHandler> observers_;
};
} // namespace Soldank
