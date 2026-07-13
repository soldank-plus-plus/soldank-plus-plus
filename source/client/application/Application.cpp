module;

#include <GLFW/glfw3.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

export module Application;

import Extern.Glm;

import Application.CLI.CommandLineParameters;
import Application.ClientModes;
import Application.Window;
import Application.Input.ApplicationInputController;
import Application.Input.PlatformInput;
import Application.Platform.ClientTransportStartup;
import Application.Platform.WebAssemblyStartupAdapter;
import Runtime.ClientRuntime;
import Gameplay.GameSession;
import Gameplay.PlayerController;
import Editor.EditorSession;
import Scene;
import RenderPipeline;
import MapEditor;
import ClientState;

import Networking.NetworkingClient;
import Networking.INetworkingClient;
import Networking.NetworkClientSession;
import Networking.AssignPlayerIdNetworkEventHandler;
import Networking.PingCheckNetworkEventHandler;
import Networking.PlayerLeaveNetworkEventHandler;
import Networking.ProjectileSpawnNetworkEventHandler;
import Networking.SoldierInfoNetworkEventHandler;
import Networking.SoldierStateNetworkEventHandler;
import Networking.SpawnSoldierNetworkEventHandler;
import Networking.KillSoldierNetworkEventHandler;
import Networking.HitSoldierNetworkEventHandler;

import Shared.Core.IWorld;
import Shared.Core.World;
import Shared.CoreEventHandler;

import Shared.Core.State.StateManager;
import Shared.Core.Entities.Bullet;
import Shared.Core.Entities.Soldier;
import Shared.Core.Map.PMSStructs;
import Shared.Core.Entities.Item;

import Shared.Networking.NetworkPackets;
import Shared.Networking.NetworkEventDispatcher;
import Shared.Networking.NetworkEvent;
import Shared.Networking.DeliveryMode;

import Extern.Spdlog;
import Extern.SimpleIni;

export namespace Soldank
{
class Application
{
public:
    Application(const std::vector<const char*>& cli_parameters);
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(Application other) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&& other) = delete;

    void Run();

private:
    std::unique_ptr<Window> window_;
    std::shared_ptr<IWorld> world_;
    std::unique_ptr<INetworkingClient> networking_client_;
    std::shared_ptr<ClientState> client_state_;
    std::shared_ptr<NetworkEventDispatcher> client_network_event_dispatcher_;
    std::unique_ptr<MapEditor> map_editor_;
    std::unique_ptr<EditorSession> editor_session_;
    std::unique_ptr<NetworkClientSession> network_client_session_;
    std::unique_ptr<RenderPipeline> render_pipeline_;
    std::unique_ptr<ApplicationInputController> input_controller_;
    std::unique_ptr<PlayerController> player_controller_;

    CommandLineParameters::ApplicationMode application_mode_;
    WindowSizeMode window_size_mode_;
    int fps_limit_ = 0;
    ClientRuntime client_runtime_;
    GameSession game_session_;
    ClientTransportStartup client_transport_startup_;
};
} // namespace Soldank

namespace Soldank
{
Application::Application(const std::vector<const char*>& cli_parameters)
{
    Spdlog::set_level(Spdlog::level::debug);

    SimpleIni::CSimpleIniA ini_config;
    SimpleIni::SI_Error rc = ini_config.LoadFile("debug_config.ini");
    std::string server_ip;
    int server_port = 0;
    if (rc < 0) {
        Spdlog::warn("INI File could not be loaded: debug_config.ini");
        application_mode_ = CommandLineParameters::ApplicationMode::Local;
    } else {
        application_mode_ = ini_config.GetBoolValue("Network", "Online")
                              ? CommandLineParameters::ApplicationMode::Online
                              : CommandLineParameters::ApplicationMode::Local;
        if (application_mode_ == CommandLineParameters::ApplicationMode::Online) {
            Spdlog::info("Online = true");
            const auto* ip = ini_config.GetValue("Network", "Server_IP");
            int port = ini_config.GetLongValue("Network", "Server_Port");
            if (ip == nullptr || port == 0) {
                Spdlog::warn("Server_IP or Server_Port not set, setting is_online to false");
                application_mode_ = CommandLineParameters::ApplicationMode::Local;
            } else {
                server_ip = ip;
                server_port = port;
                Spdlog::debug("Server_ip = {} server_port = {}", server_ip, server_port);
            }
        } else {
            Spdlog::info("Online = false");
        }
    }

    CommandLineParameters::ParsedValues parsed_cli_parameters =
      CommandLineParameters::Parse(cli_parameters);
    if (!parsed_cli_parameters.is_parsing_successful) {
        exit(1);
    }

    if (auto url_server_endpoint = WebAssemblyStartupAdapter::GetServerEndpointFromUrl()) {
        application_mode_ = CommandLineParameters::ApplicationMode::Online;
        server_ip = url_server_endpoint->ip;
        server_port = url_server_endpoint->port;
        Spdlog::info("Using server endpoint from URL: {}:{}", server_ip, server_port);
    }

    window_size_mode_ = parsed_cli_parameters.window_size_mode;
    fps_limit_ = parsed_cli_parameters.fps_limit;

    std::string map_path;

    // If specified then override the application mode that was set from the INI file
    if (parsed_cli_parameters.application_mode != CommandLineParameters::ApplicationMode::Default) {
        application_mode_ = parsed_cli_parameters.application_mode;
    }

    window_ = std::make_unique<Window>();
    world_ = std::make_shared<World>();
    client_state_ = std::make_shared<ClientState>();
    input_controller_ = std::make_unique<ApplicationInputController>(
      *window_, *world_, *client_state_, client_runtime_, game_session_);
    player_controller_ =
      std::make_unique<PlayerController>(*world_, *window_, client_state_, *input_controller_);
    client_state_->debug_render.is_game_debug_interface_enabled =
      parsed_cli_parameters.is_debug_ui_enabled;

    switch (application_mode_) {
        case CommandLineParameters::ApplicationMode::Default: {
            Spdlog::critical("Application mode = Default. That should have never happened.");
            std::unreachable();
            break;
        }
        case CommandLineParameters::ApplicationMode::Local: {
            Spdlog::info("Application mode = Local");
            client_runtime_.SetClientMode(ClientMode::LocalGame);
            map_path = "maps/ctf_Ash.pms";
            break;
        }
        case CommandLineParameters::ApplicationMode::Online: {
            if (parsed_cli_parameters.application_mode !=
                CommandLineParameters::ApplicationMode::Default) {
                server_ip = parsed_cli_parameters.join_server_ip;
                server_port = parsed_cli_parameters.join_server_port;
            }
            client_runtime_.SetClientMode(ClientMode::OnlineGame);
            map_path = "maps/ctf_Ash.pms";
            client_state_->network.draw_server_pov_client_pos = true;
            Spdlog::info("Application mode = Online");
            break;
        }
        case CommandLineParameters::ApplicationMode::MapEditor: {
            client_runtime_.SetClientMode(ClientMode::MapEditor);
            client_runtime_.SetEditorMode(EditorMode::Edit);
            map_editor_ = std::make_unique<MapEditor>(*client_state_, *world_->GetStateManager());
            Spdlog::info("Application mode = MapEditor");
            break;
        }
    }

    if (parsed_cli_parameters.map) {
        map_path = "maps/" + *parsed_cli_parameters.map + ".pms";
    }
    Spdlog::debug("{} Map: {}", map_path.empty(), map_path);

    if (map_path.empty()) {
        world_->GetStateManager()->CreateEmptyMapDocument();
    } else {
        world_->GetStateManager()->LoadMapDocument(map_path);
    }

    if (application_mode_ == CommandLineParameters::ApplicationMode::Online) {
        Spdlog::info("Connecting to {}:{}", server_ip, server_port);
        std::vector<std::shared_ptr<INetworkEventHandler>> network_event_handlers{
            std::make_shared<AssignPlayerIdNetworkEventHandler>(world_, client_state_),
            std::make_shared<PingCheckNetworkEventHandler>(client_state_),
            std::make_shared<PlayerLeaveNetworkEventHandler>(world_),
            std::make_shared<ProjectileSpawnNetworkEventHandler>(world_),
            std::make_shared<SoldierInfoNetworkEventHandler>(world_, client_state_),
            std::make_shared<SoldierStateNetworkEventHandler>(world_, client_state_),
            std::make_shared<SpawnSoldierNetworkEventHandler>(world_),
            std::make_shared<KillSoldierNetworkEventHandler>(world_),
            std::make_shared<HitSoldierNetworkEventHandler>(world_)
        };
        client_network_event_dispatcher_ =
          std::make_shared<NetworkEventDispatcher>(network_event_handlers);

        client_transport_startup_.Initialize();
        networking_client_ = std::make_unique<NetworkingClient>(server_ip.c_str(), server_port);
        network_client_session_ = std::make_unique<NetworkClientSession>(
          *networking_client_, client_network_event_dispatcher_, *world_, *client_state_);
    } else {
        CoreEventHandler::ObserveAll(world_.get());
    }
}

Application::~Application()
{
    window_.reset(nullptr);
    networking_client_.reset(nullptr);

    if (application_mode_ == CommandLineParameters::ApplicationMode::Online) {
        client_transport_startup_.Shutdown();
    }
}

void Application::Run()
{
    input_controller_->ConfigureHandlers();

    world_->GetPhysicsEvents().soldier_collides_with_polygon.AddObserver(
      [&](const Soldier& /*soldier*/, const PMSPolygon& polygon) {
          if (client_state_->debug_render.draw_colliding_polygons) {
              bool exists = false;
              for (unsigned int poly_id : client_state_->debug_render.colliding_polygon_ids) {
                  if (polygon.id == poly_id) {
                      exists = true;
                      break;
                  }
              }
              if (!exists) {
                  client_state_->debug_render.colliding_polygon_ids.push_back(polygon.id);
              }
          }
      });
    world_->GetPhysicsEvents().item_collides_with_polygon.AddObserver(
      [&](Item& /*item*/, const PMSPolygon& polygon) {
          if (client_state_->debug_render.draw_colliding_polygons) {
              bool exists = false;
              for (unsigned int poly_id : client_state_->debug_render.colliding_polygon_ids) {
                  if (polygon.id == poly_id) {
                      exists = true;
                      break;
                  }
              }
              if (!exists) {
                  client_state_->debug_render.colliding_polygon_ids.push_back(polygon.id);
              }
          }
      });

    window_->Create(window_size_mode_);
    input_controller_->ResetMousePositions();
    input_controller_->UpdateWindowSize();
    client_state_->camera.view.UpdateWindowDimensions(
      { client_state_->input.window_width, client_state_->input.window_height });

    client_state_->event_respawn_player_at_spawn_point_requested.AddObserver(
      [&](unsigned int spawn_point_id) {
          const auto& spawn_point =
            world_->GetStateManager()->GetMap().GetSpawnPoints().at(spawn_point_id);
          glm::vec2 position = { spawn_point.x, spawn_point.y };
          world_->SpawnSoldier(*client_state_->client_soldier_id, position);
      });
    client_state_->event_respawn_soldier_requested.AddObserver(
      [&](unsigned int soldier_id) { world_->SpawnSoldier(soldier_id); });

    window_->RegisterOnScreenResizedObserver([&](glm::vec2 new_window_dimensions) {
        client_state_->camera.view.UpdateWindowDimensions(new_window_dimensions);
        client_state_->event_window_resized.Notify(new_window_dimensions);
    });

    // Set lower FPS limit when the window is not focused to free up the CPU
    window_->RegisterOnFocusGainObserver([&]() { world_->SetFPSLimit(fps_limit_); });
    window_->RegisterOnFocusLossObserver([&]() { world_->SetFPSLimit(60); });

    if (application_mode_ == CommandLineParameters::ApplicationMode::MapEditor) {
        window_->SetCursorMode(CursorMode::Normal);
        world_->GetStateManager()->PauseGame();
        editor_session_ =
          std::make_unique<EditorSession>(*client_state_, *world_, *window_, *map_editor_);
    }

    client_state_->map_editor_state.event_pressed_play.AddObserver([&]() {
        if (editor_session_) {
            editor_session_->StartPlayTest();
            client_runtime_.SetEditorMode(editor_session_->GetEditorMode());
        }
    });

    client_state_->event_key_pressed.AddObserver([&](int key) {
        if (key == GLFW_KEY_F5 &&
            application_mode_ == CommandLineParameters::ApplicationMode::MapEditor) {
            if (editor_session_) {
                editor_session_->TogglePlayTest();
                client_runtime_.SetEditorMode(editor_session_->GetEditorMode());
            }
        }
    });

    render_pipeline_ = std::make_unique<RenderPipeline>(world_->GetStateManager(), *client_state_);

    client_state_->map_editor_state.event_pixel_color_under_cursor_requested.AddObserver([&]() {
        glm::vec2 mouse_position = window_->GetCursorScreenPosition();
        // glReadPixels requires inverted Y axis for some reason...
        mouse_position.y = (float)window_->GetWindowSize().y - mouse_position.y;
        client_state_->map_editor_state.last_requested_pixel_color =
          Scene::GetPixelColor(mouse_position);
    });

    world_->SetShouldStopGameLoopCallback([&]() { return window_->ShouldClose(); });
    world_->SetPreGameLoopIterationCallback([&]() {
        input_controller_->UpdateContext();
        input_controller_->Route();

        const PlatformInput& input = window_->GetPlatformInput();
        if (input.KeyWentDown(GLFW_KEY_ESCAPE)) {
            window_->Close();
        }
        input_controller_->UpdateWindowSize();

        glm::vec2 mouse_screen_position = input_controller_->GetMouseScreenPosition();

        bool is_gameplay_active = game_session_.IsGameplayActive(client_runtime_.GetClientMode(),
                                                                 client_runtime_.GetEditorMode());
        if (is_gameplay_active) {
            if (!input_controller_->IsUiCaptured() && input.KeyWentDown(GLFW_KEY_F10)) {
                world_->GetStateManager()->TogglePauseGame();
            }

            if (world_->GetStateManager()->IsGamePaused()) {
                glm::vec2 mouse_position = { input.GetX(), input.GetY() };

                client_state_->camera.previous_position = client_state_->camera.position;

                client_state_->input.mouse_screen_position.x = mouse_position.x;
                client_state_->input.mouse_screen_position.y = mouse_position.y;
            }
        } else if (client_runtime_.GetClientMode() == ClientMode::MapEditor) {
            client_state_->camera.previous_position = client_state_->camera.position;

            client_state_->input.mouse_screen_position.x = mouse_screen_position.x;
            client_state_->input.mouse_screen_position.y = mouse_screen_position.y;
        }
    });
    world_->SetPreWorldUpdateCallback([&]() {
        client_state_->debug_render.colliding_polygon_ids.clear();

        if (application_mode_ == CommandLineParameters::ApplicationMode::Online) {
            network_client_session_->UpdateBeforeWorldTick();
        }

        if (client_state_->client_soldier_id.has_value()) {
            std::uint8_t client_soldier_id = *client_state_->client_soldier_id;
            player_controller_->Update(client_soldier_id);

            if (application_mode_ == CommandLineParameters::ApplicationMode::Online) {
                glm::vec2 mouse_map_position = input_controller_->GetMouseMapPosition();
                network_client_session_->SendSoldierInput(client_soldier_id, mouse_map_position);

                if (client_state_->kill_button_just_pressed) {
                    client_state_->kill_button_just_pressed = false;
                    network_client_session_->SendKillCommand();
                }
            } else {
                if (client_state_->kill_button_just_pressed) {
                    client_state_->kill_button_just_pressed = false;
                    world_->KillSoldier(client_soldier_id);
                }
            }
        } else {
            client_state_->camera.position = { 0.0F, 0.0F };
        }
    });
    world_->SetPostWorldUpdateCallback([&](const StateManager& /*state_manager*/) {});
    world_->SetPostGameLoopIterationCallback(
      [&](const StateManager& state_manager, double frame_percent, int last_fps) {
          if (!client_state_->network.objects_interpolation) {
              frame_percent = 1.0F;
          }
          render_pipeline_->Render(state_manager,
                                   *client_state_,
                                   client_runtime_.GetClientMode(),
                                   client_runtime_.GetEditorMode(),
                                   frame_percent,
                                   last_fps);

          window_->SwapBuffers();
          window_->GetPlatformInput().ResetFrame();
          window_->PollInput();
      });

    world_->SetPreSoldierUpdateCallback([&](const Soldier& soldier) {
        if (application_mode_ != CommandLineParameters::ApplicationMode::Online) {
            return true;
        }

        if (client_state_->client_soldier_id.has_value()) {
            if (*client_state_->client_soldier_id == soldier.id &&
                client_state_->network.client_side_prediction) {
                return true;
            }
        }

        return false;
    });
    world_->SetPreProjectileSpawnCallback([&](const BulletParams& /*bullet_params*/) {
        return application_mode_ != CommandLineParameters::ApplicationMode::Online;
    });

    if (application_mode_ == CommandLineParameters::ApplicationMode::Local ||
        application_mode_ == CommandLineParameters::ApplicationMode::MapEditor) {
        const auto& soldier = world_->CreateSoldier();
        client_state_->client_soldier_id = soldier.id;
    }

    if (application_mode_ == CommandLineParameters::ApplicationMode::Local) {
        world_->SpawnSoldier(*client_state_->client_soldier_id);
    }

    world_->SetFPSLimit(fps_limit_);
    world_->RunLoop();
}

} // namespace Soldank
