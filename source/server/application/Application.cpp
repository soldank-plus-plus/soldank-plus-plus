module;

#include <core/math/Glm.hpp>

#include <chrono>
#include <cstdlib>
#include <memory>
#include <thread>

export module Application;

import Application.ServerState;

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

import Shared.Core.State.StateManager;
import Shared.Core.Entities.Bullet;

import Shared.Networking.NetworkPackets;
import Shared.Networking.NetworkEventDispatcher;
import Shared.Networking.NetworkEvent;

import Extern.SimpleIni;
import Extern.Spdlog;
import Extern.GameNetworkingSockets;

namespace Soldank
{
export class Application
{
public:
    Application()
        : world_(std::make_shared<World>())
        , lobby_client_(std::make_shared<LobbyClient>())
    {
        world_->GetStateManager()->GetMap().LoadMap("maps/ctf_Ash.pms");
        Spdlog::set_level(Spdlog::level::debug);

        DaScriptScriptingEngine::Init();
        Spdlog::info("daScript module initialized");

        scripting_engine_ = std::make_shared<DaScriptScriptingEngine>();

        server_state_ = std::make_shared<ServerState>();

        SimpleIni::CSimpleIniA ini_config;
        SimpleIni::SI_Error rc = ini_config.LoadFile("soldat.ini");
        if (rc < 0) {
            Spdlog::critical("Error: INI File could not be loaded: soldat.ini");
            exit(1);
        } else {
            server_state_->server_port = ini_config.GetLongValue("NETWORK", "Port");
            if (server_state_->server_port == 0) {
                Spdlog::critical("Error: Port can't be 0");
                exit(1);
            }

            const char* server_name_cstr = ini_config.GetValue("NETWORK", "Server_Name");
            if (server_name_cstr == nullptr) {
                Spdlog::critical("Error: Port can't be 0");
                exit(1);
            }
            server_state_->server_name = server_name_cstr;
        }

        GNS::SteamDatagramErrMsg err_msg;
        if (!GNS::GameNetworkingSocketsInit(nullptr, err_msg)) {
            Spdlog::error("GameNetworkingSockets_Init failed. {}", std::span(err_msg).data());
        }

        log_time_zero_ = GNS::GameNetworkingUtils()->GetLocalTimestamp();

        GNS::GameNetworkingUtils()->SetDebugOutputFunction(
          GNS::ESteamNetworkingSocketsDebugOutputType_Enum::Msg, DebugOutput);

        for (unsigned int& last_processed_input_id : server_state_->last_processed_input_id) {
            last_processed_input_id = 0;
        }
        std::vector<std::shared_ptr<INetworkEventHandler>> network_event_handlers{};
        server_network_event_dispatcher_ =
          std::make_shared<NetworkEventDispatcher>(network_event_handlers);
        game_server_ = std::make_shared<GameServer>(
          server_state_->server_port, server_network_event_dispatcher_, world_, server_state_);
        server_network_event_dispatcher_->AddNetworkEventHandler(
          std::make_shared<PingCheckNetworkEventHandler>(game_server_));
        server_network_event_dispatcher_->AddNetworkEventHandler(
          std::make_shared<KillCommandNetworkEventHandler>(world_, game_server_));
        server_network_event_dispatcher_->AddNetworkEventHandler(
          std::make_shared<SoldierInputNetworkEventHandler>(world_, server_state_, game_server_));

        CoreEventHandler::ObserveAll(world_.get());
        CoreEventsConnectionNotifier::ObserveAll(
          game_server_.get(), world_->GetWorldEvents(), world_->GetPhysicsEvents());
    }

    ~Application()
    {
        DaScriptScriptingEngine::Shutdown();
        GNS::GameNetworkingSocketsKill();
    }

    Application(Application&& other) = delete;
    Application& operator=(Application&& other) = delete;
    Application(Application& other) = delete;
    Application& operator=(Application& other) = delete;

    void Run()
    {
        Spdlog::info("Server started!");

        world_->SetShouldStopGameLoopCallback([&]() { return false; });
        world_->SetPreGameLoopIterationCallback([&]() {});
        world_->SetPreWorldUpdateCallback([&]() { game_server_->Update(); });
        world_->SetPostWorldUpdateCallback([&](const StateManager& state_manager) {
            state_manager.ForEachSoldier([&](const auto& soldier) {
                SoldierStatePacket update_soldier_state_packet{
                    .game_tick = state_manager.GetGameTick(),
                    .player_id = soldier.id,
                    .position_x = soldier.particle.position.x,
                    .position_y = soldier.particle.position.y,
                    .old_position_x = soldier.particle.old_position.x,
                    .old_position_y = soldier.particle.old_position.y,
                    .body_animation_type = soldier.body_animation->GetType(),
                    .body_animation_frame = soldier.body_animation->GetFrame(),
                    .body_animation_speed = soldier.body_animation->GetSpeed(),
                    .body_animation_count = soldier.body_animation->GetCount(),
                    .legs_animation_type = soldier.legs_animation->GetType(),
                    .legs_animation_frame = soldier.legs_animation->GetFrame(),
                    .legs_animation_speed = soldier.legs_animation->GetSpeed(),
                    .legs_animation_count = soldier.legs_animation->GetCount(),
                    .velocity_x = soldier.particle.GetVelocity().x,
                    .velocity_y = soldier.particle.GetVelocity().y,
                    .force_x = soldier.particle.GetForce().x,
                    .force_y = soldier.particle.GetForce().y,
                    .on_ground = soldier.on_ground,
                    .on_ground_for_law = soldier.on_ground_for_law,
                    .on_ground_last_frame = soldier.on_ground_last_frame,
                    .on_ground_permanent = soldier.on_ground_permanent,
                    .stance = soldier.stance,
                    .mouse_map_position_x = (float)soldier.control.mouse_aim_x,
                    .mouse_map_position_y = (float)soldier.control.mouse_aim_y,
                    .using_jets = soldier.control.jets,
                    .jets_count = soldier.jets_count,
                    .active_weapon = soldier.active_weapon,
                    .last_processed_input_id = server_state_->last_processed_input_id.at(soldier.id)
                };
                game_server_->SendNetworkMessageToAll(
                  { NetworkEvent::SoldierState, update_soldier_state_packet });
            });

            // Re-register the server in the lobby every 3 minutes
            if (state_manager.GetGameTick() % (3600 * 3) == 0) {
                lobby_client_->Register(server_state_->server_name, server_state_->server_port);
            }

            // for (const auto& soldier : state->soldiers) {
            //     if (soldier.active) {
            //         Spdlog::info("{}, Player {} pos: {}, {}; velocity: {} {}; force: {} {}",
            //                      server_state_->last_processed_input_id,
            //                      soldier.id,
            //                      soldier.particle.position.x,
            //                      soldier.particle.position.y,
            //                      soldier.particle.GetVelocity().x,
            //                      soldier.particle.GetVelocity().y,
            //                      soldier.particle.GetForce().x,
            //                      soldier.particle.GetForce().y);
            //     }
            // }
        });
        world_->SetPostGameLoopIterationCallback([&](const StateManager& /*state_manager*/,
                                                     double /*frame_percent*/,
                                                     int /*last_fps*/) {});
        world_->SetPreProjectileSpawnCallback([&](const BulletParams& bullet_params) {
            ProjectileSpawnPacket projectile_spawn_packet{
                .projectile_id = 0, // TODO: set the correct ID
                .style = bullet_params.style,
                .weapon = bullet_params.weapon,
                .position_x = bullet_params.position.x,
                .position_y = bullet_params.position.y,
                .velocity_x = bullet_params.velocity.x,
                .velocity_y = bullet_params.velocity.y,
                .timeout = bullet_params.timeout,
                .hit_multiply = bullet_params.hit_multiply,
                .team = bullet_params.team,
                .owner_id = bullet_params.owner_id,
            };
            game_server_->SendNetworkMessageToAll(
              { NetworkEvent::ProjectileSpawn, projectile_spawn_packet });

            return true;
        });

        // Increased tick rate to 240 (tickrate * 4) because when it was 64
        // (which is higher than the tickrate - 60) on the client there was
        // always a difference of one frame (i.e. there was a visual bug where
        // soldier was always getting teleported few pixels behind)
        // TODO: Figure out why it's related and possibly fix
        world_->SetFPSLimit(240);
        world_->RunLoop();

        // Give connections time to finish up.  This is an application layer protocol
        // here, it's not TCP.  Note that if you have an application and you need to be
        // more sure about cleanup, you won't be able to do this.  You will need to send
        // a message and then either wait for the peer to close the connection, or
        // you can pool the connection to see if any reliable data is pending.
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

private:
    static void DebugOutput(GNS::ESteamNetworkingSocketsDebugOutputType output_type,
                            const char* message)
    {
        GNS::SteamNetworkingMicroseconds time =
          GNS::GameNetworkingUtils()->GetLocalTimestamp() - log_time_zero_;
        Spdlog::info("{} {}", (double)time * 1e-6, message);
        fflush(stdout);
        if (output_type == GNS::ESteamNetworkingSocketsDebugOutputType_Enum::Bug) {
            exit(1);
        }
    }

    static GNS::SteamNetworkingMicroseconds log_time_zero_;

    std::shared_ptr<IGameServer> game_server_;
    std::shared_ptr<IWorld> world_;
    std::shared_ptr<NetworkEventDispatcher> server_network_event_dispatcher_;
    std::shared_ptr<ServerState> server_state_;
    std::shared_ptr<IScriptingEngine> scripting_engine_;
    std::shared_ptr<LobbyClient> lobby_client_;
};

GNS::SteamNetworkingMicroseconds Application::log_time_zero_ = 0;

} // namespace Soldank
