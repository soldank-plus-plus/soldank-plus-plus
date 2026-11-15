module;

#include <chrono>
#include <memory>
#include <thread>
#include <utility>
#include <vector>
#include <optional>
#include <string>
#include <cstdint>

export module Runtime.ServerRuntime;

import Application.ServerConfig;
import Replication.ReplicationService;
import Runtime.ServerCommandQueues;
import Runtime.ServerRuntimeServices;
import Runtime.ServerSimulationEventRouter;
import Sessions.PlayerSessionManager;

import Shared.Core.IWorld;
import Shared.Core.GameModes.GameModeRuntime;
import Shared.Core.Entities.Bullet;
import Shared.Core.Loop.FixedTimestepRunner;
import Shared.Core.Loop.NativeFixedTimestepLoop;
import Shared.Core.Simulation.SimulationCommands;
import Shared.Core.Simulation.WorldTick;

import Extern.Spdlog;

export namespace Soldank
{
class ServerRuntime
{
public:
    ServerRuntime(ServerConfig config,
                  const std::shared_ptr<IWorld>& world,
                  IServerNetworkHost& network_host,
                  ILobbyRegistrationClient& lobby_client,
                  PlayerSessionManager& player_session_manager,
                  ServerCommandQueues& command_queues)
        : config_(std::move(config))
        , world_(world)
        , network_host_(network_host)
        , lobby_client_(lobby_client)
        , player_session_manager_(player_session_manager)
        , command_queues_(command_queues)
        , replication_service_(network_host_, player_session_manager_)
    {
        simulation_event_router_.AddSink(replication_service_);
        lobby_client_.SetGameMode(config_.game_mode, config_.survival);
    }

    void Run()
    {
        Spdlog::info("Server started!");
        world_->SetPreProjectileSpawnCallback(
          [](const BulletParams& /*bullet_params*/) { return true; });

        FixedTimestepRunner fixed_timestep_runner;
        NativeFixedTimestepLoop native_loop;
        FixedTimestepCallbacks callbacks{
            .should_continue = []() { return true; },
            .before_frame = []() {},
            .should_tick = []() { return true; },
            .tick =
              [&](double /*delta_time*/) {
                  AdvanceMatch();
                  network_host_.Update();

                  const std::uint32_t server_tick = world_->GetStateManager()->GetGameTick();
                  auto player_inputs = command_queues_.SelectPlayerInputsForSimulation(server_tick);
                  auto simulation_commands = command_queues_.DrainSimulationCommands();
                  const WorldTickInput input{
                      .tick = server_tick,
                      .player_inputs = player_inputs,
                      .commands = simulation_commands,
                  };
                  WorldTickResult result = world_->Tick(input);
                  for (const auto& player_input : player_inputs) {
                      player_session_manager_.MarkInputApplied(player_input.soldier_id,
                                                               player_input.input_sequence_id);
                  }
#ifndef NDEBUG
                  command_queues_.ResetInputDebugStats();
#endif
                  simulation_event_router_.OnSimulationEvents(result.events);
                  replication_service_.BroadcastTick(*world_->GetStateManager());

                  if (world_->GetStateManager()->GetGameTick() % (3600 * 3) == 0) {
                      lobby_client_.Register(config_.server_name, config_.server_port);
                  }
              },
            .after_tick =
              [&](unsigned int next_game_tick) {
                  world_->GetStateManager()->SetGameTick(next_game_tick);
              },
            .render = [](double /*frame_percent*/, int /*last_fps*/) {},
            .report_stats =
              [](int frame_count_since_last_fps_check, int world_updates) {
                  Spdlog::info("{} ms/frame",
                               1000.0 / static_cast<double>(frame_count_since_last_fps_check));
                  Spdlog::info("FPS: {}", frame_count_since_last_fps_check);
                  Spdlog::info("World updates: {}", world_updates);
              },
        };

        native_loop.Run(fixed_timestep_runner, callbacks, [&]() { return config_.fps_limit; });

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

private:
    void AdvanceMatch()
    {
        auto state = world_->GetStateManager();
        const auto* mode = state->GetGameMode();
        if (!mode || mode->GetState().phase != MatchPhase::Finished) {
            finished_tick_.reset();
            return;
        }
        if (!finished_tick_) {
            finished_tick_ = state->GetGameTick();
        }
        if (state->GetGameTick() - *finished_tick_ < 300U) {
            return;
        }
        std::string map = config_.map_path;
        if (!config_.map_rotation.empty()) {
            map_index_ = (map_index_ + 1) % config_.map_rotation.size();
            map = config_.map_rotation.at(map_index_);
        }
        state->LoadMapDocument(map);
        command_queues_.ClearPendingCommands();
        state->TransformSoldiers([&](auto& soldier) {
            if (!soldier.IsSpectator()) {
                world_->SpawnSoldier(soldier.id);
            }
        });
        finished_tick_.reset();
    }
    std::optional<std::uint32_t> finished_tick_;
    std::size_t map_index_{};
    ServerConfig config_;
    std::shared_ptr<IWorld> world_;
    IServerNetworkHost& network_host_;
    ILobbyRegistrationClient& lobby_client_;
    PlayerSessionManager& player_session_manager_;
    ServerCommandQueues& command_queues_;
    ReplicationService replication_service_;
    ServerSimulationEventRouter simulation_event_router_;
};
} // namespace Soldank
