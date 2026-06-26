module;

#include <memory>
#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#include "httplib.h"

module Extern.Httplib;

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
