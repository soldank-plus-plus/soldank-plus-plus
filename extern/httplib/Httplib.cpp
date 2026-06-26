module;

#include <memory>
#include <optional>
#include <string>
#include <utility>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#include "httplib.h"

export module Extern.Httplib;

export namespace Httplib
{
struct Response
{
    int status;
};

class Result
{
public:
    Result(Response response)
        : response_(response)
    {
    }

    Result(int error_code, std::string error_message)
        : error_code_(error_code)
        , error_message_(std::move(error_message))
    {
    }

    explicit operator bool() const { return response_.has_value(); }

    const Response* operator->() const { return &*response_; }

    int error_code() const { return error_code_; }

    const std::string& error_message() const { return error_message_; }

private:
    std::optional<Response> response_;
    int error_code_ = 0;
    std::string error_message_;
};

class Client
{
public:
    explicit Client(const std::string& scheme_host_port);

    Client(Client&&) noexcept;
    Client& operator=(Client&&) noexcept;

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    ~Client();

    void set_follow_location(bool on);

    void set_keep_alive(bool on);

    std::string host() const;

    Result Post(const std::string& path, const std::string& body, const std::string& content_type);

private:
    struct Implementation;
    std::unique_ptr<Implementation> implementation_;
};
} // namespace Httplib

namespace Httplib
{
struct Client::Implementation
{
    explicit Implementation(const std::string& scheme_host_port)
        : client(scheme_host_port)
    {
    }

    httplib::Client client;
};

Client::Client(const std::string& scheme_host_port)
    : implementation_(std::make_unique<Implementation>(scheme_host_port))
{
}

Client::Client(Client&&) noexcept = default;

Client& Client::operator=(Client&&) noexcept = default;

Client::~Client() = default;

void Client::set_follow_location(bool on)
{
    implementation_->client.set_follow_location(on);
}

void Client::set_keep_alive(bool on)
{
    implementation_->client.set_keep_alive(on);
}

std::string Client::host() const
{
    return implementation_->client.host();
}

Result Client::Post(const std::string& path, const std::string& body, const std::string& content_type)
{
    auto response = implementation_->client.Post(path, body, content_type);
    if (response) {
        return Result{ Response{ response->status } };
    }

    return Result{ static_cast<int>(response.error()), httplib::to_string(response.error()) };
}
} // namespace Httplib
