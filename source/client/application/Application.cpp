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

#ifdef SOLDANK_WEBASM_CLIENT_TRANSPORT
import Application.Platform.WebAssemblyStartupAdapter;
#else
import Application.CLI.CommandLineParameters;
#endif
import Application.ClientModes;
import Application.LaunchParameters;
import Application.Window;
import Application.Input.ApplicationInputController;
import Application.Input.Shortcut;
import Application.Input.PlatformInput;
import Application.Platform.ClientTransportStartup;
import Runtime.ClientRuntime;
import Gameplay.GameSession;
import Gameplay.PlayerController;
import Audio.WeaponAudio;
import Gameplay.EmotePresentation;
import Shared.Core.Animations.SoldierEmotes;
import Audio.AudioEvents;
import Gameplay.WeaponSoundEventRouter;
import Shared.Core.Physics.Bullets.BulletEvents;
import Shared.Core.Physics.PhysicsEvents;
import Editor.EditorSession;
import Scene;
import RenderPipeline;
import MapEditor;
import MapEditorState;
import ClientState;

import Networking.NetworkingClient;
import Networking.INetworkingClient;
import Networking.NetworkClientSession;
import Networking.AssignPlayerIdNetworkEventHandler;
import Networking.PingCheckNetworkEventHandler;
import Networking.ChatNetworkEventHandler;
import Chat.Model;
import Shared.Networking.DeliveryMode;
import Networking.PlayerLeaveNetworkEventHandler;
import Networking.ProjectileSpawnNetworkEventHandler;
import Networking.ProjectilePhysicsNetworkEventHandler;
import Networking.ItemStateNetworkEventHandler;
import Networking.PolygonEffectNetworkEventHandler;
import Gameplay.PolygonPresentationEventRouter;
import Networking.SoldierInfoNetworkEventHandler;
import Networking.SoldierStateNetworkEventHandler;
import Networking.SpawnSoldierNetworkEventHandler;
import Networking.MatchStateNetworkEventHandler;
import Networking.KillSoldierNetworkEventHandler;
import Networking.HitSoldierNetworkEventHandler;

import Shared.Core.IWorld;
import Shared.Core.World;
import Shared.Core.GameModes.BuiltinGameModes;
import Shared.Core.GameModes.GameModeRuntime;
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
export namespace Soldank
{
class Application
{
public:
    Application(int argc, const char* argv[]);
    Application(const std::vector<const char*>& cli_parameters);
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(Application other) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&& other) = delete;

    void Run();

private:
    void RestartLocalMatchIfDue()
    {
        if (application_mode_ != ApplicationMode::Local) {
            return;
        }
        const auto state = world_->GetStateManager();
        const auto* mode = state->GetGameMode();
        if (mode == nullptr || mode->GetState().phase != MatchPhase::Finished) {
            local_finished_tick_.reset();
            return;
        }
        if (!local_finished_tick_) {
            local_finished_tick_ = state->GetGameTick();
        }
        if (state->GetGameTick() - *local_finished_tick_ < 300U) {
            return;
        }
        const auto map_path = state->GetLoadedMapPath();
        state->LoadMapDocument(map_path);
        state->TransformSoldiers([&](Soldier& soldier) {
            if (!soldier.IsSpectator()) {
                world_->SpawnSoldier(soldier.id);
            }
        });
        local_finished_tick_.reset();
    }
    std::optional<std::uint32_t> local_finished_tick_;
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
    std::shared_ptr<AudioEvents> audio_events_;
    std::unique_ptr<WeaponAudio> weapon_audio_;
    std::shared_ptr<WeaponSoundEventRouter> weapon_sound_event_router_;
    std::shared_ptr<EmotePresentation> emote_presentation_;
    std::uint32_t next_emote_request_{ 1 };
    std::shared_ptr<PolygonPresentationEventRouter> polygon_presentation_router_;

    ApplicationMode application_mode_;
    WindowSizeMode window_size_mode_;
    int fps_limit_ = 0;
    ClientRuntime client_runtime_;
    GameSession game_session_;
    ClientTransportStartup client_transport_startup_;
};
} // namespace Soldank

namespace Soldank
{
Application::Application(int argc, const char* argv[])
    : Application(std::vector<const char*>(argv, argv + argc))
{
}

Application::Application(const std::vector<const char*>& cli_parameters)
{
    Spdlog::set_level(Spdlog::level::debug);

    std::unique_ptr<ILaunchParameters> launch_parameters;
#ifdef SOLDANK_WEBASM_CLIENT_TRANSPORT
    launch_parameters = std::make_unique<WebAssemblyStartupAdapter>();
#else
    launch_parameters = std::make_unique<CommandLineParameters>(cli_parameters);
#endif
    const ParsedLaunchParameters parsed_launch_parameters = launch_parameters->Parse();
    if (!parsed_launch_parameters.is_parsing_successful) {
        exit(1);
    }

    application_mode_ = parsed_launch_parameters.application_mode;
    window_size_mode_ = parsed_launch_parameters.window_size_mode;
    fps_limit_ = parsed_launch_parameters.fps_limit;

    std::string map_path;

    window_ = std::make_unique<Window>();
    world_ = std::make_shared<World>();
    if (application_mode_ == ApplicationMode::Local) {
        auto definition = MakeBuiltinGameModeRegistry().Create(parsed_launch_parameters.game_mode);
        if (parsed_launch_parameters.survival) {
            definition = WithSurvival(definition);
        }
        world_->GetStateManager()->ConfigureGameMode(std::move(definition));
    }
    world_->GetStateManager()->SetAuthoritativeProjectiles(application_mode_ !=
                                                           ApplicationMode::Online);
    client_state_ = std::make_shared<ClientState>();
    world_->GetPhysicsEvents().projectile_event.AddObserver(
      [client = client_state_](const ProjectilePhysicsEvent& event) {
          if (client->network.replaying_prediction) {
              return;
          }
          auto& events = client->projectile_visual_events;
          if (events.size() >= 512) {
              events.erase(events.begin());
          }
          events.push_back(event);
      });
    world_->GetStateManager()->GetMap().GetMapChangeEvents().replaced_contents.AddObserver(
      [client = client_state_]() {
          client->visual_effects.Reset();
          client->projectile_visual_events.clear();
          client->polygon_visual_events.clear();
      });
    world_->GetWorldEvents().after_soldier_spawns.AddObserver(
      [client = client_state_](const Soldier& soldier) {
          if (!client->network.replaying_prediction) {
              client->visual_effects.ObserveSpawn(soldier);
          }
      });
    input_controller_ = std::make_unique<ApplicationInputController>(
      *window_, *world_, *client_state_, client_runtime_, game_session_);
    player_controller_ =
      std::make_unique<PlayerController>(*world_, *window_, client_state_, *input_controller_);
    client_state_->debug_render.is_game_debug_interface_enabled =
      parsed_launch_parameters.is_debug_ui_enabled;

    switch (application_mode_) {
        case ApplicationMode::Local: {
            Spdlog::info("Application mode = Local");
            client_runtime_.SetClientMode(ClientMode::LocalGame);
            map_path = "maps/ctf_Ash.pms";
            break;
        }
        case ApplicationMode::Online: {
            client_runtime_.SetClientMode(ClientMode::OnlineGame);
            map_path = "maps/ctf_Ash.pms";
            client_state_->network.draw_server_pov_client_pos = true;
            Spdlog::info("Application mode = Online");
            break;
        }
        case ApplicationMode::MapEditor: {
            client_runtime_.SetClientMode(ClientMode::MapEditor);
            client_runtime_.SetEditorMode(EditorMode::Edit);
            map_editor_ = std::make_unique<MapEditor>(*client_state_, *world_->GetStateManager());
            Spdlog::info("Application mode = MapEditor");
            break;
        }
    }

    if (parsed_launch_parameters.map) {
        map_path = "maps/" + *parsed_launch_parameters.map + ".pms";
    }
    Spdlog::debug("{} Map: {}", map_path.empty(), map_path);

    if (map_path.empty()) {
        world_->GetStateManager()->CreateEmptyMapDocument();
    } else {
        world_->GetStateManager()->LoadMapDocument(map_path);
    }

    if (map_editor_) {
        map_editor_->InitializeDocumentTabs();
    }

    if (application_mode_ == ApplicationMode::Online) {
        const ServerEndpoint& server_endpoint = *parsed_launch_parameters.server_endpoint;
        const std::string& server_ip = server_endpoint.ip;
        const std::uint16_t server_port = server_endpoint.port;
        Spdlog::info("Connecting to {}:{}", server_ip, server_port);
        std::vector<std::shared_ptr<INetworkEventHandler>> network_event_handlers{
            std::make_shared<AssignPlayerIdNetworkEventHandler>(world_, client_state_),
            std::make_shared<PingCheckNetworkEventHandler>(client_state_),
            std::make_shared<ChatNetworkEventHandler>(client_state_),
            std::make_shared<PlayerLeaveNetworkEventHandler>(world_),
            std::make_shared<ProjectileSpawnNetworkEventHandler>(world_),
            std::make_shared<ProjectilePhysicsNetworkEventHandler>(world_, client_state_),
            std::make_shared<ItemStateNetworkEventHandler>(world_),
            std::make_shared<PolygonEffectNetworkEventHandler>(world_),
            std::make_shared<SoldierInfoNetworkEventHandler>(world_, client_state_),
            std::make_shared<SoldierStateNetworkEventHandler>(world_, client_state_),
            std::make_shared<SpawnSoldierNetworkEventHandler>(world_),
            std::make_shared<MatchStateNetworkEventHandler>(world_),
            std::make_shared<KillSoldierNetworkEventHandler>(world_),
            std::make_shared<HitSoldierNetworkEventHandler>(world_)
        };
        client_network_event_dispatcher_ =
          std::make_shared<NetworkEventDispatcher>(network_event_handlers);

        client_transport_startup_.Initialize();
        auto networking_client = std::make_unique<NetworkingClient>(server_ip.c_str(), server_port);
        networking_client->SetDisconnectHandler([this] { client_state_->chat.Disconnect(); });
        networking_client_ = std::move(networking_client);
        client_state_->chat.online = true;
        client_state_->chat.send = [this](const NetworkMessage& message) {
            networking_client_->SendNetworkMessage(message, DeliveryMode::Reliable);
        };
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

    if (application_mode_ == ApplicationMode::Online) {
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

    if (application_mode_ == ApplicationMode::MapEditor) {
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
    client_state_->map_editor_state.event_close_application_requested.AddObserver(
      [this]() { window_->Close(); });

    client_state_->event_key_pressed.AddObserver([&](int key, int modifiers) {
        if (key == GLFW_KEY_ESCAPE &&
            game_session_.IsGameplayActive(client_runtime_.GetClientMode(),
                                           client_runtime_.GetEditorMode())) {
            bool& is_menu_open = client_state_->map_editor_state.is_play_test_escape_menu_open;
            is_menu_open = !is_menu_open;
            window_->SetCursorMode(is_menu_open ? CursorMode::Normal : CursorMode::Locked);
            return;
        }
        if (EncodeShortcut(key, modifiers) ==
              client_state_->map_editor_state.GetPlayModeShortcut() &&
            application_mode_ == ApplicationMode::MapEditor &&
            !client_state_->map_editor_state.is_modal_or_popup_open) {
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
        if (application_mode_ == ApplicationMode::Online) {
            network_client_session_->UpdateBeforeWorldTick();
        }
        input_controller_->UpdateContext();
        input_controller_->Route();

        const PlatformInput& input = window_->GetPlatformInput();
        input_controller_->UpdateWindowSize();

        glm::vec2 mouse_screen_position = input_controller_->GetMouseScreenPosition();

        bool is_gameplay_active = game_session_.IsGameplayActive(client_runtime_.GetClientMode(),
                                                                 client_runtime_.GetEditorMode());
        if (is_gameplay_active) {
            client_state_->input.mouse_screen_position = { input.GetX(), input.GetY() };
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
        RestartLocalMatchIfDue();
        client_state_->debug_render.colliding_polygon_ids.clear();

        if (client_state_->client_soldier_id.has_value()) {
            std::uint8_t client_soldier_id = *client_state_->client_soldier_id;
            player_controller_->Update(client_soldier_id);

            if (application_mode_ == ApplicationMode::Online) {
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
    client_state_->chat.request_emote = [this](Emote emote) {
        if (!client_state_->client_soldier_id) return;
        if (application_mode_ == ApplicationMode::Online) {
            networking_client_->SendNetworkMessage(
              { NetworkEvent::EmoteRequest, EmoteRequestPacket{ emote, next_emote_request_++ } },
              DeliveryMode::Reliable);
        } else {
            world_->GetStateManager()->TransformSoldier(
              *client_state_->client_soldier_id, [&](Soldier& soldier) {
                  SoldierEmotes::Request(soldier, emote);
              });
        }
    };
    audio_events_ = std::make_shared<AudioEvents>();
    weapon_audio_ = std::make_unique<WeaponAudio>(*audio_events_);
    polygon_presentation_router_ = PolygonPresentationEventRouter::Create(
      world_->GetPhysicsEvents(), audio_events_, client_state_);
    weapon_sound_event_router_ = WeaponSoundEventRouter::Create(
      world_->GetPhysicsEvents(),
      world_->GetWorldEvents(),
      *world_->GetStateManager(),
      audio_events_,
      [client = client_state_]() { return !client->network.replaying_prediction; });
    emote_presentation_ = EmotePresentation::Create(
      *world_->GetStateManager(), world_->GetWorldEvents(), audio_events_,
      [client = client_state_](const Soldier& soldier, std::uint32_t cues) {
          client->visual_effects.ObserveEmote(soldier, cues);
      },
      [client = client_state_]() { return !client->network.replaying_prediction; });
    world_->SetPostWorldUpdateCallback([&](const StateManager& state_manager) {
        const auto& camera = client_state_->camera;
        client_state_->visual_effects.Update(state_manager,
                                             client_state_->projectile_visual_events,
                                             { camera.position.x, -camera.position.y },
                                             { camera.view.GetWidth(), camera.view.GetHeight() },
                                             client_state_->world_render_options.draw_weather);
        client_state_->projectile_visual_events.clear();
        for (unsigned int sound = 0;
             sound < client_state_->visual_effects.GetCigarSmokeSoundCount();
             ++sound) {
            audio_events_->sound_requested.Notify({ AudioCommand::Play, "smoke" });
        }
        if (!state_manager.IsGamePaused()) {
            for (const auto& event : client_state_->polygon_visual_events) {
                client_state_->visual_effects.ObservePolygon(event);
            }
            client_state_->polygon_visual_events.clear();
        }
        if (application_mode_ == ApplicationMode::Online &&
            client_state_->client_soldier_id.has_value()) {
            network_client_session_->StorePredictedSoldierSnapshot(
              *client_state_->client_soldier_id);
        }
    });
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
        if (client_state_->network.weapon_config_mismatch) {
            return false;
        }
        if (application_mode_ != ApplicationMode::Online) {
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
    world_->GetStateManager()->SetAuthoritativeProjectiles(application_mode_ !=
                                                           ApplicationMode::Online);
    world_->SetPreProjectileSpawnCallback([&](const BulletParams& /*bullet_params*/) {
        return application_mode_ != ApplicationMode::Online;
    });

    if (application_mode_ == ApplicationMode::Local) {
        const auto& soldier = world_->CreateSoldier();
        client_state_->client_soldier_id = soldier.id;
    }

    if (application_mode_ == ApplicationMode::Local) {
        world_->SpawnSoldier(*client_state_->client_soldier_id);
    }

    world_->SetFPSLimit(fps_limit_);
    world_->RunLoop();
}

} // namespace Soldank
