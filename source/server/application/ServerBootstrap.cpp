module;

#include <cstdlib>
#include <cstdio>
#include <span>

export module Application.ServerBootstrap;

import Scripting.DaScript;

import Extern.GameNetworkingSockets;
import Extern.Spdlog;

export namespace Soldank
{
class ServerBootstrap
{
public:
    ServerBootstrap()
    {
        Spdlog::set_level(Spdlog::level::debug);

        DaScriptScriptingEngine::Init();
        Spdlog::info("daScript module initialized");

        GNS::SteamDatagramErrMsg err_msg;
        if (!GNS::GameNetworkingSocketsInit(nullptr, err_msg)) {
            Spdlog::error("GameNetworkingSockets_Init failed. {}", std::span(err_msg).data());
        }

        log_time_zero_ = GNS::GameNetworkingUtils()->GetLocalTimestamp();
        GNS::GameNetworkingUtils()->SetDebugOutputFunction(
          GNS::ESteamNetworkingSocketsDebugOutputType_Enum::Msg, DebugOutput);
    }

    ~ServerBootstrap()
    {
        DaScriptScriptingEngine::Shutdown();
        GNS::GameNetworkingSocketsKill();
    }

    ServerBootstrap(ServerBootstrap&& other) = delete;
    ServerBootstrap& operator=(ServerBootstrap&& other) = delete;
    ServerBootstrap(const ServerBootstrap& other) = delete;
    ServerBootstrap& operator=(const ServerBootstrap& other) = delete;

private:
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
};
} // namespace Soldank
