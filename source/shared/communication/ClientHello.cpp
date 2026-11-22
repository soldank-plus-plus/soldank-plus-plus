module;

#include <optional>
#include <string>
#include <string_view>

export module Shared.Networking.ClientHello;

import Shared.Networking.ChatProtocol;

export namespace Soldank
{
constexpr std::string_view CLIENT_HELLO_PREFIX = "soldank-mode/3:";
std::string MakeClientHello(std::string_view nick)
{
    return std::string(CLIENT_HELLO_PREFIX) + std::string(nick);
}
std::optional<std::string> ParseClientHello(std::string_view hello)
{
    if (!hello.starts_with(CLIENT_HELLO_PREFIX)) {
        return std::nullopt;
    }
    hello.remove_prefix(CLIENT_HELLO_PREFIX.size());
    if (!IsValidChatText(hello, CHAT_NAME_LIMIT)) {
        return std::nullopt;
    }
    return std::string(hello);
}
} // namespace Soldank
