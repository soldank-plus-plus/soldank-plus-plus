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
        Spdlog::info("[WebRtcServerTransport] Stub send skipped: connection {}, bytes {}, mode {}",
                     connection_id,
                     payload.size(),
                     delivery_mode == DeliveryMode::Reliable ? "reliable" : "unreliable");
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
        std::string session_id;
        std::shared_ptr<rtc::PeerConnection> peer_connection;
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

    Httplib::ServerResponse HandleOffer(const Httplib::ServerRequest& request)
    {
        if (request.body.empty()) {
            return JsonResponse(400, R"({"error":"empty WebRTC offer"})");
        }

        std::string session_id;
        {
            std::scoped_lock sessions_lock(sessions_mutex_);
            session_id = std::to_string(next_session_id_++);
        }
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

        peer_connection->onDataChannel([session_id](std::shared_ptr<rtc::DataChannel> data_channel) {
            Spdlog::info("[WebRtcServerTransport] Session {} received DataChannel '{}'",
                         session_id,
                         data_channel->label());
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
            return JsonResponse(504, R"({"error":"timed out creating WebRTC answer"})");
        }

        const std::string answer_sdp = static_cast<std::string>(*pending_description->description);
        {
            std::scoped_lock sessions_lock(sessions_mutex_);
            sessions_.push_back(
              { .session_id = session_id, .peer_connection = std::move(peer_connection) });
        }

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

        std::shared_ptr<rtc::PeerConnection> peer_connection;
        {
            std::scoped_lock sessions_lock(sessions_mutex_);
            for (const auto& session : sessions_) {
                if (session.session_id == session_id) {
                    peer_connection = session.peer_connection;
                    break;
                }
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
    unsigned int next_session_id_ = 1;
    Httplib::Server signaling_server_;
    std::thread signaling_thread_;
    std::mutex sessions_mutex_;
    std::vector<PeerSession> sessions_;
    std::vector<ConnectionStateChangedHandler> observers_;
};
} // namespace Soldank
