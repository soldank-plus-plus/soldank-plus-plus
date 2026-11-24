module;

#include <span>
#include <vector>
#include <string>
#include <tuple>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>
#include <cstring>
#include <cstdint>

#include "core/utility/Expected.hpp"

export module Shared.Networking.NetworkMessage;

import Shared.Networking.NetworkEvent;

namespace Soldank
{
export enum class ParseError : unsigned short
{
    InvalidStringSize = 0,
    BufferTooSmall,
    BufferTooBig,
    InvalidString,
    InvalidNetworkEvent,
};

export class NetworkMessage
{
public:
    NetworkMessage(std::span<const char> data)
        : data_{ data.begin(), data.end() }
    {
    }

    NetworkMessage(NetworkEvent event) { AppendBytes(std::to_underlying(event)); }

    template<class Arg>
    NetworkMessage(NetworkEvent event, Arg one_arg, unsigned int size)
        : NetworkMessage(event)
    {
        AppendBytes(one_arg, size);
    }

    template<class... Args>
    NetworkMessage(NetworkEvent event, Args... args)
        : NetworkMessage(event)
    {
        (AppendBytes(args), ...);
    }

    template<class Head, class... Tail>
    void AppendBytes(Head head, Tail... tail)
    {
        AppendBytes(head);
        (AppendBytes(tail), ...);
    }

    void AppendBytes(const std::string& text) { AppendString(text); }

    void AppendBytes(const char* text) { AppendString(text); }

    template<typename Head>
    void AppendBytes(Head head)
    {
        const auto* bytes = reinterpret_cast<const char*>(&head);
        data_.insert(data_.end(), bytes, bytes + sizeof(Head));
    }

    std::span<const char> GetData() const { return { data_ }; }

    template<typename... Args>
    static std::expected<std::tuple<Args...>, ParseError> ParseData(std::span<const char> data)
    {
        static_assert(sizeof...(Args) > 0);
        std::tuple<std::optional<Args>...> fields;
        std::optional<ParseError> error;
        auto remaining_fields = sizeof...(Args);
        auto read = [&]<typename Arg>(std::optional<Arg>& field) {
            auto value = ReadParameter<Arg>(data, --remaining_fields == 0);
            if (!value) {
                error = value.error();
                return false;
            }
            field.emplace(*std::move(value));
            return true;
        };

        if (!std::apply([&](auto&... field) { return (read(field) && ...); }, fields)) {
            return std::unexpected(*error);
        }

        return std::apply(
          [](auto&... field) { return std::tuple<Args...>{ *std::move(field)... }; }, fields);
    }

    template<typename... Args>
    std::expected<std::tuple<Args...>, ParseError> Parse() const
    {
        return ParseData<Args...>(data_);
    }

    std::expected<NetworkEvent, ParseError> GetNetworkEvent() const
    {
        auto data = GetData();
        return ReadValue<NetworkEvent>(data);
    }

private:
    template<typename Arg>
    static std::expected<Arg, ParseError> ReadValue(std::span<const char>& data, bool last = false)
    {
        if (data.size() < sizeof(Arg)) {
            return std::unexpected(ParseError::BufferTooSmall);
        }
        if (last && data.size() > sizeof(Arg)) {
            return std::unexpected(ParseError::BufferTooBig);
        }

        Arg value{};
        std::memcpy(&value, data.data(), sizeof(Arg));
        data = data.subspan(sizeof(Arg));
        return value;
    }

    static std::expected<std::string, ParseError> ReadString(std::span<const char>& data, bool last)
    {
        auto size = ReadValue<std::uint16_t>(data);
        if (!size) {
            return std::unexpected(size.error());
        }
        if (*size == 0 || *size > data.size()) {
            return std::unexpected(ParseError::InvalidStringSize);
        }
        // For the final field, report trailing bytes before validating the string contents.
        if (last && data.size() > *size) {
            return std::unexpected(ParseError::BufferTooBig);
        }

        std::string_view text{ data.data(), *size };
        if (text.contains('\0')) {
            return std::unexpected(ParseError::InvalidString);
        }
        data = data.subspan(*size);
        return std::string{ text };
    }

    template<typename Arg>
    static std::expected<Arg, ParseError> ReadParameter(std::span<const char>& data, bool last)
    {
        if constexpr (std::is_same_v<Arg, std::string>) {
            return ReadString(data, last);
        } else {
            return ReadValue<Arg>(data, last);
        }
    }

    void AppendString(std::string_view text)
    {
        auto length = static_cast<std::uint16_t>(text.size());
        AppendBytes(length);
        data_.insert(data_.end(), text.begin(), text.begin() + length);
    }

    std::vector<char> data_;
};
} // namespace Soldank
