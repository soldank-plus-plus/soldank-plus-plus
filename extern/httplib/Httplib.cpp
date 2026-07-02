module;

#include <memory>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <utility>

export module Extern.Httplib;

export namespace Httplib
{
struct Response
{
    int status;
};

struct Header
{
    std::string name;
    std::string value;
};

struct ServerRequest
{
    std::string body;
};

struct ServerResponse
{
    int status;
    std::string body;
    std::string content_type;
    std::vector<Header> headers;
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

class Server
{
public:
    using Handler = std::function<ServerResponse(const ServerRequest&)>;

    Server();

    Server(Server&&) noexcept;
    Server& operator=(Server&&) noexcept;

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    ~Server();

    void Post(const std::string& path, Handler handler);

    bool Listen(const std::string& host, int port);

    void Stop();

private:
    struct Implementation;
    std::unique_ptr<Implementation> implementation_;
};
} // namespace Httplib
