#include "Application.hpp"

#include "application/cli/CommandLineParameters.hpp"
#include "application/config/Config.hpp"
#include "application/input/Keyboard.hpp"
#include "application/input/Mouse.hpp"
#include "application/input/PlayerInput.hpp"
#include "cli/CommandLineParameters.hpp"
#include "application/window/Window.hpp"

#include "communication/NetworkPackets.hpp"
#include "core/state/Control.hpp"
#include "map_editor/MapEditor.hpp"
#include "networking/NetworkingClient.hpp"
#include "networking/event_handlers/AssignPlayerIdNetworkEventHandler.hpp"
#include "networking/event_handlers/PingCheckNetworkEventHandler.hpp"
#include "networking/event_handlers/PlayerLeaveNetworkEventHandler.hpp"
#include "networking/event_handlers/ProjectileSpawnNetworkEventHandler.hpp"
#include "networking/event_handlers/SoldierInfoNetworkEventHandler.hpp"
#include "networking/event_handlers/SoldierStateNetworkEventHandler.hpp"
#include "networking/event_handlers/SpawnSoldierNetworkEventHandler.hpp"
#include "networking/event_handlers/KillSoldierNetworkEventHandler.hpp"
#include "networking/event_handlers/HitSoldierNetworkEventHandler.hpp"

#include "core/World.hpp"
#include "core/CoreEventHandler.hpp"

#include "rendering/Scene.hpp"
#include "rendering/ClientState.hpp"

#include "communication/NetworkEventDispatcher.hpp"

#include "spdlog/spdlog.h"

#include <spdlog/common.h>
#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>

#include <GLFW/glfw3.h>

#include <SimpleIni.h>

#include <cmath>
#include <memory>
#include <chrono>
#include <thread>
#include <span>

namespace Soldank::Application
{
std::unique_ptr<Window> window;
std::shared_ptr<IWorld> world;
std::unique_ptr<INetworkingClient> networking_client;
std::shared_ptr<ClientState> client_state;
std::shared_ptr<NetworkEventDispatcher> client_network_event_dispatcher;
std::unique_ptr<MapEditor> map_editor;

SteamNetworkingMicroseconds log_time_zero;

CommandLineParameters::ApplicationMode application_mode;
WindowSizeMode window_size_mode;

int fps_limit = 0;

void DebugOutput(ESteamNetworkingSocketsDebugOutputType output_type, const char* message)
{
    SteamNetworkingMicroseconds time = SteamNetworkingUtils()->GetLocalTimestamp() - log_time_zero;
    spdlog::info("{} {}", (double)time * 1e-6, message);
    fflush(stdout);
    if (output_type == k_ESteamNetworkingSocketsDebugOutputType_Bug) {
        exit(1);
    }
}

// NOLINTNEXTLINE modernize-avoid-c-arrays
bool Init(int argc, const char* argv[])
{
    spdlog::set_level(spdlog::level::debug);

    CSimpleIniA ini_config;
    SI_Error rc = ini_config.LoadFile("debug_config.ini");
    std::string server_ip;
    int server_port = 0;
    if (rc < 0) {
        spdlog::warn("INI File could not be loaded: debug_config.ini");
        application_mode = CommandLineParameters::ApplicationMode::Local;
    } else {
        application_mode = ini_config.GetBoolValue("Network", "Online")
                             ? CommandLineParameters::ApplicationMode::Online
                             : CommandLineParameters::ApplicationMode::Local;
        if (application_mode == CommandLineParameters::ApplicationMode::Online) {
            spdlog::info("Online = true");
            const auto* ip = ini_config.GetValue("Network", "Server_IP");
            int port = ini_config.GetLongValue("Network", "Server_Port");
            if (ip == nullptr || port == 0) {
                spdlog::warn("Server_IP or Server_Port not set, setting is_online to false");
                application_mode = CommandLineParameters::ApplicationMode::Local;
            } else {
                server_ip = ip;
                server_port = port;
                spdlog::debug("Server_ip = {} server_port = {}", server_ip, server_port);
            }
        } else {
            spdlog::info("Online = false");
        }
    }

    CommandLineParameters::ParsedValues cli_parameters = CommandLineParameters::Parse(argc, argv);
    if (!cli_parameters.is_parsing_successful) {
        return false;
    }

    window_size_mode = cli_parameters.window_size_mode;
    fps_limit = cli_parameters.fps_limit;

    std::string map_path;

    // If specified then override the application mode that was set from the INI file
    if (cli_parameters.application_mode != CommandLineParameters::ApplicationMode::Default) {
        application_mode = cli_parameters.application_mode;
    }

    window = std::make_unique<Window>();
    world = std::make_shared<World>();
    client_state = std::make_shared<ClientState>();
    client_state->is_game_debug_interface_enabled = cli_parameters.is_debug_ui_enabled;

    switch (application_mode) {
        case CommandLineParameters::ApplicationMode::Default: {
            spdlog::critical("Application mode = Default. That should have never happened.");
            std::unreachable();
            break;
        }
        case CommandLineParameters::ApplicationMode::Local: {
            spdlog::info("Application mode = Local");
            map_path = "maps/ctf_Ash.pms";
            client_state->draw_game_debug_interface = true;
            client_state->draw_game_interface = true;
            break;
        }
        case CommandLineParameters::ApplicationMode::Online: {
            if (cli_parameters.application_mode !=
                CommandLineParameters::ApplicationMode::Default) {
                server_ip = cli_parameters.join_server_ip;
                server_port = cli_parameters.join_server_port;
            }
            map_path = "maps/ctf_Ash.pms";
            client_state->draw_game_debug_interface = true;
            client_state->draw_server_pov_client_pos = true;
            client_state->draw_game_interface = true;
            spdlog::info("Application mode = Online");
            break;
        }
        case CommandLineParameters::ApplicationMode::MapEditor: {
            client_state->draw_map_editor_interface = true;
            client_state->draw_game_interface = false;
            map_editor =
              std::make_unique<MapEditor>(*client_state, world->GetStateManager()->GetState());
            spdlog::info("Application mode = MapEditor");
            break;
        }
    }

    if (cli_parameters.map) {
        map_path = "maps/" + *cli_parameters.map + ".pms";
    }
    spdlog::debug("{} Map: {}", map_path.empty(), map_path);

    if (map_path.empty()) {
        world->GetStateManager()->GetState().map.CreateEmptyMap();
    } else {
        world->GetStateManager()->GetState().map.LoadMap(map_path);
    }
    client_state->server_reconciliation = true;
    client_state->client_side_prediction = true;
    client_state->objects_interpolation = true;

    Mouse::SubscribeButtonObserver([&](int button, int action) {
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            client_state->event_left_mouse_button_clicked.Notify();
        }

        if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
            client_state->event_right_mouse_button_clicked.Notify();
        }

        if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS) {
            client_state->event_middle_mouse_button_clicked.Notify();
        }

        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
            client_state->event_left_mouse_button_released.Notify();
        }

        if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
            client_state->event_right_mouse_button_released.Notify();
        }

        if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE) {
            client_state->event_middle_mouse_button_released.Notify();
        }
    });

    Keyboard::SubscribeKeyObserver([&](int key, int action) {
        if (action == GLFW_PRESS) {
            client_state->event_key_pressed.Notify(key);
        }

        if (action == GLFW_RELEASE) {
            client_state->event_key_released.Notify(key);
        }
    });

    if (application_mode == CommandLineParameters::ApplicationMode::Online) {
        spdlog::info("Connecting to {}:{}", server_ip, server_port);
        std::vector<std::shared_ptr<INetworkEventHandler>> network_event_handlers{
            std::make_shared<AssignPlayerIdNetworkEventHandler>(world, client_state),
            std::make_shared<PingCheckNetworkEventHandler>(client_state),
            std::make_shared<PlayerLeaveNetworkEventHandler>(world),
            std::make_shared<ProjectileSpawnNetworkEventHandler>(world),
            std::make_shared<SoldierInfoNetworkEventHandler>(world, client_state),
            std::make_shared<SoldierStateNetworkEventHandler>(world, client_state),
            std::make_shared<SpawnSoldierNetworkEventHandler>(world),
            std::make_shared<KillSoldierNetworkEventHandler>(world),
            std::make_shared<HitSoldierNetworkEventHandler>(world)
        };
        client_network_event_dispatcher =
          std::make_shared<NetworkEventDispatcher>(network_event_handlers);

        SteamDatagramErrMsg err_msg;
        if (!GameNetworkingSockets_Init(nullptr, err_msg)) {
            spdlog::error("GameNetworkingSockets_Init failed. {}", std::span(err_msg).data());
        }

        log_time_zero = SteamNetworkingUtils()->GetLocalTimestamp();

        SteamNetworkingUtils()->SetDebugOutputFunction(k_ESteamNetworkingSocketsDebugOutputType_Msg,
                                                       DebugOutput);

        networking_client = std::make_unique<NetworkingClient>(server_ip.c_str(), server_port);
    } else {
        CoreEventHandler::ObserveAll(world.get());
    }

    world->GetPhysicsEvents().soldier_collides_with_polygon.AddObserver(
      [&](const Soldier& /*soldier*/, const PMSPolygon& polygon) {
          if (client_state->draw_colliding_polygons) {
              bool exists = false;
              for (unsigned int poly_id : client_state->colliding_polygon_ids) {
                  if (polygon.id == poly_id) {
                      exists = true;
                      break;
                  }
              }
              if (!exists) {
                  client_state->colliding_polygon_ids.push_back(polygon.id);
              }
          }
      });
    world->GetPhysicsEvents().item_collides_with_polygon.AddObserver(
      [&](Item& /*item*/, const PMSPolygon& polygon) {
          if (client_state->draw_colliding_polygons) {
              bool exists = false;
              for (unsigned int poly_id : client_state->colliding_polygon_ids) {
                  if (polygon.id == poly_id) {
                      exists = true;
                      break;
                  }
              }
              if (!exists) {
                  client_state->colliding_polygon_ids.push_back(polygon.id);
              }
          }
      });

    return true;
}

glm::vec2 GetCurrentMouseScreenPosition()
{
    return window->GetCursorScreenPosition();
}

glm::vec2 GetCurrentMouseMapPosition()
{
    glm::vec2 window_size = window->GetWindowSize();
    glm::vec2 mouse_map_position;
    if (window->GetCursorMode() == CursorMode::Locked) {
        mouse_map_position = { Mouse::GetX(), window_size.y - Mouse::GetY() };
    } else {
        mouse_map_position = window->GetCursorScreenPosition();
    }

    float ratio_x = window_size.x / client_state->camera_component.GetWidth();
    float ratio_y = window_size.y / client_state->camera_component.GetHeight();

    mouse_map_position.x /= ratio_x;
    mouse_map_position.y /= ratio_y;
    window_size.x /= ratio_x;
    window_size.y /= ratio_y;

    mouse_map_position.x =
      (mouse_map_position.x - (float)window_size.x / 2.0F + client_state->camera.x);
    mouse_map_position.y =
      (mouse_map_position.y - (float)window_size.y / 2.0F - client_state->camera.y);
    return mouse_map_position;
}

void UpdateWindowSize()
{
    glm::ivec2 window_size = window->GetWindowSize();
    client_state->window_width = window_size.x;
    client_state->window_height = window_size.y;
}

void Run()
{
    window->Create(window_size_mode);
    glm::vec2 last_mouse_screen_position = GetCurrentMouseScreenPosition();
    glm::vec2 last_mouse_map_position = GetCurrentMouseMapPosition();
    UpdateWindowSize();
    client_state->camera_component.UpdateWindowDimensions(
      { client_state->window_width, client_state->window_height });

    client_state->event_respawn_player_at_spawn_point_requested.AddObserver(
      [&](unsigned int spawn_point_id) {
          const auto& spawn_point =
            world->GetStateManager()->GetState().map.GetSpawnPoints().at(spawn_point_id);
          glm::vec2 position = { spawn_point.x, spawn_point.y };
          world->SpawnSoldier(*client_state->client_soldier_id, position);
      });
    client_state->event_respawn_soldier_requested.AddObserver(
      [&](unsigned int soldier_id) { world->SpawnSoldier(soldier_id); });

    window->RegisterOnScreenResizedObserver([](glm::vec2 new_window_dimensions) {
        client_state->camera_component.UpdateWindowDimensions(new_window_dimensions);
        client_state->event_window_resized.Notify(new_window_dimensions);
    });

    // Set lower FPS limit when the window is not focused to free up the CPU
    window->RegisterOnFocusGainObserver([]() { world->SetFPSLimit(fps_limit); });
    window->RegisterOnFocusLossObserver([]() { world->SetFPSLimit(60); });

    if (application_mode == CommandLineParameters::ApplicationMode::MapEditor) {
        window->SetCursorMode(CursorMode::Normal);
        world->GetStateManager()->GetState().paused = true;
    }

    client_state->map_editor_state.event_pressed_play.AddObserver([&]() {
        std::uint8_t client_soldier_id = *client_state->client_soldier_id;
        bool is_soldier_active = false;
        bool is_soldier_alive = false;
        for (const auto& soldier : world->GetStateManager()->GetState().soldiers) {
            if (soldier.id == client_soldier_id && soldier.active) {
                is_soldier_active = true;
                is_soldier_alive = !soldier.dead_meat;
            }
        }

        if (!is_soldier_active || !is_soldier_alive) {
            world->SpawnSoldier(*client_state->client_soldier_id);
        }

        client_state->draw_game_interface = true;
        client_state->draw_map_editor_interface = false;
        client_state->draw_game_debug_interface = true;
        client_state->camera_component.ResetZoom();
        world->GetStateManager()->GetState().paused = false;
        if (!world->GetStateManager()->GetState().map.GetPolygons().empty()) {
            auto vertex =
              world->GetStateManager()->GetState().map.GetPolygons().at(0).vertices.at(0);
            glm::vec2 old_polygon_position = { vertex.x, vertex.y };

            world->GetStateManager()->GetState().map.GenerateSectors();

            vertex = world->GetStateManager()->GetState().map.GetPolygons().at(0).vertices.at(0);
            glm::vec2 move_offset = { vertex.x - old_polygon_position.x,
                                      vertex.y - old_polygon_position.y };

            // Move soldiers if the map moved
            for (const auto& soldier : world->GetStateManager()->GetState().soldiers) {
                world->GetStateManager()->MoveSoldier(soldier.id, move_offset);
            }
        }
        window->SetCursorMode(CursorMode::Locked);
        map_editor->Lock();
    });

    client_state->event_key_pressed.AddObserver([&](int key) {
        if (key == GLFW_KEY_F5 &&
            application_mode == CommandLineParameters::ApplicationMode::MapEditor) {
            if (client_state->draw_game_interface) {
                client_state->draw_game_interface = false;
                client_state->draw_map_editor_interface = true;
                client_state->draw_game_debug_interface = false;
                world->GetStateManager()->GetState().paused = true;
                window->SetCursorMode(CursorMode::Normal);
                map_editor->Unlock();
            } else {
                client_state->map_editor_state.event_pressed_play.Notify();
            }
        }
    });

    Mouse::SubscribeMouseMovementObserver([&](double x, double y) {
        glm::vec2 mouse_screen_position = GetCurrentMouseScreenPosition();
        if (std::abs(last_mouse_screen_position.x - mouse_screen_position.x) > 0.001F ||
            std::abs(last_mouse_screen_position.y - mouse_screen_position.y) > 0.001F) {
            client_state->event_mouse_screen_position_changed.Notify(last_mouse_screen_position,
                                                                     mouse_screen_position);
            mouse_screen_position = GetCurrentMouseScreenPosition();
        }
        last_mouse_screen_position = mouse_screen_position;

        glm::vec2 mouse_map_position = GetCurrentMouseMapPosition();
        if (std::abs(last_mouse_map_position.x - mouse_map_position.x) > 0.0000001F ||
            std::abs(last_mouse_map_position.y - mouse_map_position.y) > 0.0000001F) {
            client_state->event_mouse_map_position_changed.Notify(last_mouse_map_position,
                                                                  mouse_map_position);
            mouse_map_position = GetCurrentMouseMapPosition();
            client_state->mouse_map_position = mouse_map_position;
        }
        last_mouse_map_position = mouse_map_position;
    });

    Mouse::SubscribeMouseScrollObserver([&](double dx, double dy) {
        if (dy < 0.0) {
            client_state->event_mouse_wheel_scrolled_down.Notify();
        } else if (dy > 0.0) {
            client_state->event_mouse_wheel_scrolled_up.Notify();
        }

        glm::vec2 mouse_map_position = GetCurrentMouseMapPosition();
        if (std::abs(last_mouse_map_position.x - mouse_map_position.x) > 0.0000001F ||
            std::abs(last_mouse_map_position.y - mouse_map_position.y) > 0.0000001F) {
            client_state->event_mouse_map_position_changed.Notify(last_mouse_map_position,
                                                                  mouse_map_position);
            mouse_map_position = GetCurrentMouseMapPosition();
            client_state->mouse_map_position = mouse_map_position;
        }
        last_mouse_map_position = mouse_map_position;
    });

    Scene scene(world->GetStateManager(), *client_state);

    client_state->map_editor_state.event_pixel_color_under_cursor_requested.AddObserver([&]() {
        glm::vec2 mouse_position = window->GetCursorScreenPosition();
        // glReadPixels requires inverted Y axis for some reason...
        mouse_position.y = window->GetWindowSize().y - mouse_position.y;
        client_state->map_editor_state.last_requested_pixel_color =
          scene.GetPixelColor(mouse_position);
    });

    world->SetShouldStopGameLoopCallback([&]() { return window->ShouldClose(); });
    world->SetPreGameLoopIterationCallback([&]() {
        if (Keyboard::KeyWentDown(GLFW_KEY_ESCAPE)) {
            window->Close();
        }
        UpdateWindowSize();

        glm::vec2 mouse_screen_position = GetCurrentMouseScreenPosition();

        if (application_mode == CommandLineParameters::ApplicationMode::Local ||
            (application_mode == CommandLineParameters::ApplicationMode::MapEditor &&
             client_state->draw_game_interface)) {
            if (Keyboard::KeyWentDown(GLFW_KEY_F10)) {
                world->GetStateManager()->GetState().paused =
                  !world->GetStateManager()->GetState().paused;
            }

            if (world->GetStateManager()->GetState().paused) {
                glm::vec2 mouse_position = { Mouse::GetX(), Mouse::GetY() };

                client_state->camera_prev = client_state->camera;

                client_state->mouse.x = mouse_position.x;
                client_state->mouse.y = mouse_position.y;
            }
        } else if (application_mode == CommandLineParameters::ApplicationMode::MapEditor) {
            client_state->camera_prev = client_state->camera;

            client_state->mouse.x = mouse_screen_position.x;
            client_state->mouse.y = mouse_screen_position.y;
        }
    });
    unsigned int input_sequence_id = 1;
    world->SetPreWorldUpdateCallback([&]() {
        client_state->colliding_polygon_ids.clear();

        if (application_mode == CommandLineParameters::ApplicationMode::Online) {
            networking_client->SetLag(client_state->network_lag);
            networking_client->Update(client_network_event_dispatcher);

            if ((world->GetStateManager()->GetState().game_tick % 60 == 0)) {
                if (client_state->ping_timer.IsRunning()) {
                    client_state->ping_timer.Update();
                    if (client_state->ping_timer.IsOverThreshold()) {
                        networking_client->SendNetworkMessage({ NetworkEvent::PingCheck });
                    }
                } else {
                    client_state->ping_timer.Start();
                    networking_client->SendNetworkMessage({ NetworkEvent::PingCheck });
                }
            }
        }

        glm::vec2 mouse_position = { Mouse::GetX(), Mouse::GetY() };
        client_state->mouse.x = mouse_position.x;
        client_state->mouse.y = mouse_position.y;

        float ratio_x = client_state->window_width / client_state->camera_component.GetWidth();
        float ratio_y = client_state->window_height / client_state->camera_component.GetHeight();
        mouse_position.x /= ratio_x;
        mouse_position.y /= ratio_y;
        client_state->camera_prev = client_state->camera;

        if (client_state->client_soldier_id.has_value()) {
            std::uint8_t client_soldier_id = *client_state->client_soldier_id;
            bool is_soldier_active = false;
            bool is_soldier_alive = false;
            for (const auto& soldier : world->GetStateManager()->GetState().soldiers) {
                if (soldier.id == client_soldier_id && soldier.active) {
                    is_soldier_active = true;
                    is_soldier_alive = !soldier.dead_meat;
                }
            }

            if (is_soldier_active) {
                if (is_soldier_alive) {
                    world->GetStateManager()->ChangeSoldierControlActionState(
                      client_soldier_id, ControlActionType::MoveLeft, Keyboard::Key(GLFW_KEY_A));
                    world->GetStateManager()->ChangeSoldierControlActionState(
                      client_soldier_id, ControlActionType::MoveRight, Keyboard::Key(GLFW_KEY_D));
                    world->GetStateManager()->ChangeSoldierControlActionState(
                      client_soldier_id, ControlActionType::Jump, Keyboard::Key(GLFW_KEY_W));
                    world->GetStateManager()->ChangeSoldierControlActionState(
                      client_soldier_id, ControlActionType::Crouch, Keyboard::Key(GLFW_KEY_S));
                    world->GetStateManager()->ChangeSoldierControlActionState(
                      client_soldier_id,
                      ControlActionType::ChangeWeapon,
                      Keyboard::Key(GLFW_KEY_Q));
                    world->GetStateManager()->ChangeSoldierControlActionState(
                      client_soldier_id,
                      ControlActionType::ThrowGrenade,
                      Keyboard::Key(GLFW_KEY_E));
                    world->GetStateManager()->ChangeSoldierControlActionState(
                      client_soldier_id, ControlActionType::DropWeapon, Keyboard::Key(GLFW_KEY_F));
                    world->GetStateManager()->ChangeSoldierControlActionState(
                      client_soldier_id, ControlActionType::Prone, Keyboard::Key(GLFW_KEY_X));
                    world->GetStateManager()->ChangeSoldierControlActionState(
                      client_soldier_id,
                      ControlActionType::ThrowFlag,
                      Keyboard::Key(GLFW_KEY_W) && Keyboard::Key(GLFW_KEY_S));

                    world->GetStateManager()->ChangeSoldierControlActionState(
                      client_soldier_id,
                      ControlActionType::UseJets,
                      Mouse::Button(GLFW_MOUSE_BUTTON_RIGHT));
                    world->GetStateManager()->ChangeSoldierControlActionState(
                      client_soldier_id,
                      ControlActionType::Fire,
                      Mouse::Button(GLFW_MOUSE_BUTTON_LEFT));

                    world->GetStateManager()->SoldierControlApply(
                      client_soldier_id, [](const Soldier& soldier, Control& control) {
                          PlayerInput::UpdatePlayerSoldierControlCollisions(
                            soldier, control, client_state);
                      });
                }

                world->UpdateWeaponChoices(client_soldier_id,
                                           client_state->primary_weapon_type_choice,
                                           client_state->secondary_weapon_type_choice);

                // TODO: Move camera calculation somewhere
                client_state->camera_prev = client_state->camera;

                glm::vec2 mouse;
                mouse.x = mouse_position.x;
                mouse.y = client_state->camera_component.GetHeight() -
                          mouse_position.y; // TODO: soldier.control.mouse_aim_y expects
                // top to be 0 and bottom to be game_height

                mouse.y = mouse_position.y;

                float width = client_state->camera_component.GetWidth();
                float height = client_state->camera_component.GetHeight();

                if (client_state->smooth_camera) {
                    auto z = 1.0F;
                    glm::vec2 m{ 0.0F, 0.0F };

                    m.x = z * (mouse.x - width / 2.0F) / 7.0F *
                          ((2.0F * 640.0F / width - 1.0F) + (width - 640.0F) / width * 0.0F / 6.8F);
                    m.y = z * (mouse.y - height / 2.0F) / 7.0F;

                    glm::vec2 cam_v = client_state->camera;
                    glm::vec2 p = { world->GetSoldier(client_soldier_id).particle.position.x,
                                    -world->GetSoldier(client_soldier_id).particle.position.y };
                    glm::vec2 norm = p - cam_v;
                    glm::vec2 s = norm * 0.14F;
                    cam_v += s;
                    cam_v += m;
                    client_state->camera = cam_v;

                } else {
                    client_state->camera.x =
                      world->GetSoldier(client_soldier_id).particle.position.x +
                      (float)(mouse.x - (width / 2));
                    client_state->camera.y =
                      -world->GetSoldier(client_soldier_id).particle.position.y +
                      (float)((mouse.y) - (height / 2));
                }

                glm::vec2 mouse_map_position = GetCurrentMouseMapPosition();

                world->GetStateManager()->ChangeSoldierMousePosition(client_soldier_id,
                                                                     mouse_map_position);
            } else {
                client_state->camera = { 0.0F, 0.0F };
            }

            if (application_mode == CommandLineParameters::ApplicationMode::Online) {
                SoldierInputPacket update_soldier_state_packet{
                    .input_sequence_id = input_sequence_id,
                    .game_tick = world->GetStateManager()->GetState().game_tick,
                    .position_x = world->GetSoldier(client_soldier_id).particle.position.x,
                    .position_y = world->GetSoldier(client_soldier_id).particle.position.y,
                    .mouse_position_x = mouse_position.x,
                    .mouse_position_y = mouse_position.y,
                    .control = world->GetSoldier(client_soldier_id).control
                };
                input_sequence_id++;
                if (client_state->server_reconciliation) {
                    client_state->pending_inputs.push_back(update_soldier_state_packet);
                }
                networking_client->SendNetworkMessage(
                  { NetworkEvent::SoldierInput, update_soldier_state_packet });

                if (client_state->kill_button_just_pressed) {
                    client_state->kill_button_just_pressed = false;
                    networking_client->SendNetworkMessage({ NetworkEvent::KillCommand });
                }
            } else {
                if (client_state->kill_button_just_pressed) {
                    client_state->kill_button_just_pressed = false;
                    world->KillSoldier(client_soldier_id);
                }
            }
        } else {
            client_state->camera = { 0.0F, 0.0F };
        }
    });
    world->SetPostWorldUpdateCallback([&](const State& /*state*/) {});
    world->SetPostGameLoopIterationCallback([&](const State& /*state*/,
                                                double frame_percent,
                                                int last_fps) {
        if (!client_state->objects_interpolation) {
            frame_percent = 1.0F;
        }
        scene.Render(world->GetStateManager()->GetState(), *client_state, frame_percent, last_fps);

        window->SwapBuffers();
        window->PollInput();
    });

    world->SetPreSoldierUpdateCallback([&](const Soldier& soldier) {
        if (application_mode != CommandLineParameters::ApplicationMode::Online) {
            return true;
        }

        if (client_state->client_soldier_id.has_value()) {
            if (*client_state->client_soldier_id == soldier.id &&
                client_state->client_side_prediction) {
                return true;
            }
        }

        return false;
    });
    world->SetPreProjectileSpawnCallback([&](const BulletParams& /*bullet_params*/) {
        return application_mode != CommandLineParameters::ApplicationMode::Online;
    });

    if (application_mode == CommandLineParameters::ApplicationMode::Local ||
        application_mode == CommandLineParameters::ApplicationMode::MapEditor) {
        const auto& soldier = world->CreateSoldier();
        client_state->client_soldier_id = soldier.id;
    }

    if (application_mode == CommandLineParameters::ApplicationMode::Local) {
        world->SpawnSoldier(*client_state->client_soldier_id);
    }

    world->SetFPSLimit(fps_limit);
    world->RunLoop(fps_limit);
}

void Free()
{
    window.reset(nullptr);
    networking_client.reset(nullptr);

    if (application_mode == CommandLineParameters::ApplicationMode::Online) {
        // Give connections time to finish up.  This is an application layer protocol
        // here, it's not TCP.  Note that if you have an application and you need to be
        // more sure about cleanup, you won't be able to do this.  You will need to send
        // a message and then either wait for the peer to close the connection, or
        // you can pool the connection to see if any reliable data is pending.
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        GameNetworkingSockets_Kill();
    }
}
} // namespace Soldank::Application
