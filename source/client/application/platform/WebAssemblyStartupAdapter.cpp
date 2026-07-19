module;

#include <cstdlib>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#if defined(SOLDANK_WEBASM_CLIENT_TRANSPORT)
#include <emscripten/emscripten.h>
#endif

export module Application.Platform.WebAssemblyStartupAdapter;

import Application.LaunchParameters;

export namespace Soldank
{
class WebAssemblyStartupAdapter final : public ILaunchParameters
{
public:
    ParsedLaunchParameters Parse() const override
    {
#if defined(SOLDANK_WEBASM_CLIENT_TRANSPORT)
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        auto* raw_query = reinterpret_cast<char*>(EM_ASM_PTR(
          { return stringToNewUTF8(globalThis.location ? globalThis.location.search : ""); }));
        std::unique_ptr<char, decltype(&std::free)> query_holder(raw_query, &std::free);
        if (query_holder == nullptr) {
            return {};
        }

        auto parameter_values = ParseQueryString(query_holder.get());
        AddLegacyServerEndpoint(parameter_values);
        if (!parameter_values.contains("local") && !parameter_values.contains("online") &&
            !parameter_values.contains("map-editor") && parameter_values.contains("ip") &&
            parameter_values.contains("port")) {
            parameter_values.emplace("online", "");
        }
        return ParseLaunchParameters(parameter_values);
#else
        return {};
#endif
    }

private:
    static ParameterValues ParseQueryString(std::string_view query)
    {
        ParameterValues parameter_values;
        if (query.starts_with('?')) {
            query.remove_prefix(1);
        }

        while (!query.empty()) {
            const std::size_t separator = query.find('&');
            const std::string_view pair = query.substr(0, separator);
            const std::size_t equals = pair.find('=');
            const std::string key = DecodeUrlComponent(pair.substr(0, equals));
            if (!key.empty()) {
                const std::string_view value =
                  equals == std::string_view::npos ? std::string_view{} : pair.substr(equals + 1);
                parameter_values.try_emplace(key, DecodeUrlComponent(value));
            }
            if (separator == std::string_view::npos) {
                break;
            }
            query.remove_prefix(separator + 1);
        }
        return parameter_values;
    }

    static std::string DecodeUrlComponent(std::string_view value)
    {
        std::string decoded;
        decoded.reserve(value.size());
        for (std::size_t index = 0; index < value.size(); ++index) {
            if (value[index] == '+') {
                decoded.push_back(' ');
                continue;
            }
            if (value[index] == '%' && index + 2 < value.size()) {
                const int high = HexDigit(value[index + 1]);
                const int low = HexDigit(value[index + 2]);
                if (high >= 0 && low >= 0) {
                    decoded.push_back(static_cast<char>((high << 4) | low));
                    index += 2;
                    continue;
                }
            }
            decoded.push_back(value[index]);
        }
        return decoded;
    }

    static int HexDigit(char value)
    {
        if (value >= '0' && value <= '9') {
            return value - '0';
        }
        if (value >= 'a' && value <= 'f') {
            return value - 'a' + 10;
        }
        if (value >= 'A' && value <= 'F') {
            return value - 'A' + 10;
        }
        return -1;
    }

    static void AddLegacyServerEndpoint(ParameterValues& parameter_values)
    {
        const auto copy_alias = [&parameter_values](std::string_view target,
                                                    std::string_view alias) {
            if (!parameter_values.contains(std::string{ target })) {
                const auto it = parameter_values.find(std::string{ alias });
                if (it != parameter_values.end()) {
                    parameter_values.emplace(target, it->second);
                }
            }
        };
        copy_alias("ip", "server_ip");
        copy_alias("ip", "host");
        copy_alias("port", "server_port");

        if (parameter_values.contains("ip") || parameter_values.contains("port")) {
            return;
        }
        const auto endpoint = parameter_values.find("server");
        const auto connect = parameter_values.find("connect");
        const auto endpoint_it = endpoint != parameter_values.end() ? endpoint : connect;
        if (endpoint_it == parameter_values.end()) {
            return;
        }

        std::string_view value = endpoint_it->second;
        if (value.starts_with('[')) {
            const std::size_t bracket_end = value.find(']');
            if (bracket_end == std::string_view::npos || bracket_end + 1 >= value.size() ||
                value[bracket_end + 1] != ':') {
                return;
            }
            parameter_values.emplace("ip", value.substr(1, bracket_end - 1));
            parameter_values.emplace("port", value.substr(bracket_end + 2));
            return;
        }

        const std::size_t colon = value.rfind(':');
        if (colon == std::string_view::npos || colon == 0 || colon + 1 >= value.size()) {
            return;
        }
        parameter_values.emplace("ip", value.substr(0, colon));
        parameter_values.emplace("port", value.substr(colon + 1));
    }
};
} // namespace Soldank
