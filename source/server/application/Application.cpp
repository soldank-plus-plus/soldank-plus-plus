module;

#include <memory>
#include <vector>
#include <optional>
#include <cstdint>
#include <utility>

export module Application;

import Application.ServerBootstrap;
import Application.ServerConfig;
import Application.ServerConfigLoader;
import Application.ServerState;
import Runtime.ServerCommandQueues;
import Runtime.ServerRuntime;
import Sessions.PlayerSessionManager;

import Scripting.ScriptingEngine;
import Scripting.DaScript;

import Networking.IGameServer;
import Networking.GameServer;
import Networking.CoreEventsConnectionNotifier;
import Networking.LobbyClient;
import Networking.ServerNetworkHost;
import Networking.EventHandlers.KillCommandNetworkEventHandler;
import Networking.EventHandlers.PingCheckNetworkEventHandler;
import Networking.EventHandlers.SoldierInputNetworkEventHandler;

import Shared.Core.IWorld;
import Shared.Core.World;
import Shared.Core.GameModes.BuiltinGameModes;
import Shared.CoreEventHandler;

import Shared.Networking.NetworkEventDispatcher;

namespace Soldank
{
export class Application
{
public:
    Application()
        : config_(ServerConfigLoader::Load())
        , bootstrap_(std::make_unique<ServerBootstrap>())
        , world_(std::make_shared<World>())
        , server_state_(std::make_shared<ServerState>())
        , lobby_client_(std::make_shared<LobbyClient>())
        , player_session_manager_(std::make_unique<PlayerSessionManager>())
        , command_queues_(std::make_unique<ServerCommandQueues>())
    {
        auto definition = MakeBuiltinGameModeRegistry().Create(config_.game_mode);
        GameModeBuilder builder(definition);
        builder.FriendlyFire(config_.friendly_fire);
        if (config_.score_limit > 0) {
            builder.ScoreLimit(config_.score_limit);
        }
        builder.TimeLimit(config_.time_limit == 0
                            ? std::nullopt
                            : std::optional<std::uint32_t>(config_.time_limit));
        definition = builder.Build();
        if (config_.survival) {
            definition = WithSurvival(definition, config_.clear_weapons);
        }
        world_->GetStateManager()->ConfigureGameMode(std::move(definition));
        world_->GetStateManager()->SetRealistic(config_.realistic);
        world_->GetStateManager()->SetMaxGrenades(config_.max_grenades);
        world_->GetStateManager()->LoadMapDocument(config_.map_path);

        scripting_engine_ = std::make_shared<DaScriptScriptingEngine>();

        server_state_->server_name = config_.server_name;
        server_state_->server_port = config_.server_port;

        std::vector<std::shared_ptr<INetworkEventHandler>> network_event_handlers{};
        server_network_event_dispatcher_ =
          std::make_shared<NetworkEventDispatcher>(network_event_handlers);
        game_server_ = std::make_shared<GameServer>(
          config_.server_port, server_network_event_dispatcher_, world_, *player_session_manager_);
        server_network_event_dispatcher_->AddNetworkEventHandler(
          std::make_shared<PingCheckNetworkEventHandler>(game_server_));
        server_network_event_dispatcher_->AddNetworkEventHandler(
          std::make_shared<KillCommandNetworkEventHandler>(game_server_, *command_queues_));
        server_network_event_dispatcher_->AddNetworkEventHandler(
          std::make_shared<SoldierInputNetworkEventHandler>(
            game_server_, *player_session_manager_, *command_queues_));

        CoreEventHandler::ObserveAll(world_.get());
        CoreEventsConnectionNotifier::ObserveAll(
          game_server_.get(), world_->GetWorldEvents(), world_->GetPhysicsEvents());
        network_host_ = std::make_unique<ServerNetworkHost>(game_server_);
        server_runtime_ = std::make_unique<ServerRuntime>(config_,
                                                          world_,
                                                          *network_host_,
                                                          *lobby_client_,
                                                          *player_session_manager_,
                                                          *command_queues_);
    }

    ~Application() = default;

    Application(Application&& other) = delete;
    Application& operator=(Application&& other) = delete;
    Application(Application& other) = delete;
    Application& operator=(Application& other) = delete;

    void Run() { server_runtime_->Run(); }

private:
    ServerConfig config_;
    std::unique_ptr<ServerBootstrap> bootstrap_;
    std::shared_ptr<IGameServer> game_server_;
    std::shared_ptr<IWorld> world_;
    std::shared_ptr<NetworkEventDispatcher> server_network_event_dispatcher_;
    std::shared_ptr<ServerState> server_state_;
    std::shared_ptr<IScriptingEngine> scripting_engine_;
    std::shared_ptr<LobbyClient> lobby_client_;
    std::unique_ptr<ServerNetworkHost> network_host_;
    std::unique_ptr<PlayerSessionManager> player_session_manager_;
    std::unique_ptr<ServerCommandQueues> command_queues_;
    std::unique_ptr<ServerRuntime> server_runtime_;
};

} // namespace Soldank
