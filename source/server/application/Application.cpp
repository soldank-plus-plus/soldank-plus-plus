module;

#include <memory>
#include <vector>

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
import Networking.EventHandlers.KillCommandNetworkEventHandler;
import Networking.EventHandlers.PingCheckNetworkEventHandler;
import Networking.EventHandlers.SoldierInputNetworkEventHandler;

import Shared.Core.IWorld;
import Shared.Core.World;
import Shared.CoreEventHandler;

import Shared.Networking.NetworkEventDispatcher;

namespace Soldank
{
export class Application
{
public:
    Application()
        : config_(ServerConfigLoader().Load())
        , bootstrap_(std::make_unique<ServerBootstrap>())
        , world_(std::make_shared<World>())
        , server_state_(std::make_shared<ServerState>())
        , lobby_client_(std::make_shared<LobbyClient>())
        , player_session_manager_(std::make_unique<PlayerSessionManager>(*server_state_))
        , command_queues_(std::make_unique<ServerCommandQueues>())
    {
        world_->GetStateManager()->LoadMapDocument(config_.map_path);

        scripting_engine_ = std::make_shared<DaScriptScriptingEngine>();

        server_state_->server_name = config_.server_name;
        server_state_->server_port = config_.server_port;

        std::vector<std::shared_ptr<INetworkEventHandler>> network_event_handlers{};
        server_network_event_dispatcher_ =
          std::make_shared<NetworkEventDispatcher>(network_event_handlers);
        game_server_ = std::make_shared<GameServer>(
          config_.server_port, server_network_event_dispatcher_, world_, server_state_);
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
        server_runtime_ = std::make_unique<ServerRuntime>(config_,
                                                          world_,
                                                          game_server_,
                                                          lobby_client_,
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
    std::unique_ptr<PlayerSessionManager> player_session_manager_;
    std::unique_ptr<ServerCommandQueues> command_queues_;
    std::unique_ptr<ServerRuntime> server_runtime_;
};

} // namespace Soldank
