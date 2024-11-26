#include "application/Application.hpp"

#include "communication/NetworkEventDispatcher.hpp"
#include "communication/NetworkPackets.hpp"
#include "networking/event_handlers/PingCheckNetworkEventHandler.hpp"
#include "networking/event_handlers/SoldierInputNetworkEventHandler.hpp"
#include "networking/event_handlers/KillCommandNetworkEventHandler.hpp"
#include "networking/CoreEventsConnectionNotifier.hpp"

#include "core/CoreEventHandler.hpp"

#include "scripting/dascript/DaScriptInit.hpp"
#include "scripting/dascript/DaScriptScriptingEngine.hpp"

#include "spdlog/spdlog.h"

#include <spdlog/common.h>
#include <steam/steamnetworkingsockets.h>
#include <SimpleIni.h>

#include <span>
#include <cstdlib>

namespace Soldank
{
SteamNetworkingMicroseconds Application::log_time_zero_ = 0;

void Application::DebugOutput(ESteamNetworkingSocketsDebugOutputType output_type,
                              const char* message)
{
    SteamNetworkingMicroseconds time = SteamNetworkingUtils()->GetLocalTimestamp() - log_time_zero_;
    spdlog::info("{} {}", (double)time * 1e-6, message);
    fflush(stdout);
    if (output_type == k_ESteamNetworkingSocketsDebugOutputType_Bug) {
        exit(1);
    }
}

Application::Application()
    : world_(std::make_shared<World>())
    , lobby_client_(std::make_shared<LobbyClient>())
{
    world_->GetStateManager()->GetState().map.LoadMap("maps/ctf_Ash.pms");
    spdlog::set_level(spdlog::level::debug);

    bool da_script_initialized = InitDaScriptModule();
    if (da_script_initialized) {
        spdlog::info("daScript module initialized");

        scripting_engine_ = std::make_shared<DaScriptScriptingEngine>();
    } else {
        spdlog::error("Could not initialize daScript module");
    }

    server_state_ = std::make_shared<ServerState>();

    CSimpleIniA ini_config;
    SI_Error rc = ini_config.LoadFile("soldat.ini");
    if (rc < 0) {
        spdlog::critical("Error: INI File could not be loaded: soldat.ini");
        exit(1);
    } else {
        server_state_->server_port = ini_config.GetLongValue("NETWORK", "Port");
        if (server_state_->server_port == 0) {
            spdlog::critical("Error: Port can't be 0");
            exit(1);
        }

        const char* server_name_cstr = ini_config.GetValue("NETWORK", "Server_Name");
        if (server_name_cstr == nullptr) {
            spdlog::critical("Error: Port can't be 0");
            exit(1);
        }
        server_state_->server_name = server_name_cstr;
    }

    SteamDatagramErrMsg err_msg;
    if (!GameNetworkingSockets_Init(nullptr, err_msg)) {
        spdlog::error("GameNetworkingSockets_Init failed. {}", std::span(err_msg).data());
    }

    log_time_zero_ = SteamNetworkingUtils()->GetLocalTimestamp();

    SteamNetworkingUtils()->SetDebugOutputFunction(k_ESteamNetworkingSocketsDebugOutputType_Msg,
                                                   DebugOutput);

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

Application::~Application()
{
    ShutdownDaScriptModule();
    GameNetworkingSockets_Kill();
}

void Application::Run()
{
    spdlog::info("Server started!");

    world_->SetShouldStopGameLoopCallback([&]() { return false; });
    world_->SetPreGameLoopIterationCallback([&]() {});
    world_->SetPreWorldUpdateCallback([&]() { game_server_->Update(); });
    world_->SetPostWorldUpdateCallback([&](const State& state) {
        for (const auto& soldier : state.soldiers) {
            SoldierStatePacket update_soldier_state_packet{
                .game_tick = state.game_tick,
                .player_id = soldier.id,
                .position_x = soldier.particle.position.x,
                .position_y = soldier.particle.position.y,
                .old_position_x = soldier.particle.old_position.x,
                .old_position_y = soldier.particle.old_position.y,
                .body_animation_type = soldier.body_animation->GetType(),
                .body_animation_frame = soldier.body_animation->GetFrame(),
                .body_animation_speed = soldier.body_animation->GetSpeed(),
                .legs_animation_type = soldier.legs_animation->GetType(),
                .legs_animation_frame = soldier.legs_animation->GetFrame(),
                .legs_animation_speed = soldier.legs_animation->GetSpeed(),
                .velocity_x = soldier.particle.GetVelocity().x,
                .velocity_y = soldier.particle.GetVelocity().y,
                .force_x = soldier.particle.GetForce().x,
                .force_y = soldier.particle.GetForce().y,
                .on_ground = soldier.on_ground,
                .on_ground_for_law = soldier.on_ground_for_law,
                .on_ground_last_frame = soldier.on_ground_last_frame,
                .on_ground_permanent = soldier.on_ground_permanent,
                .stance = soldier.stance,
                .mouse_position_x = soldier.mouse.x,
                .mouse_position_y = 480.0F - soldier.mouse.y, // TODO: is it correct?
                .using_jets = soldier.control.jets,
                .jets_count = soldier.jets_count,
                .active_weapon = soldier.active_weapon,
                .last_processed_input_id = server_state_->last_processed_input_id.at(soldier.id)
            };
            game_server_->SendNetworkMessageToAll(
              { NetworkEvent::SoldierState, update_soldier_state_packet });
        }

        // Re-register the server in the lobby every 3 minutes
        if (state.game_tick % (3600 * 3) == 0) {
            lobby_client_->Register(server_state_->server_name, server_state_->server_port);
        }

        // for (const auto& soldier : state->soldiers) {
        //     if (soldier.active) {
        //         spdlog::info("{}, Player {} pos: {}, {}; velocity: {} {}; force: {} {}",
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
    world_->SetPostGameLoopIterationCallback(
      [&](const State& state, double frame_percent, int last_fps) {});
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

    world_->SetFPSLimit(64);
    world_->RunLoop();

    // Give connections time to finish up.  This is an application layer protocol
    // here, it's not TCP.  Note that if you have an application and you need to be
    // more sure about cleanup, you won't be able to do this.  You will need to send
    // a message and then either wait for the peer to close the connection, or
    // you can pool the connection to see if any reliable data is pending.
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}
} // namespace Soldank
