module;

#include <algorithm>
#include <chrono>
#include <fstream>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

export module Runtime.ServerRuntime;

import Application.ServerConfig;
import Replication.ReplicationService;
import Runtime.ServerCommandQueues;
import Runtime.ServerRuntimeServices;
import Runtime.ServerSimulationEventRouter;
import Sessions.PlayerSessionManager;

import Shared.Core.IWorld;
import Shared.Core.Entities.Bullet;
import Shared.Core.Entities.Soldier;
import Shared.Core.Loop.FixedTimestepRunner;
import Shared.Core.Loop.NativeFixedTimestepLoop;
import Shared.Core.Simulation.SimulationCommands;
import Shared.Core.Simulation.WorldTick;
import Shared.Core.State.Control;
import Shared.Core.State.StateManager;

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
#ifndef NDEBUG
        WriteServerTimelineLogHeader();
#endif
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
                  const auto input_debug_stats = command_queues_.GetInputDebugStats();
                  Spdlog::debug("Input tick {}: received {}, selected {}, late {}, superseded {}",
                                input.tick,
                                input_debug_stats.received_input_count,
                                player_inputs.size(),
                                input_debug_stats.late_applied_input_count,
                                input_debug_stats.superseded_input_count);
                  for (const auto& player_input : player_inputs) {
                      Spdlog::debug(
                        "Input tick {} soldier {}: selected {}, received {}, applied {}",
                        input.tick,
                        player_input.soldier_id,
                        player_input.input_sequence_id,
                        player_session_manager_.GetLastReceivedInputId(player_input.soldier_id),
                        player_session_manager_.GetLastAppliedInputId(player_input.soldier_id));
                  }
                  LogServerTimelineTick(input.tick, player_inputs, input_debug_stats);
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
#ifndef NDEBUG
    void WriteServerTimelineLogHeader()
    {
        server_timeline_log_
          << "server_tick,soldier_id,received_input_count,selected_input,selected_input_id,"
             "selected_client_tick,selected_apply_server_tick,last_received_input_id,"
             "last_applied_input_id,late_input_count,superseded_input_count,input_left,"
             "input_right,input_up,input_down,input_fire,input_jets,input_change,"
             "input_throw_grenade,input_drop,input_reload,input_prone,input_flag_throw,"
             "input_mouse_aim_x,input_mouse_aim_y,input_mouse_dist,input_was_running_left,"
             "input_was_jumping,input_was_throwing_weapon,input_was_changing_weapon,"
             "input_was_throwing_grenade,input_was_reloading_weapon,position_x,position_y,"
             "old_position_x,old_position_y,velocity_x,velocity_y,force_x,force_y,on_ground,"
             "on_ground_for_law,on_ground_last_frame,on_ground_permanent,stance,direction,"
             "old_direction,jets_count,jets_count_prev,grenade_can_throw,active_weapon,"
             "body_animation_type,body_animation_frame,body_animation_speed,"
             "body_animation_count,legs_animation_type,legs_animation_frame,"
             "legs_animation_speed,legs_animation_count,post_control_left,post_control_right,"
             "post_control_up,post_control_down,post_control_fire,post_control_jets,"
             "post_control_change,post_control_throw_grenade,post_control_drop,"
             "post_control_reload,post_control_prone,post_control_flag_throw,"
             "post_control_mouse_aim_x,post_control_mouse_aim_y,post_control_mouse_dist,"
             "post_control_was_running_left,post_control_was_jumping,"
             "post_control_was_throwing_weapon,post_control_was_changing_weapon,"
             "post_control_was_throwing_grenade,post_control_was_reloading_weapon\n";
        server_timeline_log_.flush();
    }

    void LogServerTimelineTick(std::uint32_t server_tick,
                               const std::vector<PlayerInputCommand>& player_inputs,
                               const ServerInputQueueDebugStats& input_debug_stats)
    {
        world_->GetStateManager()->ForEachSoldier([&](const Soldier& soldier) {
            const auto selected_input =
              std::find_if(player_inputs.begin(), player_inputs.end(), [&](const auto& input) {
                  return input.soldier_id == soldier.id;
              });
            const bool has_selected_input = selected_input != player_inputs.end();
            const Control* input_control = has_selected_input ? &selected_input->control : nullptr;
            const Control& post_control = soldier.control;

            server_timeline_log_
              << server_tick << ',' << static_cast<int>(soldier.id) << ','
              << input_debug_stats.received_input_count << ',' << has_selected_input << ','
              << (has_selected_input ? selected_input->input_sequence_id : 0) << ','
              << (has_selected_input ? selected_input->client_tick : 0) << ','
              << (has_selected_input ? selected_input->apply_server_tick : 0) << ','
              << player_session_manager_.GetLastReceivedInputId(soldier.id) << ','
              << player_session_manager_.GetLastAppliedInputId(soldier.id) << ','
              << input_debug_stats.late_applied_input_count << ','
              << input_debug_stats.superseded_input_count << ','
              << (input_control != nullptr && input_control->left) << ','
              << (input_control != nullptr && input_control->right) << ','
              << (input_control != nullptr && input_control->up) << ','
              << (input_control != nullptr && input_control->down) << ','
              << (input_control != nullptr && input_control->fire) << ','
              << (input_control != nullptr && input_control->jets) << ','
              << (input_control != nullptr && input_control->change) << ','
              << (input_control != nullptr && input_control->throw_grenade) << ','
              << (input_control != nullptr && input_control->drop) << ','
              << (input_control != nullptr && input_control->reload) << ','
              << (input_control != nullptr && input_control->prone) << ','
              << (input_control != nullptr && input_control->flag_throw) << ','
              << (input_control != nullptr ? input_control->mouse_aim_x : 0) << ','
              << (input_control != nullptr ? input_control->mouse_aim_y : 0) << ','
              << (input_control != nullptr ? input_control->mouse_dist : 0) << ','
              << (input_control != nullptr && input_control->was_running_left) << ','
              << (input_control != nullptr && input_control->was_jumping) << ','
              << (input_control != nullptr && input_control->was_throwing_weapon) << ','
              << (input_control != nullptr && input_control->was_changing_weapon) << ','
              << (input_control != nullptr && input_control->was_throwing_grenade) << ','
              << (input_control != nullptr && input_control->was_reloading_weapon) << ','
              << soldier.particle.position.x << ',' << soldier.particle.position.y << ','
              << soldier.particle.old_position.x << ',' << soldier.particle.old_position.y << ','
              << soldier.particle.GetVelocity().x << ',' << soldier.particle.GetVelocity().y << ','
              << soldier.particle.GetForce().x << ',' << soldier.particle.GetForce().y << ','
              << soldier.on_ground << ',' << soldier.on_ground_for_law << ','
              << soldier.on_ground_last_frame << ',' << soldier.on_ground_permanent << ','
              << static_cast<int>(soldier.stance) << ',' << static_cast<int>(soldier.direction)
              << ',' << static_cast<int>(soldier.old_direction) << ',' << soldier.jets_count << ','
              << soldier.jets_count_prev << ',' << soldier.grenade_can_throw << ','
              << static_cast<int>(soldier.active_weapon) << ','
              << static_cast<int>(soldier.body_animation->GetType()) << ','
              << soldier.body_animation->GetFrame() << ',' << soldier.body_animation->GetSpeed()
              << ',' << soldier.body_animation->GetCount() << ','
              << static_cast<int>(soldier.legs_animation->GetType()) << ','
              << soldier.legs_animation->GetFrame() << ',' << soldier.legs_animation->GetSpeed()
              << ',' << soldier.legs_animation->GetCount() << ',' << post_control.left << ','
              << post_control.right << ',' << post_control.up << ',' << post_control.down << ','
              << post_control.fire << ',' << post_control.jets << ',' << post_control.change << ','
              << post_control.throw_grenade << ',' << post_control.drop << ','
              << post_control.reload << ',' << post_control.prone << ',' << post_control.flag_throw
              << ',' << post_control.mouse_aim_x << ',' << post_control.mouse_aim_y << ','
              << post_control.mouse_dist << ',' << post_control.was_running_left << ','
              << post_control.was_jumping << ',' << post_control.was_throwing_weapon << ','
              << post_control.was_changing_weapon << ',' << post_control.was_throwing_grenade << ','
              << post_control.was_reloading_weapon << '\n';
        });
        server_timeline_log_.flush();
    }
#endif

    ServerConfig config_;
    std::shared_ptr<IWorld> world_;
    IServerNetworkHost& network_host_;
    ILobbyRegistrationClient& lobby_client_;
    PlayerSessionManager& player_session_manager_;
    ServerCommandQueues& command_queues_;
    ReplicationService replication_service_;
    ServerSimulationEventRouter simulation_event_router_;
#ifndef NDEBUG
    std::ofstream server_timeline_log_{ "network-reconciliation-server.csv", std::ios::trunc };
#endif
};
} // namespace Soldank
