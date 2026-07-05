module;

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <span>
#include <thread>

export module Application.Platform.ClientTransportStartup;

#if !defined(SOLDANK_WEBASM_CLIENT_TRANSPORT)
import Extern.GameNetworkingSockets;
#endif
import Extern.Spdlog;

export namespace Soldank
{
class ClientTransportStartup
{
public:
    void Initialize()
    {
#if !defined(SOLDANK_WEBASM_CLIENT_TRANSPORT)
        GNS::SteamDatagramErrMsg err_msg;
        if (!GNS::GameNetworkingSocketsInit(nullptr, err_msg)) {
            Spdlog::error("GameNetworkingSocketsInit failed. {}", std::span(err_msg).data());
        }

        log_time_zero_ = GNS::GameNetworkingUtils()->GetLocalTimestamp();
        GNS::GameNetworkingUtils()->SetDebugOutputFunction(
          GNS::ESteamNetworkingSocketsDebugOutputType_Enum::Msg, DebugOutput);
#endif
    }

    void Shutdown()
    {
#if !defined(SOLDANK_WEBASM_CLIENT_TRANSPORT)
        // Give connections time to finish up.  This is an application layer protocol
        // here, it's not TCP.  Note that if you have an application and you need to be
        // more sure about cleanup, you won't be able to do this.  You will need to send
        // a message and then either wait for the peer to close the connection, or
        // you can pool the connection to see if any reliable data is pending.
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        GNS::GameNetworkingSocketsKill();
#endif
    }

private:
#if !defined(SOLDANK_WEBASM_CLIENT_TRANSPORT)
    static void DebugOutput(GNS::ESteamNetworkingSocketsDebugOutputType output_type,
                            const char* message)
    {
        GNS::SteamNetworkingMicroseconds time =
          GNS::GameNetworkingUtils()->GetLocalTimestamp() - log_time_zero_;
        Spdlog::info("{} {}", static_cast<double>(time) * 1e-6, message);
        fflush(stdout);
        if (output_type == GNS::ESteamNetworkingSocketsDebugOutputType_Enum::Bug) {
            exit(1);
        }
    }

    inline static GNS::SteamNetworkingMicroseconds log_time_zero_ = 0;
#endif
};
} // namespace Soldank
