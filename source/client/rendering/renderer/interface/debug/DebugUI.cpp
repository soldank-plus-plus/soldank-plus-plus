#include "rendering/renderer/interface/debug/DebugUI.hpp"

#include "core/state/Control.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

namespace Soldank::DebugUI
{
struct AnimationRecording
{
    bool is_legs;
    unsigned int game_tick;
    AnimationType animation_type;
    unsigned int frame;
    int speed;
    bool soldier_looking_left;
    glm::vec2 soldier_position;
};

struct ActionRecording
{
    bool state;
    ControlActionType action_type;
    unsigned int game_tick;
};

std::string AnimationTypeToString(AnimationType animation_type)
{
    switch (animation_type) {
        case AnimationType::Stand:
            return "Stand";
        case AnimationType::Run:
            return "Run";
        case AnimationType::RunBack:
            return "RunBack";
        case AnimationType::Jump:
            return "Jump";
        case AnimationType::JumpSide:
            return "JumpSide";
        case AnimationType::Fall:
            return "Fall";
        case AnimationType::Crouch:
            return "Crouch";
        case AnimationType::CrouchRun:
            return "CrouchRun";
        case AnimationType::Reload:
            return "Reload";
        case AnimationType::Throw:
            return "Throw";
        case AnimationType::Recoil:
            return "Recoil";
        case AnimationType::SmallRecoil:
            return "SmallRecoil";
        case AnimationType::Shotgun:
            return "Shotgun";
        case AnimationType::ClipOut:
            return "ClipOut";
        case AnimationType::ClipIn:
            return "ClipIn";
        case AnimationType::SlideBack:
            return "SlideBack";
        case AnimationType::Change:
            return "Change";
        case AnimationType::ThrowWeapon:
            return "ThrowWeapon";
        case AnimationType::WeaponNone:
            return "WeaponNone";
        case AnimationType::Punch:
            return "Punch";
        case AnimationType::ReloadBow:
            return "ReloadBow";
        case AnimationType::Barret:
            return "Barret";
        case AnimationType::Roll:
            return "Roll";
        case AnimationType::RollBack:
            return "RollBack";
        case AnimationType::CrouchRunBack:
            return "CrouchRunBack";
        case AnimationType::Cigar:
            return "Cigar";
        case AnimationType::Match:
            return "Match";
        case AnimationType::Smoke:
            return "Smoke";
        case AnimationType::Wipe:
            return "Wipe";
        case AnimationType::Groin:
            return "Groin";
        case AnimationType::Piss:
            return "Piss";
        case AnimationType::Mercy:
            return "Mercy";
        case AnimationType::Mercy2:
            return "Mercy2";
        case AnimationType::TakeOff:
            return "TakeOff";
        case AnimationType::Prone:
            return "Prone";
        case AnimationType::Victory:
            return "Victory";
        case AnimationType::Aim:
            return "Aim";
        case AnimationType::HandsUpAim:
            return "HandsUpAim";
        case AnimationType::ProneMove:
            return "ProneMove";
        case AnimationType::GetUp:
            return "GetUp";
        case AnimationType::AimRecoil:
            return "AimRecoil";
        case AnimationType::HandsUpRecoil:
            return "HandsUpRecoil";
        case AnimationType::Melee:
            return "Melee";
        case AnimationType::Own:
            return "Own";
    }
}

std::string ControlActionTypeToString(ControlActionType action_type)
{
    switch (action_type) {
        case ControlActionType::ChangeWeapon:
            return "ChangeWeapon";
        case ControlActionType::MoveLeft:
            return "MoveLeft";
        case ControlActionType::MoveRight:
            return "MoveRight";
        case ControlActionType::Jump:
            return "Jump";
        case ControlActionType::Crouch:
            return "Crouch";
        case ControlActionType::Fire:
            return "Fire";
        case ControlActionType::UseJets:
            return "UseJets";
        case ControlActionType::ThrowGrenade:
            return "ThrowGrenade";
        case ControlActionType::DropWeapon:
            return "DropWeapon";
        case ControlActionType::Reload:
            return "Reload";
        case ControlActionType::Prone:
            return "Prone";
        case ControlActionType::ThrowFlag:
            return "ThrowFlag";
    }
}

bool GetControlActionTypeState(const Control& soldier_control,
                               ControlActionType control_action_type)
{
    switch (control_action_type) {
        case ControlActionType::MoveLeft:
            return soldier_control.left;
        case ControlActionType::MoveRight:
            return soldier_control.right;
        case ControlActionType::Jump:
            return soldier_control.up;
        case ControlActionType::Crouch:
            return soldier_control.down;
        case ControlActionType::Fire:
            return soldier_control.fire;
        case ControlActionType::UseJets:
            return soldier_control.jets;
        case ControlActionType::ChangeWeapon:
            return soldier_control.change;
        case ControlActionType::ThrowGrenade:
            return soldier_control.throw_grenade;
        case ControlActionType::DropWeapon:
            return soldier_control.drop;
        case ControlActionType::Reload:
            return soldier_control.reload;
        case ControlActionType::Prone:
            return soldier_control.prone;
        case ControlActionType::ThrowFlag:
            return soldier_control.flag_throw;
    }
}

void AddControlActionTypeIfChanged(const Control& last_control,
                                   const Control& current_control,
                                   ControlActionType control_action_type,
                                   std::vector<ActionRecording>& recorded_actions,
                                   unsigned int game_tick)
{
    bool last_field_state = GetControlActionTypeState(last_control, control_action_type);
    bool current_field_state = GetControlActionTypeState(current_control, control_action_type);
    if (last_field_state != current_field_state) {
        recorded_actions.push_back({ .state = current_field_state,
                                     .action_type = control_action_type,
                                     .game_tick = game_tick });
    }
}

void AddControlActionTypesIfChanged(const Control& last_control,
                                    const Control& current_control,
                                    std::vector<ActionRecording>& recorded_actions,
                                    unsigned int game_tick)
{
    auto all_control_action_types =
      std::array{ ControlActionType::MoveLeft,     ControlActionType::MoveRight,
                  ControlActionType::Jump,         ControlActionType::Crouch,
                  ControlActionType::Fire,         ControlActionType::UseJets,
                  ControlActionType::ChangeWeapon, ControlActionType::ThrowGrenade,
                  ControlActionType::DropWeapon,   ControlActionType::Reload,
                  ControlActionType::Prone,        ControlActionType::ThrowFlag };
    for (auto control_action_type : all_control_action_types) {
        AddControlActionTypeIfChanged(
          last_control, current_control, control_action_type, recorded_actions, game_tick);
    }
}

void AddControlActionTypes(const Control& soldier_control,
                           std::vector<ActionRecording>& recorded_actions,
                           unsigned int game_tick)
{
    auto all_control_action_types =
      std::array{ ControlActionType::MoveLeft,     ControlActionType::MoveRight,
                  ControlActionType::Jump,         ControlActionType::Crouch,
                  ControlActionType::Fire,         ControlActionType::UseJets,
                  ControlActionType::ChangeWeapon, ControlActionType::ThrowGrenade,
                  ControlActionType::DropWeapon,   ControlActionType::Reload,
                  ControlActionType::Prone,        ControlActionType::ThrowFlag };
    for (auto control_action_type : all_control_action_types) {
        recorded_actions.push_back(
          { .state = GetControlActionTypeState(soldier_control, control_action_type),
            .action_type = control_action_type,
            .game_tick = game_tick });
    }
}

void Render(const StateManager& game_state_manager,
            ClientState& client_state,
            double /*frame_percent*/,
            int fps)
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(client_state.mouse.x, client_state.window_height - client_state.mouse.y);
    io.MouseDrawCursor = io.WantCaptureMouse;
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        ImGui::Begin("Network window (Works only when connected to a server)");
        ImGui::Checkbox("Server reconciliation", &client_state.server_reconciliation);
        ImGui::Checkbox("Client side prediction", &client_state.client_side_prediction);
        ImGui::Checkbox("Objects interpolation", &client_state.objects_interpolation);
        ImGui::Text("Non-acknowledged inputs: %llu", client_state.pending_inputs.size());
        ImGui::Text("Ping: %hu", client_state.ping_timer.GetLastPingMeasure());
        ImGui::SliderInt("Fake lag (milliseconds)", &client_state.network_lag, 0, 500);
        ImGui::Checkbox("Draw server POV client position",
                        &client_state.draw_server_pov_client_pos);
        ImGui::End();
    }

    {
        ImGui::Begin("Debug window");
        ImGui::Checkbox("Draw colliding polygons", &client_state.draw_colliding_polygons);
        ImGui::Checkbox("Draw soldier hitboxes", &client_state.draw_soldier_hitboxes);
        ImGui::Checkbox("Draw bullet hitboxes", &client_state.draw_bullet_hitboxes);
        ImGui::Checkbox("Draw item hitboxes", &client_state.draw_item_hitboxes);
        ImGui::Checkbox("Draw collision sectors", &client_state.draw_sectors);
        ImGui::Checkbox("Draw map boundaries", &client_state.draw_map_boundaries);

        ImGui::Checkbox("Smooth camera", &client_state.smooth_camera);
        ImGui::Text("Application average %.3f ms/frame (%d FPS)", 1000.0F / (float)fps, fps);
        ImGui::Text("Bullets in game: %zu", game_state_manager.GetBulletsCount());
        if (ImGui::Button("/kill")) {
            client_state.kill_button_just_pressed = true;
        }
        if (client_state.client_soldier_id.has_value()) {
            game_state_manager.ForEachSoldier([&](const auto& soldier) {
                if (*client_state.client_soldier_id == soldier.id) {
                    ImGui::Text("Player ID: %d", soldier.id);
                    ImGui::Text("Position: %.3f, %.3f",
                                soldier.particle.position.x,
                                soldier.particle.position.y);

                    auto rx = ((int)((soldier.particle.position.x /
                                      (float)game_state_manager.GetConstMap().GetSectorsSize()))) +
                              25;
                    auto ry = ((int)((soldier.particle.position.y /
                                      (float)game_state_manager.GetConstMap().GetSectorsSize()))) +
                              25;

                    ImGui::Text("Sector: %d, %d", rx, ry);
                    if (soldier.active && soldier.on_ground) {
                        ImGui::Text("SOLDIER IS ON GROUND");
                    }
                }
            });
        }
        ImGui::End();
    }

    {
        ImGui::Begin("Weapons");

        ImGui::SeparatorText("Primary weapon");
        static std::vector<std::pair<std::string, WeaponType>> primary_weapons = {
            { "Desert Eagles", WeaponType::DesertEagles },
            { "HK MP5", WeaponType::MP5 },
            { "Ak-74", WeaponType::Ak74 },
            { "Steyr AUG", WeaponType::SteyrAUG },
            { "Spas-12", WeaponType::Spas12 },
            { "Ruger 77", WeaponType::Ruger77 },
            { "M79", WeaponType::M79 },
            { "Barret M82A1", WeaponType::Barrett },
            { "FN Minimi", WeaponType::Minimi },
            { "XM214 Minigun", WeaponType::Minigun },
        };
        for (const auto& weapon_data : primary_weapons) {
            if (ImGui::Selectable(weapon_data.first.c_str(),
                                  client_state.primary_weapon_type_choice == weapon_data.second)) {
                client_state.primary_weapon_type_choice = weapon_data.second;
            }
        }

        ImGui::NewLine();
        ImGui::SeparatorText("Secondary weapon");
        static std::vector<std::pair<std::string, WeaponType>> secondary_weapons = {
            { "USSOCOM", WeaponType::USSOCOM },
            { "Combat Knife", WeaponType::Knife },
            { "Chainsaw", WeaponType::Chainsaw },
            { "M72 LAW", WeaponType::LAW }
        };
        for (const auto& weapon_data : secondary_weapons) {
            if (ImGui::Selectable(weapon_data.first.c_str(),
                                  client_state.secondary_weapon_type_choice ==
                                    weapon_data.second)) {
                client_state.secondary_weapon_type_choice = weapon_data.second;
            }
        }
        ImGui::End();
    }

    {
        // This is only temporary to create movement simulation tests
        ImGui::Begin("Animation Logs");

        static bool recording_started = false;
        static std::vector<AnimationRecording> recorded_animations;
        static AnimationRecording last_added_body_animation;
        static AnimationRecording last_added_legs_animation;
        static unsigned int game_tick_when_started_recording;
        static Control last_control;
        static std::vector<ActionRecording> recorded_actions;
        if (ImGui::Button(recording_started ? "Stop Recording" : "Start Recording")) {
            if (!recording_started) {
                recorded_animations.clear();
                recorded_actions.clear();
                game_tick_when_started_recording = game_state_manager.GetGameTick();

                if (client_state.client_soldier_id.has_value()) {
                    game_state_manager.ForEachSoldier([&](const auto& soldier) {
                        if (*client_state.client_soldier_id == soldier.id) {
                            recorded_animations.push_back(
                              { .is_legs = false,
                                .game_tick = game_state_manager.GetGameTick(),
                                .animation_type = soldier.body_animation->GetType(),
                                .frame = soldier.body_animation->GetFrame(),
                                .speed = soldier.body_animation->GetSpeed(),
                                .soldier_looking_left = soldier.direction == -1,
                                .soldier_position = soldier.particle.position });
                            last_added_body_animation = recorded_animations.back();
                            recorded_animations.push_back(
                              { .is_legs = true,
                                .game_tick = game_state_manager.GetGameTick(),
                                .animation_type = soldier.legs_animation->GetType(),
                                .frame = soldier.legs_animation->GetFrame(),
                                .speed = soldier.legs_animation->GetSpeed(),
                                .soldier_looking_left = soldier.direction == -1,
                                .soldier_position = soldier.particle.position });
                            last_added_legs_animation = recorded_animations.back();

                            last_control = soldier.control;

                            AddControlActionTypes(
                              soldier.control, recorded_actions, game_state_manager.GetGameTick());
                        }
                    });
                }
            }
            recording_started = !recording_started;
        }

        if (recording_started) {
            if (client_state.client_soldier_id.has_value()) {
                game_state_manager.ForEachSoldier([&](const auto& soldier) {
                    if (*client_state.client_soldier_id == soldier.id) {
                        if (last_added_legs_animation.animation_type !=
                            soldier.legs_animation->GetType()) {
                            recorded_animations.push_back(
                              { .is_legs = true,
                                .game_tick = game_state_manager.GetGameTick(),
                                .animation_type = soldier.legs_animation->GetType(),
                                .frame = soldier.legs_animation->GetFrame(),
                                .speed = soldier.legs_animation->GetSpeed(),
                                .soldier_looking_left = soldier.direction == -1,
                                .soldier_position = soldier.particle.position });
                            last_added_legs_animation = recorded_animations.back();
                        }
                        if (last_added_body_animation.animation_type !=
                            soldier.body_animation->GetType()) {
                            recorded_animations.push_back(
                              { .is_legs = false,
                                .game_tick = game_state_manager.GetGameTick(),
                                .animation_type = soldier.body_animation->GetType(),
                                .frame = soldier.body_animation->GetFrame(),
                                .speed = soldier.body_animation->GetSpeed(),
                                .soldier_looking_left = soldier.direction == -1,
                                .soldier_position = soldier.particle.position });
                            last_added_body_animation = recorded_animations.back();
                        }

                        AddControlActionTypesIfChanged(last_control,
                                                       soldier.control,
                                                       recorded_actions,
                                                       game_state_manager.GetGameTick());
                    }
                });
            }
        }

        ImGui::Separator();

        for (const auto& animation_recording : recorded_animations) {
            ImGui::Text("%6d - %s %15s %2d %d %s %.4f %.4f",
                        animation_recording.game_tick - game_tick_when_started_recording,
                        animation_recording.is_legs ? "legs" : "body",
                        AnimationTypeToString(animation_recording.animation_type).c_str(),
                        animation_recording.frame,
                        animation_recording.speed,
                        animation_recording.soldier_looking_left ? "<" : ">",
                        // Difference between the start point and current record
                        animation_recording.soldier_position.x -
                          recorded_animations.at(0).soldier_position.x,
                        animation_recording.soldier_position.y -
                          recorded_animations.at(0).soldier_position.y);
        }

        ImGui::End();

        ImGui::Begin("Actions Log");

        for (const auto& action : recorded_actions) {
            ImGui::Text("%6d - %10s %s",
                        action.game_tick - game_tick_when_started_recording,
                        ControlActionTypeToString(action.action_type).c_str(),
                        action.state ? "true" : "false");
        }

        ImGui::End();

        if (recording_started) {
            if (client_state.client_soldier_id.has_value()) {
                game_state_manager.ForEachSoldier([&](const auto& soldier) {
                    if (*client_state.client_soldier_id == soldier.id) {
                        last_control = soldier.control;
                    }
                });
            }
        }
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool GetWantCaptureMouse()
{
    ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureMouse;
}
} // namespace Soldank::DebugUI
