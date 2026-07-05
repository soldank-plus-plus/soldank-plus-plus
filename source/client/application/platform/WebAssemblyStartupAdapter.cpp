module;

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <optional>
#include <string>

#if defined(SOLDANK_WEBASM_CLIENT_TRANSPORT)
#include <emscripten/emscripten.h>
#endif

export module Application.Platform.WebAssemblyStartupAdapter;

export namespace Soldank
{
struct UrlServerEndpoint
{
    std::string ip;
    std::uint16_t port;
};

class WebAssemblyStartupAdapter
{
public:
    static std::optional<UrlServerEndpoint> GetServerEndpointFromUrl()
    {
#if defined(SOLDANK_WEBASM_CLIENT_TRANSPORT)
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        auto* raw_ip = reinterpret_cast<char*>(EM_ASM_PTR({
            const params = new URLSearchParams(globalThis.location ? globalThis.location.search
                                                                   : "");
            const endpoint = params.get("server") || params.get("connect") || "";
            let host = params.get("ip") || params.get("server_ip") || params.get("host") || "";
            if (!host && endpoint) {
                const bracketEnd = endpoint.startsWith("[") ? endpoint.indexOf("]") : -1;
                if (bracketEnd > 0) {
                    host = endpoint.slice(1, bracketEnd);
                } else {
                    const lastColon = endpoint.lastIndexOf(":");
                    host = lastColon > 0 ? endpoint.slice(0, lastColon) : endpoint;
                }
            }
            return stringToNewUTF8(host);
        }));
        std::unique_ptr<char, decltype(&std::free)> ip_holder(raw_ip, &std::free);
        if (ip_holder == nullptr || ip_holder.get()[0] == '\0') {
            return std::nullopt;
        }

        const int port = EM_ASM_INT({
            const params = new URLSearchParams(globalThis.location ? globalThis.location.search
                                                                   : "");
            const endpoint = params.get("server") || params.get("connect") || "";
            let portText = params.get("port") || params.get("server_port") || "";
            if (!portText && endpoint) {
                const bracketEnd = endpoint.startsWith("[") ? endpoint.indexOf("]") : -1;
                if (bracketEnd > 0 && endpoint[bracketEnd + 1] === ":") {
                    portText = endpoint.slice(bracketEnd + 2);
                } else {
                    const lastColon = endpoint.lastIndexOf(":");
                    portText = lastColon > 0 ? endpoint.slice(lastColon + 1) : "";
                }
            }
            const parsedPort = Number.parseInt(portText, 10);
            return Number.isInteger(parsedPort) ? parsedPort : 0;
        });

        if (port <= 0 || port > 65535) {
            return std::nullopt;
        }

        return UrlServerEndpoint{ .ip = ip_holder.get(),
                                  .port = static_cast<std::uint16_t>(port) };
#else
        return std::nullopt;
#endif
    }
};
} // namespace Soldank
