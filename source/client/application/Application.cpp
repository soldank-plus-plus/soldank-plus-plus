module;

#include "core/math/Glm.hpp"

#include <GLFW/glfw3.h>

#include <cstdint>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

export module Application;

import Application.CLI.CommandLineParameters;
import Application.Window;
import Application.Input.InputSnapshot;
import Application.Input.InputRouter;
import Application.Input.PlatformInput;
import Application.Input.PlayerInput;
import Application.Platform.ClientTransportStartup;
import Application.Platform.WebAssemblyStartupAdapter;
import Scene;
import MapEditor;
import ClientState;
import DebugUI;

import Networking.NetworkingClient;
import Networking.INetworkingClient;
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
import Shared.Core.State.Control;
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
    glm::vec2 GetCurrentMouseScreenPosition();
    glm::vec2 GetCurrentMouseMapPosition();
    void UpdateWindowSize();
    void RouteInput();
    void UpdateInputContext();

    std::unique_ptr<Window> window_;
    std::shared_ptr<IWorld> world_;
    std::unique_ptr<INetworkingClient> networking_client_;
    std::shared_ptr<ClientState> client_state_;
    std::shared_ptr<NetworkEventDispatcher> client_network_event_dispatcher_;
    std::unique_ptr<MapEditor> map_editor_;
    std::unique_ptr<Scene> scene_;

    CommandLineParameters::ApplicationMode application_mode_;
    WindowSizeMode window_size_mode_;

    int fps_limit_ = 0;
    unsigned int input_sequence_id_ = 1;
    InputRouter input_router_;
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
    client_state_->is_game_debug_interface_enabled = parsed_cli_parameters.is_debug_ui_enabled;

    switch (application_mode_) {
        case CommandLineParameters::ApplicationMode::Default: {
            Spdlog::critical("Application mode = Default. That should have never happened.");
            std::unreachable();
            break;
        }
        case CommandLineParameters::ApplicationMode::Local: {
            Spdlog::info("Application mode = Local");
            map_path = "maps/ctf_Ash.pms";
            client_state_->draw_game_debug_interface = true;
            client_state_->draw_game_interface = true;
            break;
        }
        case CommandLineParameters::ApplicationMode::Online: {
            if (parsed_cli_parameters.application_mode !=
                CommandLineParameters::ApplicationMode::Default) {
                server_ip = parsed_cli_parameters.join_server_ip;
                server_port = parsed_cli_parameters.join_server_port;
            }
            map_path = "maps/ctf_Ash.pms";
            client_state_->draw_game_debug_interface = true;
            client_state_->draw_server_pov_client_pos = true;
            client_state_->draw_game_interface = true;
            Spdlog::info("Application mode = Online");
            break;
        }
        case CommandLineParameters::ApplicationMode::MapEditor: {
            client_state_->draw_map_editor_interface = true;
            client_state_->draw_game_interface = false;
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
        world_->GetStateManager()->GetMap().CreateEmptyMap();
    } else {
        world_->GetStateManager()->GetMap().LoadMap(map_path);
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
    input_router_.SetMouseButtonHandler([&](int button, int action) {
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            client_state_->event_left_mouse_button_clicked.Notify();
        }

        if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
            client_state_->event_right_mouse_button_clicked.Notify();
        }

        if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS) {
            client_state_->event_middle_mouse_button_clicked.Notify();
        }

        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
            client_state_->event_left_mouse_button_released.Notify();
        }

        if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
            client_state_->event_right_mouse_button_released.Notify();
        }

        if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE) {
            client_state_->event_middle_mouse_button_released.Notify();
        }
    });

    input_router_.SetKeyHandler([&](int key, int action) {
        if (action == GLFW_PRESS) {
            client_state_->event_key_pressed.Notify(key);
        }

        if (action == GLFW_RELEASE) {
            client_state_->event_key_released.Notify(key);
        }
    });
    input_router_.SetMouseScreenMoveHandler(
      [&](glm::vec2 last_mouse_position, glm::vec2 new_mouse_position) {
          if (std::abs(last_mouse_position.x - new_mouse_position.x) > 0.001F ||
              std::abs(last_mouse_position.y - new_mouse_position.y) > 0.001F) {
              client_state_->event_mouse_screen_position_changed.Notify(last_mouse_position,
                                                                        new_mouse_position);
          }
      });
    input_router_.SetMouseMapMoveHandler(
      [&](glm::vec2 last_mouse_position, glm::vec2 new_mouse_position) {
          if (std::abs(last_mouse_position.x - new_mouse_position.x) > 0.0000001F ||
              std::abs(last_mouse_position.y - new_mouse_position.y) > 0.0000001F) {
              client_state_->event_mouse_map_position_changed.Notify(last_mouse_position,
                                                                     new_mouse_position);
              client_state_->mouse_map_position = GetCurrentMouseMapPosition();
          }
      });
    input_router_.SetMouseScrollHandler([&](float wheel_delta) {
        if (wheel_delta < 0.0F) {
            client_state_->event_mouse_wheel_scrolled_down.Notify();
        } else if (wheel_delta > 0.0F) {
            client_state_->event_mouse_wheel_scrolled_up.Notify();
        }

        glm::vec2 mouse_map_position = GetCurrentMouseMapPosition();
        client_state_->event_mouse_map_position_changed.Notify(client_state_->mouse_map_position,
                                                               mouse_map_position);
        client_state_->mouse_map_position = mouse_map_position;
    });

    world_->GetPhysicsEvents().soldier_collides_with_polygon.AddObserver(
      [&](const Soldier& /*soldier*/, const PMSPolygon& polygon) {
          if (client_state_->draw_colliding_polygons) {
              bool exists = false;
              for (unsigned int poly_id : client_state_->colliding_polygon_ids) {
                  if (polygon.id == poly_id) {
                      exists = true;
                      break;
                  }
              }
              if (!exists) {
                  client_state_->colliding_polygon_ids.push_back(polygon.id);
              }
          }
      });
    world_->GetPhysicsEvents().item_collides_with_polygon.AddObserver(
      [&](Item& /*item*/, const PMSPolygon& polygon) {
          if (client_state_->draw_colliding_polygons) {
              bool exists = false;
              for (unsigned int poly_id : client_state_->colliding_polygon_ids) {
                  if (polygon.id == poly_id) {
                      exists = true;
                      break;
                  }
              }
              if (!exists) {
                  client_state_->colliding_polygon_ids.push_back(polygon.id);
              }
          }
      });

    window_->Create(window_size_mode_);
    input_router_.ResetMousePositions(GetCurrentMouseScreenPosition(), GetCurrentMouseMapPosition());
    UpdateWindowSize();
    client_state_->camera_component.UpdateWindowDimensions(
      { client_state_->window_width, client_state_->window_height });

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
        client_state_->camera_component.UpdateWindowDimensions(new_window_dimensions);
        client_state_->event_window_resized.Notify(new_window_dimensions);
    });

    // Set lower FPS limit when the window is not focused to free up the CPU
    window_->RegisterOnFocusGainObserver([&]() { world_->SetFPSLimit(fps_limit_); });
    window_->RegisterOnFocusLossObserver([&]() { world_->SetFPSLimit(60); });

    if (application_mode_ == CommandLineParameters::ApplicationMode::MapEditor) {
        window_->SetCursorMode(CursorMode::Normal);
        world_->GetStateManager()->PauseGame();
    }

    client_state_->map_editor_state.event_pressed_play.AddObserver([&]() {
        std::uint8_t client_soldier_id = *client_state_->client_soldier_id;
        bool is_soldier_active = world_->GetStateManager()->GetSoldier(client_soldier_id).active;
        bool is_soldier_alive = !world_->GetStateManager()->GetSoldier(client_soldier_id).dead_meat;

        if (!is_soldier_active || !is_soldier_alive) {
            world_->SpawnSoldier(*client_state_->client_soldier_id);
        }

        client_state_->draw_game_interface = true;
        client_state_->draw_map_editor_interface = false;
        client_state_->draw_game_debug_interface = true;
        client_state_->camera_component.ResetZoom();
        world_->GetStateManager()->UnPauseGame();
        if (!world_->GetStateManager()->GetMap().GetPolygons().empty()) {
            auto vertex = world_->GetStateManager()->GetMap().GetPolygons().at(0).vertices.at(0);
            glm::vec2 old_polygon_position = { vertex.x, vertex.y };

            world_->GetStateManager()->GetMap().GenerateSectors();

            vertex = world_->GetStateManager()->GetMap().GetPolygons().at(0).vertices.at(0);
            glm::vec2 move_offset = { vertex.x - old_polygon_position.x,
                                      vertex.y - old_polygon_position.y };

            // Move soldiers if the map moved
            world_->GetStateManager()->TransformSoldiers([&](auto& soldier) {
                world_->GetStateManager()->MoveSoldier(soldier.id, move_offset);
            });
        }
        window_->SetCursorMode(CursorMode::Locked);
        map_editor_->Lock();
    });

    client_state_->event_key_pressed.AddObserver([&](int key) {
        if (key == GLFW_KEY_F5 &&
            application_mode_ == CommandLineParameters::ApplicationMode::MapEditor) {
            if (client_state_->draw_game_interface) {
                client_state_->draw_game_interface = false;
                client_state_->draw_map_editor_interface = true;
                client_state_->draw_game_debug_interface = false;
                world_->GetStateManager()->PauseGame();
                window_->SetCursorMode(CursorMode::Normal);
                map_editor_->Unlock();
            } else {
                client_state_->map_editor_state.event_pressed_play.Notify();
            }
        }
    });

    scene_ = std::make_unique<Scene>(world_->GetStateManager(), *client_state_);

    client_state_->map_editor_state.event_pixel_color_under_cursor_requested.AddObserver([&]() {
        glm::vec2 mouse_position = window_->GetCursorScreenPosition();
        // glReadPixels requires inverted Y axis for some reason...
        mouse_position.y = (float)window_->GetWindowSize().y - mouse_position.y;
        client_state_->map_editor_state.last_requested_pixel_color =
          Scene::GetPixelColor(mouse_position);
    });

    world_->SetShouldStopGameLoopCallback([&]() { return window_->ShouldClose(); });
    world_->SetPreGameLoopIterationCallback([&]() {
        UpdateInputContext();
        RouteInput();

        const PlatformInput& input = window_->GetPlatformInput();
        if (input.KeyWentDown(GLFW_KEY_ESCAPE)) {
            window_->Close();
        }
        UpdateWindowSize();

        glm::vec2 mouse_screen_position = GetCurrentMouseScreenPosition();

        if (application_mode_ == CommandLineParameters::ApplicationMode::Local ||
            (application_mode_ == CommandLineParameters::ApplicationMode::MapEditor &&
             client_state_->draw_game_interface)) {
            if (input_router_.GetActiveContext() != InputContext::UiCaptured &&
                input.KeyWentDown(GLFW_KEY_F10)) {
                world_->GetStateManager()->TogglePauseGame();
            }

            if (world_->GetStateManager()->IsGamePaused()) {
                glm::vec2 mouse_position = { input.GetX(), input.GetY() };

                client_state_->camera_prev = client_state_->camera;

                client_state_->mouse.x = mouse_position.x;
                client_state_->mouse.y = mouse_position.y;
            }
        } else if (application_mode_ == CommandLineParameters::ApplicationMode::MapEditor) {
            client_state_->camera_prev = client_state_->camera;

            client_state_->mouse.x = mouse_screen_position.x;
            client_state_->mouse.y = mouse_screen_position.y;
        }
    });
    world_->SetPreWorldUpdateCallback([&]() {
        client_state_->colliding_polygon_ids.clear();

        if (application_mode_ == CommandLineParameters::ApplicationMode::Online) {
            networking_client_->SetLag(client_state_->network_lag);
            networking_client_->Update(client_network_event_dispatcher_);

            if ((world_->GetStateManager()->GetGameTick() % 60 == 0)) {
                if (client_state_->ping_timer.IsRunning()) {
                    client_state_->ping_timer.Update();
                    if (client_state_->ping_timer.IsOverThreshold()) {
                        networking_client_->SendNetworkMessage({ NetworkEvent::PingCheck },
                                                               DeliveryMode::Unreliable);
                    }
                } else {
                    client_state_->ping_timer.Start();
                    networking_client_->SendNetworkMessage({ NetworkEvent::PingCheck },
                                                           DeliveryMode::Unreliable);
                }
            }
        }

        const PlatformInput& input = window_->GetPlatformInput();
        glm::vec2 mouse_position = { input.GetX(), input.GetY() };
        client_state_->mouse.x = mouse_position.x;
        client_state_->mouse.y = mouse_position.y;

        float ratio_x = client_state_->window_width / client_state_->camera_component.GetWidth();
        float ratio_y = client_state_->window_height / client_state_->camera_component.GetHeight();
        mouse_position.x /= ratio_x;
        mouse_position.y /= ratio_y;
        client_state_->camera_prev = client_state_->camera;

        if (client_state_->client_soldier_id.has_value()) {
            std::uint8_t client_soldier_id = *client_state_->client_soldier_id;
            bool is_soldier_active =
              world_->GetStateManager()->GetSoldier(client_soldier_id).active;
            bool is_soldier_alive =
              !world_->GetStateManager()->GetSoldier(client_soldier_id).dead_meat;

            if (is_soldier_active) {
                if (is_soldier_alive) {
                    world_->GetStateManager()->ChangeSoldierControlActionState(
                      client_soldier_id, ControlActionType::MoveLeft, input.Key(GLFW_KEY_A));
                    world_->GetStateManager()->ChangeSoldierControlActionState(
                      client_soldier_id, ControlActionType::MoveRight, input.Key(GLFW_KEY_D));
                    world_->GetStateManager()->ChangeSoldierControlActionState(
                      client_soldier_id, ControlActionType::Jump, input.Key(GLFW_KEY_W));
                    world_->GetStateManager()->ChangeSoldierControlActionState(
                      client_soldier_id, ControlActionType::Crouch, input.Key(GLFW_KEY_S));
                    world_->GetStateManager()->ChangeSoldierControlActionState(
                      client_soldier_id,
                      ControlActionType::ChangeWeapon,
                      input.Key(GLFW_KEY_Q));
                    world_->GetStateManager()->ChangeSoldierControlActionState(
                      client_soldier_id,
                      ControlActionType::ThrowGrenade,
                      input.Key(GLFW_KEY_E));
                    world_->GetStateManager()->ChangeSoldierControlActionState(
                      client_soldier_id, ControlActionType::DropWeapon, input.Key(GLFW_KEY_F));
                    world_->GetStateManager()->ChangeSoldierControlActionState(
                      client_soldier_id, ControlActionType::Prone, input.Key(GLFW_KEY_X));
                    world_->GetStateManager()->ChangeSoldierControlActionState(
                      client_soldier_id,
                      ControlActionType::ThrowFlag,
                      input.Key(GLFW_KEY_W) && input.Key(GLFW_KEY_S));

                    world_->GetStateManager()->ChangeSoldierControlActionState(
                      client_soldier_id,
                      ControlActionType::UseJets,
                      input.Button(GLFW_MOUSE_BUTTON_RIGHT));
                    world_->GetStateManager()->ChangeSoldierControlActionState(
                      client_soldier_id,
                      ControlActionType::Fire,
                      input.Button(GLFW_MOUSE_BUTTON_LEFT));

                    world_->GetStateManager()->SoldierControlApply(
                      client_soldier_id, [&](const Soldier& soldier, Control& control) {
                          PlayerInput::UpdatePlayerSoldierControlCollisions(
                            soldier, control, client_state_);
                      });
                }

                world_->UpdateWeaponChoices(client_soldier_id,
                                            client_state_->primary_weapon_type_choice,
                                            client_state_->secondary_weapon_type_choice);

                // TODO: Move camera calculation somewhere
                client_state_->camera_prev = client_state_->camera;

                glm::vec2 mouse;
                mouse.x = mouse_position.x;
                mouse.y = client_state_->camera_component.GetHeight() -
                          mouse_position.y; // TODO: soldier.control.mouse_aim_y expects
                // top to be 0 and bottom to be game_height

                mouse.y = mouse_position.y;

                float width = client_state_->camera_component.GetWidth();
                float height = client_state_->camera_component.GetHeight();

                if (client_state_->smooth_camera) {
                    auto z = 1.0F;
                    glm::vec2 m{ 0.0F, 0.0F };

                    m.x = z * (mouse.x - width / 2.0F) / 7.0F *
                          ((2.0F * 640.0F / width - 1.0F) + (width - 640.0F) / width * 0.0F / 6.8F);
                    m.y = z * (mouse.y - height / 2.0F) / 7.0F;

                    glm::vec2 cam_v = client_state_->camera;
                    glm::vec2 p = { world_->GetSoldier(client_soldier_id).particle.position.x,
                                    -world_->GetSoldier(client_soldier_id).particle.position.y };
                    glm::vec2 norm = p - cam_v;
                    glm::vec2 s = norm * 0.14F;
                    cam_v += s;
                    cam_v += m;
                    client_state_->camera = cam_v;

                } else {
                    client_state_->camera.x =
                      world_->GetSoldier(client_soldier_id).particle.position.x +
                      (float)(mouse.x - (width / 2));
                    client_state_->camera.y =
                      -world_->GetSoldier(client_soldier_id).particle.position.y +
                      (float)((mouse.y) - (height / 2));
                }

                glm::vec2 mouse_map_position = GetCurrentMouseMapPosition();

                world_->GetStateManager()->ChangeSoldierMouseMapPosition(client_soldier_id,
                                                                         mouse_map_position);
            } else {
                client_state_->camera = { 0.0F, 0.0F };
            }

            if (application_mode_ == CommandLineParameters::ApplicationMode::Online) {
                glm::vec2 mouse_map_position = GetCurrentMouseMapPosition();

                SoldierInputPacket update_soldier_state_packet{
                    .input_sequence_id = input_sequence_id_,
                    .game_tick = world_->GetStateManager()->GetGameTick(),
                    .position_x = world_->GetSoldier(client_soldier_id).particle.position.x,
                    .position_y = world_->GetSoldier(client_soldier_id).particle.position.y,
                    .mouse_map_position_x = mouse_map_position.x,
                    .mouse_map_position_y = mouse_map_position.y,
                    .control = world_->GetSoldier(client_soldier_id).control
                };
                if (client_state_->server_reconciliation) {
                    client_state_->soldier_snapshot_history.emplace_back(
                      input_sequence_id_, world_->GetSoldier(client_soldier_id));
                }
                input_sequence_id_++;
                if (client_state_->server_reconciliation) {
                    client_state_->pending_inputs.push_back(update_soldier_state_packet);
                }
                networking_client_->SendNetworkMessage(
                  { NetworkEvent::SoldierInput, update_soldier_state_packet },
                  DeliveryMode::Unreliable);

                if (client_state_->kill_button_just_pressed) {
                    client_state_->kill_button_just_pressed = false;
                    networking_client_->SendNetworkMessage({ NetworkEvent::KillCommand },
                                                           DeliveryMode::Unreliable);
                }
            } else {
                if (client_state_->kill_button_just_pressed) {
                    client_state_->kill_button_just_pressed = false;
                    world_->KillSoldier(client_soldier_id);
                }
            }
        } else {
            client_state_->camera = { 0.0F, 0.0F };
        }
    });
    world_->SetPostWorldUpdateCallback([&](const StateManager& /*state_manager*/) {});
    world_->SetPostGameLoopIterationCallback(
      [&](const StateManager& state_manager, double frame_percent, int last_fps) {
          if (!client_state_->objects_interpolation) {
              frame_percent = 1.0F;
          }
          scene_->Render(state_manager, *client_state_, frame_percent, last_fps);

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
                client_state_->client_side_prediction) {
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

glm::vec2 Application::GetCurrentMouseScreenPosition()
{
    return window_->GetCursorScreenPosition();
}

glm::vec2 Application::GetCurrentMouseMapPosition()
{
    glm::vec2 window_size = window_->GetWindowSize();
    glm::vec2 mouse_map_position;
    if (window_->GetCursorMode() == CursorMode::Locked) {
        mouse_map_position = { window_->GetPlatformInput().GetX(),
                               window_size.y - window_->GetPlatformInput().GetY() };
    } else {
        mouse_map_position = window_->GetCursorScreenPosition();
    }

    float ratio_x = window_size.x / client_state_->camera_component.GetWidth();
    float ratio_y = window_size.y / client_state_->camera_component.GetHeight();

    mouse_map_position.x /= ratio_x;
    mouse_map_position.y /= ratio_y;
    window_size.x /= ratio_x;
    window_size.y /= ratio_y;

    mouse_map_position.x =
      (mouse_map_position.x - (float)window_size.x / 2.0F + client_state_->camera.x);
    mouse_map_position.y =
      (mouse_map_position.y - (float)window_size.y / 2.0F - client_state_->camera.y);
    return mouse_map_position;
}

void Application::UpdateWindowSize()
{
    glm::ivec2 window_size = window_->GetWindowSize();
    client_state_->window_width = (float)window_size.x;
    client_state_->window_height = (float)window_size.y;
}

void Application::RouteInput()
{
    const InputSnapshot input_snapshot = window_->GetPlatformInput().CreateSnapshot(
      GetCurrentMouseScreenPosition(), GetCurrentMouseMapPosition());
    input_router_.Route(input_snapshot);
}

void Application::UpdateInputContext()
{
    if (DebugUI::GetWantCaptureMouse() ||
        client_state_->map_editor_state.is_mouse_hovering_over_ui ||
        client_state_->map_editor_state.is_modal_or_popup_open) {
        input_router_.SetActiveContext(InputContext::UiCaptured);
        return;
    }

    if (application_mode_ == CommandLineParameters::ApplicationMode::MapEditor) {
        input_router_.SetActiveContext(client_state_->draw_game_interface
                                         ? InputContext::EditorPlayTest
                                         : InputContext::Editor);
        return;
    }

    input_router_.SetActiveContext(InputContext::Gameplay);
}
} // namespace Soldank
