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

    std::string map_path;

    // If specified then override the application mode that was set from the INI file
    if (cli_parameters.application_mode != CommandLineParameters::ApplicationMode::Default) {
        application_mode = cli_parameters.application_mode;
    }

    client_state = std::make_shared<ClientState>();

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
            spdlog::info("Application mode = Online");
            break;
        }
        case CommandLineParameters::ApplicationMode::MapEditor: {
            client_state->draw_map_editor_interface = true;
            map_editor = std::make_unique<MapEditor>(*client_state);
            spdlog::info("Application mode = MapEditor");
            break;
        }
    }

    if (cli_parameters.map) {
        map_path = "maps/" + *cli_parameters.map + ".pms";
    }
    spdlog::debug("{} Map: {}", map_path.empty(), map_path);

    window = std::make_unique<Window>();
    world = std::make_shared<World>();
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

    Mouse::SubscribeMouseScrollObserver([&](double dx, double dy) {
        if (dy < 0.0) {
            client_state->event_mouse_wheel_scrolled_down.Notify();
        } else if (dy > 0.0) {
            client_state->event_mouse_wheel_scrolled_up.Notify();
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
    glm::ivec2 window_size = window->GetWindowSize();
    glm::vec2 mouse_map_position = window->GetCursorScreenPosition();

    float ratio_x = window_size.x / 640.0F;
    float ratio_y = window_size.y / 480.0F;

    mouse_map_position.x /= ratio_x;
    mouse_map_position.y /= ratio_y;
    window_size.x /= ratio_x;
    window_size.y /= ratio_y;

    mouse_map_position.x =
      (mouse_map_position.x - (float)window_size.x / 2.0F + client_state->camera.x);
    mouse_map_position.y =
      (mouse_map_position.y - (float)window_size.y / 2.0F + client_state->camera.y);
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
    window->Create();
    glm::vec2 last_mouse_screen_position = GetCurrentMouseScreenPosition();
    glm::vec2 last_mouse_map_position = GetCurrentMouseMapPosition();
    UpdateWindowSize();

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
        world->GetStateManager()->GetState().paused = false;
        window->SetCursorMode(CursorMode::Locked);
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
        }
        last_mouse_map_position = mouse_map_position;
    });

    Scene scene(world->GetStateManager());

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
            if (Keyboard::KeyWentDown(GLFW_KEY_F5) &&
                application_mode == CommandLineParameters::ApplicationMode::MapEditor) {
                client_state->draw_game_interface = false;
                client_state->draw_map_editor_interface = true;
                client_state->draw_game_debug_interface = false;
                world->GetStateManager()->GetState().paused = true;
                window->SetCursorMode(CursorMode::Normal);
            }
            if (world->GetStateManager()->GetState().paused) {
                glm::vec2 mouse_position = { Mouse::GetX(), Mouse::GetY() };

                client_state->game_width = 640.0;
                client_state->game_height = 480.0;
                client_state->camera_prev = client_state->camera;

                client_state->mouse.x = mouse_position.x;
                client_state->mouse.y = mouse_position.y;
            }
        } else if (application_mode == CommandLineParameters::ApplicationMode::MapEditor) {
            client_state->game_width = 640.0;
            client_state->game_height = 480.0;
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

        client_state->game_width = 640.0;
        client_state->game_height = 480.0;
        client_state->camera_prev = client_state->camera;

        client_state->mouse.x = mouse_position.x;
        client_state->mouse.y = mouse_position.y;

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

                world->GetStateManager()->ChangeSoldierMousePosition(
                  client_soldier_id, mouse_position, client_state->smooth_camera);
                world->UpdateWeaponChoices(client_soldier_id,
                                           client_state->primary_weapon_type_choice,
                                           client_state->secondary_weapon_type_choice);

                client_state->camera = { world->GetSoldier(client_soldier_id).camera.x,
                                         -world->GetSoldier(client_soldier_id).camera.y };
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

    world->RunLoop(Config::FPS_LIMIT);
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
