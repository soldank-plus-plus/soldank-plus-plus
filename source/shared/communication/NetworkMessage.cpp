module;

#include <span>
#include <vector>
#include <string>
#include <tuple>
#include <expected>
#include <cmath>
#include <utility>

export module Shared.Networking.NetworkMessage;

import Shared.Networking.NetworkEvent;

namespace Soldank
{
export enum class ParseError : unsigned short {
    InvalidStringSize = 0,
    BufferTooSmall,
    BufferTooBig,
    InvalidString,
    InvalidNetworkEvent,
};

namespace
{
template<unsigned int N>
struct NetworkMessageData
{
    template<typename Arg>
    static typename std::enable_if<!std::is_same<std::string, Arg>::value,
                                   std::expected<Arg, ParseError>>::type
    ParseDataParameter(std::span<const char> data)
    {
        if (data.size() < sizeof(Arg)) {
            return std::unexpected(ParseError::BufferTooSmall);
        }

        Arg converted_data =
          *static_cast<const Arg*>(static_cast<const void*>(data.subspan(0, sizeof(Arg)).data()));
        return converted_data;
    }

    template<typename Arg>
    static typename std::enable_if<std::is_same<std::string, Arg>::value,
                                   std::expected<std::string, ParseError>>::type
    ParseDataParameter(std::span<const char> data)
    {
        auto text_size_or_error =
          ParseDataParameter<unsigned short>(data.subspan(0, sizeof(unsigned short)));
        if (!text_size_or_error.has_value()) {
            return std::unexpected(text_size_or_error.error());
        }

        unsigned short text_size = *text_size_or_error;
        if (text_size == 0 || text_size > data.size() - 2) {
            return std::unexpected(ParseError::InvalidStringSize);
        }

        for (char character : data.subspan(2, text_size)) {
            if (character == 0) {
                return std::unexpected(ParseError::InvalidString);
            }
        }

        auto text_data = data.subspan(2, text_size);
        std::string text{ text_data.begin(), text_data.end() };
        return text;
    }

    template<typename Arg>
    static typename std::enable_if<!std::is_same<std::string, Arg>::value,
                                   std::expected<unsigned int, ParseError>>::type
    ParseDataParameterSize(std::span<const char> data)
    {
        if (data.size() < sizeof(Arg)) {
            return std::unexpected(ParseError::BufferTooSmall);
        }

        return sizeof(Arg);
    }

    template<typename Arg>
    static typename std::enable_if<std::is_same<std::string, Arg>::value,
                                   std::expected<unsigned int, ParseError>>::type
    ParseDataParameterSize(std::span<const char> data)
    {
        auto text_size_or_error =
          ParseDataParameter<unsigned short>(data.subspan(0, sizeof(unsigned short)));
        if (!text_size_or_error.has_value()) {
            return std::unexpected(text_size_or_error.error());
        }
        unsigned short text_size = *text_size_or_error;
        if ((text_size == 0) || text_size > data.size() - 2) {
            return std::unexpected(ParseError::InvalidStringSize);
        }

        return sizeof(unsigned short) + text_size;
    }

    template<typename Head, typename... Tail>
    static
      typename std::enable_if<N != 1, std::expected<std::tuple<Head, Tail...>, ParseError>>::type
      ParseData(std::span<const char> data)
    {
        if (data.empty()) {
            return std::unexpected(ParseError::BufferTooSmall);
        }

        auto head_or_error = ParseDataParameter<Head>(data);
        if (!head_or_error.has_value()) {
            return std::unexpected(head_or_error.error());
        }

        auto tail_offset_or_error = ParseDataParameterSize<Head>(data);
        if (!tail_offset_or_error.has_value()) {
            return std::unexpected(tail_offset_or_error.error());
        }

        auto tail_or_error = NetworkMessageData<N - 1>::template ParseData<Tail...>(
          data.subspan(*tail_offset_or_error));
        if (!tail_or_error.has_value()) {
            return std::unexpected(tail_or_error.error());
        }
        std::tuple<Head> head_in_tuple{ *head_or_error };
        return std::tuple_cat(head_in_tuple, *tail_or_error);
    }

    template<typename Arg>
    static typename std::enable_if<N == 1, std::expected<std::tuple<Arg>, ParseError>>::type
    ParseData(std::span<const char> data)
    {
        if (data.empty()) {
            return std::unexpected(ParseError::BufferTooSmall);
        }

        auto last_parameter_size_or_error = ParseDataParameterSize<Arg>(data);
        if (!last_parameter_size_or_error.has_value()) {
            return std::unexpected(last_parameter_size_or_error.error());
        }
        auto last_parameter_size = *last_parameter_size_or_error;
        if (data.size() > last_parameter_size) {
            return std::unexpected(ParseError::BufferTooBig);
        }

        auto parsed_or_error = ParseDataParameter<Arg>(data);
        if (!parsed_or_error.has_value()) {
            return std::unexpected(parsed_or_error.error());
        }

        return { *parsed_or_error };
    }
};
} // namespace

export class NetworkMessage
{
public:
    NetworkMessage(std::span<const char> data)
        : data_{ data.begin(), data.end() }
    {
    }

    NetworkMessage(NetworkEvent event)
    {
        auto event_value = std::to_underlying(event);
        AppendBytes(event_value);
    }

    template<class Arg>
    NetworkMessage(NetworkEvent event, Arg one_arg, unsigned int size)
    {
        auto event_value = std::to_underlying(event);
        AppendBytes(event_value);
        AppendBytes(one_arg, size);
    }

    template<class... Args>
    NetworkMessage(NetworkEvent event, Args... args)
    {
        auto event_value = std::to_underlying(event);
        AppendBytes(event_value);
        AppendBytes(args...);
    }

    template<class Head, class... Tail>
    void AppendBytes(Head head, Tail... tail)
    {
        AppendBytes(head);
        AppendBytes(tail...);
    }

    void AppendBytes(const std::string& text)
    {
        unsigned short text_length = text.length();
        auto text_length_bytes_to_append =
          std::span{ static_cast<const char*>(static_cast<void*>(&text_length)),
                     sizeof(text_length) };
        data_.insert(
          data_.end(), text_length_bytes_to_append.cbegin(), text_length_bytes_to_append.cend());
        auto text_bytes_to_append =
          std::span{ static_cast<const char*>(static_cast<const void*>(text.c_str())),
                     text_length };
        data_.insert(data_.end(), text_bytes_to_append.cbegin(), text_bytes_to_append.cend());
    }

    void AppendBytes(const char* head)
    {
        std::string text = head;
        unsigned short text_length = text.length();
        auto text_length_bytes_to_append =
          std::span{ static_cast<const char*>(static_cast<void*>(&text_length)),
                     sizeof(text_length) };
        data_.insert(
          data_.end(), text_length_bytes_to_append.cbegin(), text_length_bytes_to_append.cend());
        auto text_bytes_to_append =
          std::span{ static_cast<const char*>(static_cast<const void*>(text.c_str())),
                     text_length };
        data_.insert(data_.end(), text_bytes_to_append.cbegin(), text_bytes_to_append.cend());
    }

    template<typename Head>
    void AppendBytes(Head head)
    {
        auto head_bytes_to_append =
          std::span{ static_cast<const char*>(static_cast<void*>(&head)), sizeof(Head) };
        data_.insert(data_.end(), head_bytes_to_append.cbegin(), head_bytes_to_append.cend());
    }

    std::span<const char> GetData() const { return { data_ }; }

    template<typename... Args>
    static std::expected<std::tuple<Args...>, ParseError> ParseData(std::span<const char> data)
    {
        return NetworkMessageData<sizeof...(Args)>::template ParseData<Args...>(data);
    }

    template<typename... Args>
    std::expected<std::tuple<Args...>, ParseError> Parse() const
    {
        return NetworkMessageData<sizeof...(Args)>::template ParseData<Args...>(data_);
    }

    std::expected<NetworkEvent, ParseError> GetNetworkEvent() const
    {
        if (data_.empty()) {
            return std::unexpected(ParseError::BufferTooSmall);
        }

        std::span<const char> data{ data_ };
        auto parsed = NetworkMessageData<1>::template ParseData<NetworkEvent>(
          data.subspan(0, std::min(sizeof(NetworkEvent), data.size())));
        if (!parsed.has_value()) {
            return std::unexpected(parsed.error());
        }
        auto [network_event] = *parsed;
        return network_event;
    }

private:
    std::vector<char> data_;
};
}; // namespace Soldank
